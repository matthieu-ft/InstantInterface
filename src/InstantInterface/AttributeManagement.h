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

#pragma once

#include "Attributes.h"

#include <vector>
#include <list>
#include <algorithm>
#include <iostream>
#include <map>


namespace InstantInterface {

/**
 * @brief makeSpeed takes a duration in milliseconds, and converts it a normalized transition speed
 * @param duration
 * @return
 */
float makeSpeed(float duration);

/**
 * @brief The Temporal class contains the temporal information that is associated to an operation on an attribute.
 * It is characterized by a normalized speed and by the normalized elapsed time. The normalized elapsed time takes its values in [0,1], it
 * equals 0 at the beginning of the transition and 1 at the end of the transition. The normalized speed is the inverse of the time (in a unit that the user
 * has to define for themself) for the transision to take place.
 * As we want the transition to be smooth, we use a transition weight, which tells the parameter manager how to ponderate the contribution of the current parameter modification in the
 * computation of the attribute value (indeed, several transitions can be active at the same time for the same attribute).
 * There are two kinds of temporals: normal temporals (transition to a new state) and pulses (transition to a temporary state and come back to the previous state).
 * For normal temporals, the weight simply equals the normalized time. For pulses, it is a function of the normalized time (the temporal has then to be implemented
 * with the subclass FunctionTemporal)
 */
class Temporal
{
public:
    Temporal();
    /**
     * @brief Transition
     * @param the speed is the normalized transition speed, wich is the inverse of the period during which the transition will be active. There is no specific unit, but it has to be set coherently together with the unit of elapsed time when update() is called
     */
    Temporal(float speed);
    Temporal(const Temporal& tr);

    /**
     * @brief creates a clone of the current Transition instance
     * @return unique ptr to the new instance
     */
    virtual std::unique_ptr<Temporal> clone();

    /**
     * @brief creates an equivalent of the current instance, copying the transition speed, and the normalized time.
     * It discards any specific behavior that might have been implemented in a subclass.
     * @return unique ptr to the new instance
     */
    std::unique_ptr<Temporal> cloneLinear();

    /**
     * @brief update the temporal given the elapsed time since the last call. The time unit depends of the unit of the temporal speed.
     * @param elapsedTime
     */
    void update(float elapsedTime);

    /**
     * @brief reset the Temporal
     * @param nt normalized time
     * @param speed
     */
    void reset(float nt, float speed);

    /**
     * @brief returns true if the modification associated to the temporal is over, false otherwise
     * @return
     */
    bool done() const;

    /**
     * @brief returns the weight of the modification. It ponderates the influence of the current modification while managing the attribute.
     * It takes its values in [0,1]. 0 make the modification neglectable and 1 makes it very influent.
     * In the default implementation of the Temporal, this is simply the normalized time (it is 0 at the beginning and grows linear in the time towards 1 for a smooth
     * transition towards a new state).
     * @return weight in [0,1]
     */
    virtual float getWeight() const;

    /**
     * @brief returns the normalized time that caracterizes the avancement of the modification. It is 0 if the modification hasn't started yet and it is 1 once the modification is finished.
     * It grows linearly in the time at the speed defined by attribute \p speed
     *
     * @return normalized time
     */
    float getNormalizedTime() const;

    /**
     * @brief Returns true if the temporal is a pulse, false otherwise.
     * A normal temporal brings smoothly to a new state, and stays there. A pulse, however, is a smooth temporary transition back and forth to a state, so that at the end of the transition
     * the attribute has come back to the previous value.
     * @return
     */
    virtual bool isPulse() const;


private:
    float normalizedTime;
    float speed;
    bool modificationDone;
};
typedef std::shared_ptr<Temporal> TemporalPtr;

/**
 * @brief FunctionTemporal is a Temporal which weight is specified by a function of (normalized) time.
 */
template <class FunctionType>
class FunctionTemporal : public Temporal
{
public:
    FunctionTemporal(FunctionType func):
        function(func)
    {}

