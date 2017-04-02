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

    TimedModifier::TimedModifier(std::shared_ptr<ParameterModifier> mod, std::unique_ptr<Temporal> trans):
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

    ParameterModifier::ParameterModifier()
    {}

    ParameterModifier::~ParameterModifier() {}

    std::shared_ptr<TimedModifier> DynamicConfiguration::add(std::shared_ptr<TimedModifier> timeMod)
    {
        int paramId = timeMod->getModifier()->getParameterId();
        auto modIt = timedModifiersCollection.find(paramId);
        if(modIt == timedModifiersCollection.end())
        {
            timedModifiersCollection[paramId] = {};
            //modifiers[paramId] = {};
        }
        if(timedModifiersCollection[paramId].size() == 0)
        {
            timedModifiersCollection[paramId].push_back(timeMod->getEquivalentTimedStaticModifier());
        }

        auto insertedInstance = timeMod->clone();
        timedModifiersCollection[paramId].push_back(insertedInstance);

        updateRequirements[paramId] = true;

        return insertedInstance;
    }

    void DynamicConfiguration::add(const DynamicConfiguration::ModifierVec &mods, float duration)
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
        for(auto& paramModsPair: timedModifiersCollection )
        {
            auto& timedModifs = paramModsPair.second;
            int paramId = paramModsPair.first;

            if(timedModifs.size() == 1
                    && timedModifs[0]->getTemporal()->done()
                    && !timedModifs[0]->getModifier()->isDynamic()
                    && timedModifs[0]->getModifier()->isPersistent()
                    && updateRequirements[paramId])
            {
                auto pTimedMod = timedModifs[0];
                auto mixer = pTimedMod->getModifier()->getMixer();
                pTimedMod->getTemporal()->update(elapsedTime); // we shouldn't need to do that because the mixing phase is over but maybe it is more coherent like that...
                mixer->mix(*pTimedMod);
                mixer->applyToAttribute();
            }
            else if(timedModifs.size()>0)
            {
                auto mixer = timedModifs[0]->getModifier()->getMixer();

                for(auto pTimeMod : timedModifs)
                {
                    pTimeMod->getTemporal()->update(elapsedTime);
                    mixer->mix(*pTimeMod);
                }

                //update the parameter
                mixer->applyToAttribute();
            }

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

            //we let at least one non pulse finished element at the beginning (if there is one), in case there are pulse afterwards

            timedModifs.erase(timedModifs.begin(), splitIt);

            if(timedModifs.size() == 1)
            {
                if(!timedModifs[0]->getModifier()->isPersistent() && timedModifs[0]->getTemporal()->done())
                {
                    timedModifs.clear();
                }
            }
        }
    }

    void DynamicConfiguration::reset()
    {
        timedModifiersCollection.clear();
    }

    std::shared_ptr<InstantInterface::TimedModifier> makeImpulse(std::shared_ptr<InstantInterface::ParameterModifier> modifier, float duration)
    {
        return std::make_shared<TimedModifier>(modifier,makeTemporal(duration,TemporalFunctions::spline));
    }

    std::shared_ptr<Action> makeTransitionAction(DynamicConfiguration &dc, const DynamicConfiguration::ModifierVec &acts, FloatAttribute transitionSpeed){
        return AttributeFactory::makeAction([acts,&dc, transitionSpeed](){
            dc.add(acts,1000.0f/(std::max(0.001f,transitionSpeed->get())));
        });
    }

    std::shared_ptr<Action> makeImpulseAction(DynamicConfiguration &dc, const DynamicConfiguration::ModifierVec &acts, FloatAttribute impulseSpeed){
        return AttributeFactory::makeAction([acts,&dc, impulseSpeed](){
            float impulseDuration = 1000.0f/(std::max(0.001f,impulseSpeed->get()));
            for(auto modif : acts)
                dc.add(makeImpulse(modif,impulseDuration));
        });
    }

}
