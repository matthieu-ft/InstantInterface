#include "Midi.h"

#include <string>
#include <regex>

#include <RtMidi.h>

using namespace std;

namespace RemoteInterface
{



const size_t MidiInput::ArturiaBeatstep_KnobsMap[16] =
{10, 74, 71, 76, 77, 93, 73, 75, 114, 18, 19, 16, 17, 91, 79, 72};

const size_t MidiInput::ArturiaBeatstep_PadMap[16] =
{44, 45, 46, 47, 48, 49, 50 ,51, 36, 37, 38, 39, 40, 41, 42, 43};

const size_t MidiInput::bigKnobId = 7;



MidiInput::MidiInput():
    rtmidi(nullptr)
{}

MidiInput::~MidiInput()
{
    delete rtmidi;
}

bool MidiInput::connect()
{
    rtmidi = new RtMidiIn();
    unsigned int nPorts = rtmidi->getPortCount();

    //look fo arturia beatstep
    int portIndex = -1;
    string portName;
    for(size_t i = 0; i<nPorts; i++)
    {
        portName = rtmidi->getPortName(i);

        if (regex_match (portName, std::regex("(.*)(Arturia BeatStep)(.*)", std::regex_constants::icase) ))
        {
            portIndex = i;
            break;
        }
    }

    if(portIndex<0)
    {
        std::cout<<"Arturia beatstep not found"<<std::endl;
        delete rtmidi;
        rtmidi = nullptr;
        return false;
    }

    rtmidi->openPort( portIndex );
    rtmidi->ignoreTypes(false,false,false);

    return true;
}

MidiParameter *MidiParameter::sensibility(float s)
{
    _sensibility = s;
    return this;
}

float MidiParameter::sensibility() const
{
    return _sensibility;
}

StateKnob* makeStateKnob(std::shared_ptr<RemoteInterface::StateParameterModifierManager> sm, int factor)
{
    return new StateKnob(sm, factor);
}

float factorModulation(float speedFactor, float timeFactor)
{
    float temp = speedFactor-1;
    float tempSign = temp >0? 1:-1;
    return 1+ tempSign*pow(tempSign*(speedFactor-1),timeFactor);
}

}