    FunctionTemporal(float speed, FunctionType func):
        Temporal(speed),
        function(func)
    {
    }

    FunctionTemporal(const FunctionTemporal<FunctionType>& tr):
        Temporal(tr),
        function(tr.function)
    {}

    virtual std::unique_ptr<Temporal> clone()
    {
        return std::unique_ptr<Temporal>(new FunctionTemporal<FunctionType>(*this));
    }

    float getWeight() const
    {
        return function(getNormalizedTime());
    }

    bool isPulse() const
    {
        // the function is considered to be a pulse if at the end the result is not 1 (i.e. the parameter has returned to default state)
        return function(1.0)<0.999;
    }

private:
    FunctionType function;

};


/**
 * list of functions that can be used as a function of normalized time by a temporal. Other functions are of course possible.
 * The requirements are: the function is defined over [0,1] and takes its values [0,1].
 */
namespace TemporalFunctions{
    //x should be between 0 and 1
    float spline(float x);
    float halfSpline(float x);
}


/**
 * creates a temporal for a modification taking place over the duration \p duration and which weight is a function of time defined by \p function
 */
template <class FunctionType>
std::unique_ptr<FunctionTemporal<FunctionType> > makeTemporal(float duration, FunctionType function)
{
    return std::unique_ptr<FunctionTemporal<FunctionType> > ( new FunctionTemporal<FunctionType>(makeSpeed(duration),function));
}

/**
 * @brief makePulse creates a temporal for a pulse-like parameter modification
 * @param duration
 */
inline auto makePulse(float duration)
{
    return makeTemporal(duration,TemporalFunctions::spline);
}

/**
 * @brief The ParameterModifier class  stands is responsible for discribing a modification of a parameter.
 * It is characterized by an aimed value (which is only defined in childclasses because the type of the value has to be known).
 * It contains no information about the temporal dynamic of the modification
 */
class StateModifier
{
public:
    StateModifier();
    virtual ~StateModifier();

    /**
     * @brief getParameterId
     * @return id of the parameter
     */
    virtual std::vector<int> getParameterIds() const = 0;

    /**
     * @brief getEquivalentStaticModifier returns a modifier which aims the current value
     * @return
     */
    virtual std::shared_ptr<StateModifier> getEquivalentStaticModifier_currentState() = 0;

    /**
     * @brief getEquivalentStaticModifier returns a modifier which aims the current at the current aimed value.
     * this is usefull in the case of StateParameterModifierValueT, where the aimed value can change in time.
     * @return
     */
    virtual std::shared_ptr<StateModifier> getEquivalentStaticModifier_aimedState() = 0;

    virtual void mix(float weight) = 0;


    virtual bool isDynamic() const {
        return false;
    }

    virtual bool isPersistent() const {
        return false;
    }

};
typedef std::shared_ptr<StateModifier> StateModifierPtr;

/**
 * @brief The TimedModifier class is a combination of a ParameterModifier and a Temporal. The Temporal describes the temporal dynamics
 * of the modification defined by the ParameterModifier that has to be done to the parameter contained in ParameterModifier
 */
class TimedModifier
{
public:

    TimedModifier();

    TimedModifier(StateModifierPtr mod, std::unique_ptr<Temporal> trans);

    void update(float elapsedTime);

    /**
     * @brief getEquivalentTimedStaticModifier returns a TimedModifier that has the same temporal dynamic (clone of the temporal)
     * and which aimed value is the current value
     * @return
     */
    std::shared_ptr<TimedModifier> getEquivalentTimedStaticModifier();

    /**
     * @brief getEquivalentTimedValueModifier returns a TimedModifier that has the same temporal dynamic (clone of the temporal)
     * and which aimed value is the aimed value. This is used for instance in the case of TimedStateModifier to create an equivalent
     * TimedValueModifier that is independant of the future state changes.
     * @return
     */
    std::shared_ptr<TimedModifier> getEquivalentTimedValueModifier();

