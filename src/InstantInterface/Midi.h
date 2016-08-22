#include <RtMidi.h>
#include <memory>
#include <map>
#include <vector>
#include <math.h>

#include "Attributes.h"
#include "AttributeManagement.h"

namespace RemoteInterface
{


class MidiParameter
{
public:
    MidiParameter()
    {}

    virtual void turned(size_t val, float timestamps) = 0;
    virtual void button(bool pressed) {}
    virtual void applyUpdate() = 0;

    MidiParameter* sensibility(float s);

    float sensibility() const;


protected:
    float _sensibility;
};

class MidiAction : public MidiParameter
{
public:
    MidiAction(std::shared_ptr<RemoteInterface::Action> a):
        action(a),
        lastButtonState(false)
    {}

    virtual void turned(size_t val, float timestamps){}

    virtual void button(bool pressed) {
        if(pressed && !lastButtonState)
        {
            action->applyAction();
        }
        lastButtonState = pressed;
    }

    void applyUpdate(){}


protected:
    std::shared_ptr<RemoteInterface::Action> action;
    bool lastButtonState;
//    float _min;
//    float _max;
//    bool _hasMin;
//    bool _hasMax;
};

template <typename DATA_TYPE>
class ValueMidiParameter_T: public MidiParameter
{
public:
    ValueMidiParameter_T(std::shared_ptr<RemoteInterface::AttributeT<DATA_TYPE> > sp):
        _attribute(sp),
        _intermediateState(0),
        _needsUpdate(false)
    {}
    ValueMidiParameter_T(std::weak_ptr<RemoteInterface::AttributeT<DATA_TYPE> > sp):
        _attribute(sp),
        _intermediateState(0),
        _needsUpdate(false)
    {}

    virtual void turned(size_t val, float timestamps)
    {
        _intermediateState += ((float)val - 64.0f) * this->sensibility();
        _needsUpdate = true;
    }

    virtual void applyUpdate()
    {
        if(_needsUpdate)
        {
            if(auto attr = _attribute.lock())
            {
                DATA_TYPE newState = attr->get() + (DATA_TYPE) _intermediateState;
                attr->set(newState);
                _intermediateState = 0;
            }
            else
            {
                std::cout<<"Dangling weak ptr"<<std::endl;
            }

            _needsUpdate = false;
        }
    }

private:
    std::weak_ptr <RemoteInterface::AttributeT<DATA_TYPE> > _attribute;
    bool _needsUpdate;
    float _intermediateState;
};

class StateKnob: public MidiParameter
{
public:
    StateKnob(std::shared_ptr<RemoteInterface::StateParameterModifierManager> sm, int fa):
        stateManager(sm),
        factor(fa)
    {}

    virtual void turned(size_t val, float timestamps)
    {
        if(auto pt = stateManager.lock())
        {
            pt->applyDelta(factor*((int)val - 64), RemoteInterface::makeSpeed(1000.0f));
        }
    }

    void applyUpdate() {}

private:
    std::weak_ptr <RemoteInterface::StateParameterModifierManager > stateManager;
    int factor;
};

StateKnob* makeStateKnob (std::shared_ptr<RemoteInterface::StateParameterModifierManager> sm, int factor = 1);


float factorModulation(float speedFactor, float timeFactor);

template <typename DATA_TYPE>
class SpeedMidiParameter_T: public MidiParameter
{
public:

    SpeedMidiParameter_T(std::shared_ptr<RemoteInterface::AttributeT<DATA_TYPE> > sp):
        _attribute(sp),
        _speed(0)
    {}

    SpeedMidiParameter_T(std::weak_ptr<RemoteInterface::AttributeT<DATA_TYPE> > sp):
        _attribute(sp),
        _speed(0)
    {}

    virtual void turned(size_t val, float timestamps)
    {

        float timeFactor = timestamps*20;

        if(_speed == 0)
        {
            float speedSign = val > 64? 1 : -1;
            _speed = speedSign *_sensibility;
        }
        else
        {
            if(_speed>0)
            {
                _speed *= factorModulation((float)val/ 64.0f, timeFactor);
            }
            else
            {
                _speed *= factorModulation(( 128.0f - (float)val)/64.0f, timeFactor);
            }
        }
    }

    void stop()
    {
        _speed = 0;
    }

    virtual void button(bool p){
        stop();
    }

