/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//                           License Agreement
//                      For InstantInterface Library
//
// The MIT License (MIT)
//
// Copyright (c) 2016 Matthieu Fraissinet-Tachet (www.matthieu-ft.com)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to use,
//  copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do so,
//  subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies
//  or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
// OR OTHER DEALINGS IN THE SOFTWARE.
//
//M*/

#include "AttributeManagement.h"

namespace InstantInterface {


    float makeSpeed(float duration)
    {
        return 1.0f/std::max(1.0f,duration);
    }

    TimedModifier::TimedModifier(): temporal() {}

    void TimedModifier::update(float elapsedTime) {
        getTemporal()->update(elapsedTime); // we shouldn't need to do that because the mixing phase is over but maybe it is more coherent like that...
        getModifier()->mix(getTemporal()->getWeight());
    }

    TimedModifier::TimedModifier(std::shared_ptr<StateModifier> mod, std::unique_ptr<Temporal> trans):
        temporal(std::move(trans)),
        modifier(mod) {}

    std::shared_ptr<TimedModifier> TimedModifier::getEquivalentTimedStaticModifier()
    {
        return std::make_shared<TimedModifier>(getModifier()->getEquivalentStaticModifier_currentState(),getTemporal()->cloneLinear());
    }

    std::shared_ptr<TimedModifier> TimedModifier::getEquivalentTimedValueModifier()
    {
        return std::make_shared<TimedModifier>(getModifier()->getEquivalentStaticModifier_aimedState(), getTemporal()->clone());
    }

    std::shared_ptr<TimedModifier> TimedModifier::clone()
    {
        return std::make_shared<TimedModifier>(getModifier(), getTemporal()->clone());
    }

    void TimedModifier::mutateToStaticModifier()
    {
        modifier = modifier->getEquivalentStaticModifier_currentState();
    }

    void TimedModifier::mutateToValueModifier()
    {
        modifier = modifier->getEquivalentStaticModifier_aimedState();
    }

    std::unique_ptr<Temporal> makeSpeedTransition(float speed)
    {
        return std::unique_ptr<Temporal>(new Temporal(speed));
    }

    std::unique_ptr<Temporal> makeDurationTransition(float duration)
    {
        return std::unique_ptr<Temporal>(new Temporal(makeSpeed(duration)));
    }

    float TemporalFunctions::spline(float y)
    {
        float x = 3*y;
        float output;
        if(x>1)
        {
            if(x<2)
            {
                output = x*(-2*x + 6) - 3;
            }
            else if(x<3)
            {
                output = x*(x-6)+9;
            }
            else
            {
                output = 0;
            }
        }
        else if (x>0)
        {
            output = x*x;
        }
        else
        {
            output = 0;
        }

        return output*0.6666666;
    }


    float TemporalFunctions::halfSpline(float x)
    {
        return spline(x+0.5f);
    }

    Temporal::Temporal():
        normalizedTime(0),
        speed(1),
        modificationDone(true)
    {}

    Temporal::Temporal(float speed):
        normalizedTime(0),
        speed(speed),
        modificationDone(false)
    {}

    Temporal::Temporal(const Temporal &tr):
        normalizedTime(tr.normalizedTime),
        speed(tr.speed),
        modificationDone(tr.modificationDone)
    {}

    std::unique_ptr<Temporal> Temporal::clone()
    {
        return std::unique_ptr<Temporal>(new Temporal(*this));
    }

    std::unique_ptr<Temporal> Temporal::cloneLinear()
    {
        return std::unique_ptr<Temporal>(new Temporal(*this));
    }

    void Temporal::update(float elapsedTime)
    {
        normalizedTime += speed*elapsedTime;
        if(normalizedTime>1)
        {
            normalizedTime = 1;
            modificationDone = true;
        }
    }

    void Temporal::reset(float factor, float speed)
    {
        this->normalizedTime = factor;
        this->speed = speed;
        this->modificationDone = false;
    }

    bool Temporal::done() const
    {
        return modificationDone;
    }

    float Temporal::getWeight() const
    {
        return normalizedTime;
    }

    float Temporal::getNormalizedTime() const
    {
        return normalizedTime;
    }

    bool Temporal::isPulse() const
    {
        return false;
    }

    StateModifier::StateModifier()
    {}

    StateModifier::~StateModifier() {}

    std::shared_ptr<TimedModifier> DynamicConfiguration::add(TimedModifierPtr timeMod)
    {
        auto paramIds = timeMod->getModifier()->getParameterIds();

        for(auto id : paramIds)
        {
            auto modIt = timedModifiersCollection.find(id);
            if(modIt == timedModifiersCollection.end())
            {
                timedModifiersCollection[id] = {};
            }
        }

        auto paramId = paramIds[0];

        if(paramIds.size() == 1
                && timedModifiersCollection[paramId].size() == 0 )
        {
            auto staticEquivalent = timeMod->getEquivalentTimedStaticModifier();
            timedModifiersCollection[paramId].push_back(staticEquivalent);
            timedModifiers.push_back(staticEquivalent);
        }

        auto insertedInstance = timeMod->clone();

        for(auto id : paramIds)
        {
            timedModifiersCollection[id].push_back(insertedInstance);
            updateRequirements[paramId] = true;
        }

        timedModifiers.push_back(insertedInstance);

        return insertedInstance;
    }