    /**
     * @brief clone  returns a TimedModifier that has exactly the same behavior (i.e. a shared copy of the modifier and a clone of the temporal)
     * @return
     */
    std::shared_ptr<TimedModifier> clone();

    /**
     * @brief mutateToStaticModifier  replaces the current modifier with the equivalent static modifier
     */
    void mutateToStaticModifier();

    /**
     * @brief mutateToValueModifier replaces the current modifier with the equivalent value modifier
     */
    void mutateToValueModifier();

    std::unique_ptr<Temporal>& getTemporal() { return temporal;}
    std::shared_ptr<StateModifier> getModifier() { return modifier; }

private:
    std::unique_ptr<Temporal> temporal;
    StateModifierPtr modifier;
};

typedef std::shared_ptr<TimedModifier> TimedModifierPtr;
typedef std::weak_ptr<TimedModifier> TimedModifierWeakPtr;

///**
// * @brief The ParameterModifierMixer class is responsible for combining the output values of TimedModifier of the same parameter.
// * It has to be created every time anew for each new computation of the value of the parameter. First, you call mix() applied to each TimedModifier
// * instance that contributes to the computation. Then it is sufficient to call applyToAttribute() for the computation to be finalized and the attribute to be updated.
// * Its child templated class ParameterModifierMixerT knows the type of the parameter and therefore is able to do the computation with the correct type.
// * It is created by calling method ParameterModifier::getMixer().
// */
//class ParameterModifierMixer
//{
//public:
//    /**
//     * @brief mix, adds the contribution of the given timedModifier to the computation of the new value of the associated parameter
//     * @param timedModifier
//     */
//    virtual void mix(TimedModifier& timedModifier) = 0;
//    /**
//     * @brief applyToAttribute finalizes the computation and update the attribute with the new value
//     */
//    virtual void applyToAttribute() = 0;
//};


//template <class ParamType> class ParameterModifierMixerT;
template <class ParamType> class ParameterModifierValueT;

/**
 * @brief see ParameterModifier for more information
 */
template <class ParamType>
class ParameterModifierT : public StateModifier, public std::enable_shared_from_this<ParameterModifierT<ParamType> >
{
public:
    ParameterModifierT(std::shared_ptr<AttributeT<ParamType> > pAttr):
        StateModifier(),
        attr(pAttr),
        persistence(false)
    {}

    virtual ParamType aimedValue() const = 0;

    std::shared_ptr<StateModifier> getEquivalentStaticModifier_currentState()
    {
        return std::make_shared<ParameterModifierValueT<ParamType> > (getAttribute(), getAttribute()->get());
    }

    std::shared_ptr<StateModifier> getEquivalentStaticModifier_aimedState()
    {
        return std::make_shared<ParameterModifierValueT<ParamType> > (getAttribute(), aimedValue());
    }

    std::vector<int> getParameterIds() const
    {
        int id = -1;
        if(auto p = attr.lock())
        {
            id = p->getId();
        }
        return {id};
    }

    std::shared_ptr<AttributeT<ParamType> > getAttribute() const
    {
        return attr.lock();
    }

    void mix(float transitionFactor) {
        auto pAttr = attr.lock();
        if(transitionFactor < 0)
            return;

        if(transitionFactor >= 1)
            pAttr->set(aimedValue());
        else
            pAttr->set((1-transitionFactor)*pAttr->get() + transitionFactor * aimedValue());

    }

    std::shared_ptr<ParameterModifierT<ParamType> > setPersistence(bool v){
        persistence = v;
        return this->shared_from_this();
    }


    virtual bool isPersistent() const {
        return persistence;
    }


private:
    std::weak_ptr<AttributeT<ParamType> > attr;
    bool persistence;
};


/**
 * @brief the ParameterModifierValueT class is a parameter modifier which aimed value is constant.
 */
template <class ParamType>
class ParameterModifierValueT : public ParameterModifierT<ParamType>
{
public:
    /**
     * @brief ParameterModifierValueT constructor
     * @param pAttr  attribute that modifier being currently created will be in charge of
     * @param val  aimed value of the modifier
     */
    ParameterModifierValueT(std::shared_ptr<AttributeT<ParamType> > pAttr, ParamType val):
        ParameterModifierT<ParamType>(pAttr),
        _aimedValue(val)
    {}