    virtual void applyUpdate()
    {
        if( _speed!=0 )
        {
            if(auto attr = _attribute.lock())
            {
                DATA_TYPE newState = attr->get() + (DATA_TYPE) _speed;
                attr->set(newState);
            }
            else
            {
                std::cout<<"Dangling weak ptr"<<std::endl;
            }
        }
    }


private:
    std::weak_ptr<RemoteInterface::AttributeT<DATA_TYPE> > _attribute;
    float _speed;
};


template <typename T>
MidiParameter* makeValueMidiParameter(std::shared_ptr<RemoteInterface::AttributeT<T> > sp)
{
    return new ValueMidiParameter_T<T>(sp);
}
template <typename T>
MidiParameter* makeValueMidiParameter(std::weak_ptr<RemoteInterface::AttributeT<T> > sp)
{
    return new ValueMidiParameter_T<T>(sp);
}


template <typename T>
MidiParameter* makeSpeedMidiParameter(std::weak_ptr<RemoteInterface::AttributeT<T> > sp)
{
    return new SpeedMidiParameter_T<T>(sp);
}
template <typename T>
MidiParameter* makeSpeedMidiParameter(std::shared_ptr<RemoteInterface::AttributeT<T> > sp)
{
    return new SpeedMidiParameter_T<T>(sp);
}

template <typename T>
MidiParameter* makeMidiAction(std::shared_ptr<RemoteInterface::ActionT<T> > sp)
{
    return new MidiAction(sp);
}



class MidiInput
{
public:
    MidiInput();
    ~MidiInput();


    static const size_t ArturiaBeatstep_KnobsMap[16];

    static const size_t ArturiaBeatstep_PadMap[16];

    static const size_t bigKnobId;

    bool connect();

    void setKnobAction(size_t knobIndex, MidiParameter* elem)
    {
        if(knobIndex>16 || knobIndex<0)
        {
            std::cout<<"There is no knob at the position "<<knobIndex<<"on arturia beatstep device"<<std::endl;
        }

        if(knobIndex == 0)
        {
            bigKnobAction.reset(elem);
        }
        else
        {
            knobActions[ArturiaBeatstep_KnobsMap[knobIndex-1]] =
                    std::unique_ptr<MidiParameter> (elem);
        }
    }

    void setSpeedKnobAction(size_t knobIndex, MidiParameter* elem)
    {
        if(knobIndex>16 || knobIndex<1)
        {
            std::cout<<"There is no knob at the position "<<knobIndex<<"on arturia beatstep device"<<std::endl;
        }

        auto knobMapIndex = ArturiaBeatstep_KnobsMap[knobIndex-1];
        knobActions[knobMapIndex] =
                std::unique_ptr<MidiParameter> (elem);
        auto padMapIndex = ArturiaBeatstep_PadMap[knobIndex-1];
        pad2Knob[padMapIndex] = knobMapIndex;
    }

    void setPadAction(size_t buttonIndex, MidiParameter* elem)
    {
        if(buttonIndex>16 || buttonIndex<1)
        {
            std::cout<<"There is no button at the position "<<buttonIndex<<"on arturia beatstep device"<<std::endl;
        }

        padActions[ArturiaBeatstep_PadMap[buttonIndex-1]] =
                std::unique_ptr<MidiParameter> (elem);
    }

    void receiveUpdate(unsigned char v0, unsigned char v1, unsigned char v2, float deltaTime)
    {
        size_t elementCode = (size_t)v1;
        size_t updateValue = (size_t)v2;
        bool pressStatus = (size_t)v0>128;


        auto it = knobActions.find(elementCode);
        if(it != knobActions.end())
        {
            //test if it is a knob
            it->second->turned(updateValue, deltaTime);
        }
        else if(elementCode == bigKnobId)
        {
            if(bigKnobAction)
            {
                bigKnobAction->turned(updateValue,deltaTime);
            }
        }
        else {
            auto it2 = pad2Knob.find(elementCode);
            if(it2 != pad2Knob.end())
            {
                // it is the pad, it has been pressed so we stop the associated knob
                size_t associatedKnob = it2->second;
                knobActions[associatedKnob]->button(pressStatus);
            }
            auto it3 = padActions.find(elementCode);
            if(it3 != padActions.end())
            {
                it3->second->button(pressStatus);
            }
        }
    }

    void receiveUpdates()
    {
        if(!rtmidi)
        {
            //std::cout<<"No rtmidi instance "<<std::endl;
            return;
        }
        std::vector<unsigned char> message;

        size_t nbrAllowedMessages = 100;
        size_t nBytes;

        double deltaTime;

        //cache updates
        while(nbrAllowedMessages>0)
        {
            deltaTime = rtmidi->getMessage( &message );
            nBytes = message.size();
            if(nBytes == 0)
                break; //there is no message left in the queue

            nbrAllowedMessages--;

            if(nBytes!=3)
            {
                continue; // we don't know how to deal with a message which doesn't have 3 bytes
            }

            receiveUpdate(message[0], message[1], message[2], deltaTime);

        }

    }

    void applyUpdate()
    {

        //apply updates
        for(auto it = knobActions.begin(); it!= knobActions.end(); ++it)
        {
            it->second->applyUpdate();
        }

        if(bigKnobAction)
        {
            bigKnobAction->applyUpdate();
        }
    }

    void clear()
    {
        knobActions.clear();
        padActions.clear();
        pad2Knob.clear();
        bigKnobAction.reset();
    }

private:
    typedef std::map<size_t,std::unique_ptr<MidiParameter> > AttributeMap;
    typedef std::map<size_t,size_t> Pad2KnobMap;
    AttributeMap knobActions;
    AttributeMap padActions;
    Pad2KnobMap pad2Knob;
    std::unique_ptr<MidiParameter> bigKnobAction;
    RtMidiIn* rtmidi;

};


}