    void DynamicConfiguration::add(const std::vector<StateModifierPtr> &mods, float duration)
    {
        for(auto mod: mods)
        {
            add(std::make_shared<TimedModifier>(mod,makeDurationTransition(duration)));
        }
    }

    void DynamicConfiguration::notifyRequiredUpdate(int paramId)
    {
        updateRequirements[paramId] = true;
    }

    void DynamicConfiguration::apply(float elapsedTime)
    {

        /*
         * apply updates
         */
        auto weakTmIt = timedModifiers.begin();
        while(weakTmIt != timedModifiers.end())
        {
            auto pTm = weakTmIt->lock();
            if(!pTm)
                // if pTm is a null ptr, it means that there is no shared_ptr
                // pointing to the object
                // in the timed modifier map, so that it doesn't need to be applied anymore
                // and can be removed from the list
            {
                weakTmIt = timedModifiers.erase(weakTmIt);
                continue;
            }

            pTm->update(elapsedTime);
            weakTmIt++;
        }

        for(auto& paramModsPair: timedModifiersCollection )
        {
            auto& timedModifs = paramModsPair.second;
            int paramId = paramModsPair.first;

//            if(timedModifs.size() == 1
//                    && timedModifs.front()->getTemporal()->done()
//                    && !timedModifs.front()->getModifier()->isDynamic()
//                    && timedModifs.front()->getModifier()->isPersistent()
//                    && updateRequirements[paramId])
//            {
//                auto pTimedMod = timedModifs.front();
//                pTimedMod->update(elapsedTime);
//            }

            updateRequirements[paramId] = false;


            /* delete modifiers that have no influence any more
             */

            auto timeModIt = timedModifs.begin();
            auto splitIt = timedModifs.begin();

            while(timeModIt!=timedModifs.end())
            {
                if((*timeModIt)->getTemporal()->done())
                {
                    if((*timeModIt)->getTemporal()->isPulse())
                    {
                        //this factor has no influence anymore, we can remove it right away (this was an impulse like timed modifier)
                        timeModIt = timedModifs.erase(timeModIt);
                        continue;
                    }
                    splitIt = timeModIt;
                }
                ++timeModIt;
            }

            timedModifs.erase(timedModifs.begin(), splitIt);

            if(timedModifs.size() == 1)
            {
                if(!timedModifs.front()->getModifier()->isPersistent() && timedModifs.front()->getTemporal()->done())
                {
                    timedModifs.clear();
                }
            }
        }
    }

    void DynamicConfiguration::reset()
    {
        timedModifiersCollection.clear();
        timedModifiers.clear();
        updateRequirements.clear();
    }

    std::shared_ptr<InstantInterface::TimedModifier> makeImpulse(std::shared_ptr<InstantInterface::StateModifier> modifier, float duration)
    {
        return std::make_shared<TimedModifier>(modifier,makeTemporal(duration,TemporalFunctions::spline));
    }

    std::shared_ptr<TimedModifier> makeImmediateImpulse(std::shared_ptr<StateModifier> modifier, float duration)
    {
        return std::make_shared<TimedModifier>(modifier, makeTemporal(duration, TemporalFunctions::halfSpline));
    }

    std::shared_ptr<Action> makeTransitionAction(DynamicConfiguration &dc, const std::vector<StateModifierPtr> &acts, FloatAttribute transitionSpeed){
        return AttributeFactory::makeAction([acts,&dc, transitionSpeed](){
            dc.add(acts,1000.0f/(std::max(0.001f,transitionSpeed->get())));
        });
    }

    std::shared_ptr<Action> makeImpulseAction(DynamicConfiguration &dc, const std::vector<StateModifierPtr> &acts, FloatAttribute impulseSpeed){
        return AttributeFactory::makeAction([acts,&dc, impulseSpeed](){
            float impulseDuration = 1000.0f/(std::max(0.001f,impulseSpeed->get()));
            for(auto modif : acts)
                dc.add(makeImpulse(modif,impulseDuration));
        });
    }

    std::shared_ptr<Action> makeImmediateImpulseAction(DynamicConfiguration &dc, const std::vector<StateModifierPtr> &acts, FloatAttribute impulseSpeed)
    {
        return AttributeFactory::makeAction([acts,&dc, impulseSpeed](){
            float impulseDuration = 1000.0f/(std::max(0.001f,impulseSpeed->get()));
            for(auto modif : acts)
                dc.add(makeImmediateImpulse(modif,impulseDuration));
        });
    }


}