    ParamType aimedValue() const
    {
        return _aimedValue;
    }

private:
    ParamType _aimedValue;
};


/**
 * @brief the StateParameterModifierValueT class is a parameter modifier which functions with states. It is responsible for making smooth transition between states.
 * It is constructed with a vector of values that define the states. The values have to be ordered in increasing order.
 * The states are indexed from 0 to N-1, in the order of the values in the state vector, N being the length of the vector.
 */
template <class ParamType>
class IndexedStateModifierT: public ParameterModifierT<ParamType>
{
public:

    /**
     * @brief StateParameterModifierValueT constructor
     * @param pAttr attribute that the modifier being currently created will be in charge of
     * @param vals  state vector, with the values orders in increasing order
     */
    IndexedStateModifierT(std::shared_ptr<AttributeT<ParamType> > pAttr, const std::vector<ParamType>& vals):
        ParameterModifierT<ParamType>(pAttr),
        values(vals),
        aimedIndex(0),
        bDiscardLastAimedIndex(true)
    {
    }

    ParamType aimedValue() const
    {
        return getValueAtIndex(aimedIndex);
    }

    /**
     * @brief setAimedIndex defines new aimed state
     * @param ind
     */
    void setAimedIndex(int ind)
    {
        aimedIndex = ind;
        bDiscardLastAimedIndex = false;
    }

    /**
     * @brief currentIndex returns the index that is the closest to the current value
     * @return
     */
    int currentIndex() const
    {
        ParamType attrValue = this->getAttribute()->get();
        return closestIndex(attrValue);
    }

    /**
     * @brief discardLastIndex the call of this method will lead to discard the previous aimed state index, when the state index gets updated
     */
    void discardLastIndex()
    {
        bDiscardLastAimedIndex = true;
    }

    /**
     * @brief indexCloseToCurrentIndex. If the current attribute is periodic, the function will return the index equivalent to the one given as argument
     * that is the closest to the current value, and returns the method argument directly otherwise.
     * @param ind index
     * @return closest equivalent index
     */
    int indexCloseToCurrentIndex(int ind)
    {
        if(!this->getAttribute()->isPeriodic())
            return ind;

        int valSize = (int)values.size();
        int delta = ((ind-currentIndex())%valSize);
        int finalDelta;
        if(delta>0)
        {
            if(delta< valSize-delta)
            {
                finalDelta = delta;
            }
            else
            {
                finalDelta = -(valSize-delta);
            }
        }
        else
        {
            if(-delta<valSize+delta)
            {
                finalDelta = delta;
            }
            else
            {
                finalDelta = valSize+delta;
            }
        }
        return currentIndex() + finalDelta;
    }

    /**
     * @brief closestIndex
     * @return closest state index to the value given as argument
     */
    int closestIndex(ParamType attrValue) const
    {
        int closestId = closestModuloIndex(attrValue);

        if(this->getAttribute()->isPeriodic())
        {
            closestId += (int)(floor(attrValue/this->getAttribute()->getPeriod())) * values.size();
        }

        return closestId;
    }

    /**
     * @brief closestIndexModulo returns the closest state index in the range [0, N-1], N being the number of states
     * @return closest state index in range [0,N-1]
     */
    int closestModuloIndex(ParamType attrValue) const
    {
        int closestId = 0;

        auto moduloDist = [this](ParamType v1, ParamType v2){

            if(this->getAttribute()->isPeriodic())
            {
                ParamType period = this->getAttribute()->getPeriod();
                ParamType modulo = std::abs(std::fmod(v1-v2,period));
                ParamType distance = std::min(modulo, period-modulo);
                return distance;
            }
            else
            {
                return std::abs(v1-v2);
            }
        };

        if(values.size()>0)
        {
            ParamType minDist = moduloDist(attrValue,values[0]);
            size_t i = 0;
            for(size_t i =1; i<values.size(); i++)
            {
                ParamType distance = moduloDist(values[i],attrValue);
                if(distance<minDist)
                {
                    minDist = distance;
                    closestId = i;
                }
            }
        }
        return closestId;
    }

    /**
     * @brief resetIndex index to the closest state
     */
    void resetIndex()
    {
        aimedIndex = currentIndex();
        bDiscardLastAimedIndex = false;
    }

    /**
     * @brief getValueAtIndex returns the value associated to the state of index \p index
     * @param index
     * @return value of the state of index \p index
     */
    ParamType getValueAtIndex(int index) const
    {
        ParamType val;
        if(values.size()>0)
        {
            if(this->getAttribute()->isPeriodic())
            {
                int nSteps = (int)floor((float)index/(float)values.size());
                int indexPeriod = (int)values.size();
                int indexModulo = index%indexPeriod;

                if(indexModulo<0)
                    indexModulo =  indexPeriod+indexModulo;

                val = values[indexModulo] + (ParamType)nSteps*this->getAttribute()->getPeriod();
            }
            else
            {
                val = values[index];
            }
        }
        return val;
    }

    /**
     * @brief updateIndex updates index given an index delta.
     * If reset index is false, then the new aimed index will depend on the last aimed index, on the current value and on the index delta.
     * If reset index is true, the new aimed index will depend only on the current value and the index delta.
     * @param delta index delta
     * @param resetIndex
     */
    void updateIndex(int delta)
    {
        // if the index delta brings in the same direction as the aimed value and the index is not to be reset, then simply update
        // the aimed index with the delta
        if((this->aimedValue()-this->getAttribute()->get())*(float)delta >= 0 && !bDiscardLastAimedIndex)
        {
            aimedIndex += delta;
        }
        // otherwise, take into account the current value in order to have a smooth a coherent transition
        else
        {
            ParamType attrValue = this->getAttribute()->get();
            int index = closestIndex(attrValue);
            float valueAtIndex = getValueAtIndex(index);
            if(attrValue - valueAtIndex>0.00000001)
            {
                if(delta>0)
                {
                    aimedIndex = index + delta;
                }
                else
                {
                    aimedIndex = index + delta + 1;
                }
            }
            else if (attrValue - valueAtIndex<-0.00000001)
            {
                if(delta>0)
                {
                    aimedIndex = index + delta - 1;
                }
                else
                {
                    aimedIndex = index + delta;
                }
            }
            else
            {
                aimedIndex = index + delta;
            }
        }

        bDiscardLastAimedIndex = false;

        if(!this->getAttribute()->isPeriodic())
        {
            aimedIndex = std::min(aimedIndex, (int)values.size()-1);
            aimedIndex = std::max(0, aimedIndex);
        }
    }

    /**
     * @brief getIndexDelta
     * @return index delta between aimed index and current index
     */
    int getIndexDelta() const{
        return aimedIndex-currentIndex();
    }

private:
    std::vector<ParamType> values;
    bool bDiscardLastAimedIndex;
    int aimedIndex;
};

std::unique_ptr<Temporal> makeSpeedTransition(float speed);
std::unique_ptr<Temporal> makeDurationTransition(float duration);

/**
 * @brief  creates a value modifier for the attribute \p attr that will modify it to the aimed value \p val
 */
template <class ParamType, class T2>
std::shared_ptr<ParameterModifierValueT<ParamType> > makeValueModifier(std::shared_ptr<AttributeT<ParamType> > attr, T2 val)
{
    return std::make_shared<ParameterModifierValueT<ParamType> >(attr, val);
}

/**
 * @brief  creates a state value modifier for the attribute \p attr with the states defined in \p vals. The values in \p vals have
 * to be stored in increasing order
 */
template <class ParamType, class T2>
std::shared_ptr<IndexedStateModifierT<ParamType> > makeStateValueModifier(std::shared_ptr<AttributeT<ParamType> > attr, std::vector<T2> vals)
{
    return std::make_shared<IndexedStateModifierT<ParamType> >(attr,vals);
}

///**
// * @brief see ParameterModifierMixer for more information
// */
//template <class ParamType>
//class ParameterModifierMixerT: public ParameterModifierMixer
//{
//public:
//    ParameterModifierMixerT():
//        temp(),
//        hasBeenInit(false),
//        attr()
//    {}

//    virtual void mix(TimedModifier&  timedModif)
//    {
//        auto modifierT = std::static_pointer_cast<ParameterModifierT<ParamType> >(timedModif.getModifier());
//        float transitionFactor = timedModif.getTemporal()->getWeight();
//        if(hasBeenInit)
//        {
//            temp = (1-transitionFactor)*temp + transitionFactor * modifierT->aimedValue();
//        }
//        else
//        {
//            temp = modifierT->aimedValue();
//            hasBeenInit = true;
//            attr = modifierT->getAttribute();
//        }
//    }

//    void applyToAttribute()
//    {
//        if(auto p = attr.lock())
//        {
//            p->set(temp);
//        }
//        else
//        {
//            std::cout<<"Parameter modifier mixer t, applyResult: couldn't lock attribute."<<std::endl;
//        }
//    }

//private:
//    ParamType temp;
//    bool hasBeenInit;
//    std::weak_ptr<AttributeT<ParamType> > attr;
//};

/**
 * @brief The DynamicConfiguration class stores and manages all the active TimedModifiers instances.
 * The method apply() updates all the attributes given their associated timed modifiers.
 */
class DynamicConfiguration
{
public:

    typedef std::list< std::shared_ptr<StateModifier> > ModifierVec;
    typedef std::list< std::shared_ptr<TimedModifier> > TimedModifierVec;

    /**
     * @brief add  adds a deep copy of the TimedModifier instance \p timeMod used as argument to the active timed modifiers.
     * It will be taken into account during the apply.
     * @param timeMod timed modifier
     * @return shared_ptr to the deep copy of \p timeMod
     */
    std::shared_ptr<TimedModifier>  add(TimedModifierPtr timeMod);

    /**
     * @brief add adds a list of mofifiers with a given duration \p duration.
     * TimedModifier instances are created based on the arguements and then added to the active timed modifiers.
     * @param mods vector of modifiers
     * @param duration  duration of the transition
     */
    void add(const std::vector<StateModifierPtr>& mods, float duration);

    auto makeTransitionLambda(const std::vector<StateModifierPtr>& config, float duration)
    {
        return [this, config, duration](){
            this->add(config,duration);
        };
    }

    auto makeTransitionLambda(std::shared_ptr<TimedModifier> timedModif)
    {
        return [this, timedModif](){
            this->add(timedModif);
        };
    }

    void notifyRequiredUpdate(int paramId);

    /**
     * @brief apply goes through all the registered instances of TimedModifier and updates the values of the associated parameters
     * @param elapsedTime
     */
    void apply(float elapsedTime);

    /**
     * @brief reset  resets the collection of timed modifiers
     */
    void reset();

private:
    std::map<int, TimedModifierVec> timedModifiersCollection;
    std::list<TimedModifierWeakPtr> timedModifiers;
    std::map<int, bool>  updateRequirements;
};

std::shared_ptr<Action> makeTransitionAction (DynamicConfiguration& dc, const std::vector<StateModifierPtr> & acts, FloatAttribute transitionSpeed);

std::shared_ptr<Action> makeImpulseAction( DynamicConfiguration& dc, const std::vector<StateModifierPtr> & acts, FloatAttribute impulseSpeed);
std::shared_ptr<Action> makeImmediateImpulseAction( DynamicConfiguration& dc, const std::vector<StateModifierPtr> & acts, FloatAttribute impulseSpeed);


/**
 * @brief The StateParameterModifierManager class is responsible managing the modifier of a state attribute.
 */
class StateParameterModifierManager
{
public:
    /**
     * @brief applyDelta updates the state index of the associated attribute with the relative value of \p delta and at speed \p speed
     * @param delta
     * @param speed
     */
    virtual void applyDeltaSpeed(DynamicConfiguration& dc, int delta, float speed) = 0;
    virtual void goToIndexSpeed(DynamicConfiguration& dc, int index, float speed) = 0;

    auto makeDeltaTransition(DynamicConfiguration& dc, int delta, float duration)
    {
        return [&dc, delta, duration, this](){
            this->applyDeltaSpeed(dc,delta,makeSpeed(duration));
        };
    }

    auto makeIndexTransition(DynamicConfiguration& dc, int index, float duration)
    {
        return [&dc, index, duration, this](){
            this->goToIndexSpeed(dc,index,makeSpeed(duration));
        };
    }
};

/**
 * @brief The StateParameterModifierManagerT class is a type aware implementation of StateParameterModifierManager
 */
template <class ParamType>
class StateParameterModifierManagerT: public StateParameterModifierManager
{
public:

    StateParameterModifierManagerT(std::shared_ptr<IndexedStateModifierT<ParamType> > mod):
        modifier(mod)
    {}



    void applyDeltaSpeed(DynamicConfiguration& dc, int delta, float speed)
    {
        processLastTimedModifier();
        modifier->updateIndex(delta);
        addModifierToConfiguration(dc, speed);
    }

    virtual void goToIndexSpeed(DynamicConfiguration& dc, int index, float speed)
    {
        processLastTimedModifier();
        modifier->setAimedIndex(modifier->indexCloseToCurrentIndex(index));
        addModifierToConfiguration(dc, speed);
    }

private:
    std::shared_ptr<IndexedStateModifierT<ParamType> > modifier;
    std::shared_ptr<TimedModifier> lastTimedModifier;

    /**
     * @brief processLastTimedModifier
     */
    void processLastTimedModifier()
    {
        if(lastTimedModifier)
        {
            lastTimedModifier->mutateToValueModifier();

            if(lastTimedModifier->getTemporal()->done())
            {
                modifier->discardLastIndex();
            }
        }
    }

    /**
     * @brief addModifierToConfiguration
     * @param speed
     */
    void addModifierToConfiguration(DynamicConfiguration& dc, float speed){

        auto timedModifier = std::make_shared<TimedModifier>(modifier,makeSpeedTransition(speed));
        lastTimedModifier = dc.add(timedModifier);
    }

};

/**
 * makeStateParameterManager is a helper function that generates a StateParameterModifierManager for the attribute \p pAttr, the state vector \p vals
 */
template <class ParamType>
std::shared_ptr <StateParameterModifierManagerT<ParamType> > makeStateParameterManager(std::shared_ptr<AttributeT<ParamType> > pAttr, const std::vector<ParamType>& vals)
{
    return std::make_shared<StateParameterModifierManagerT<ParamType> > (makeStateValueModifier(pAttr,vals));
}


/**
 * makeImpulse is a helper function that generates an impulse based on the attribute \p attr, to the value \p val and with duration \p duration
 */
template <class ParamType, class T2>
std::shared_ptr<TimedModifier> makeImpulse(std::shared_ptr<AttributeT<ParamType> > attr, T2 val, float duration)
{
    return std::make_shared<TimedModifier>(makeValueModifier(attr,val),makeTemporal(duration,TemporalFunctions::spline));
}

std::shared_ptr<TimedModifier> makeImpulse(std::shared_ptr<StateModifier> modifier, float duration);

template <class ParamType, class T2>
std::shared_ptr<TimedModifier> makeImmediateImpulse(std::shared_ptr<AttributeT<ParamType> > attr, T2 val, float duration)
{
    return std::make_shared<TimedModifier>(makeValueModifier(attr,val),makeTemporal(duration,TemporalFunctions::halfSpline));
}

std::shared_ptr<TimedModifier> makeImmediateImpulse(std::shared_ptr<StateModifier> modifier, float duration);


}
