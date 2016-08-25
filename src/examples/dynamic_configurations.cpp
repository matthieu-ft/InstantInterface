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

#include <InstantInterface/AttributeManagement.h>
#include <InstantInterface/WebInterface.h>

#include <fstream>
#include <iostream>
#include <set>
#include <streambuf>
#include <string>
#include <list>

using namespace InstantInterface;

/**
 * @file
 * @brief we demonstrate in this file how to use the modifiers and dynamic configurations, that enable to make smooth configurations between parameter values
 */
int main(int argc, char* argv[])
{
    bool withThread = true;

    // if withThread is true, the interface will use the cached value of the attributes
    // so that updateCache() has to be called after every modification of the attributes by the cpp program,
    // so that the remote interface can take that into account
    // for executing the modification that come from the interface, executeCommands() will have to be called
    // the server will have to be started with WebInterface::start()

    // if withthread is false, you can start the server with WebInterface::start() (but it is a blocking call, so you won't be
    // able to do anything until the WebInterface stops) or you can call WebInterface::poll() to let the server process the messages that
    // have arrived

    WebInterface s (withThread);

    //two float parameters that we will be manipulating through the interface
    float   i   = 0;
    float   j   = 1;

    //make a periodic attribute (an attribute is not periodic per default) for which extrema are not enforced (they are enforced per default)
    //in this case, this means that the value 101 is allowed but it is equivalent to the value 1, because the period is max-min.
    auto param1 = InstantInterface::AttributeFactory::makeAttribute(&i)->setMin(0)->setMax(100)->periodic(true)->enforceExtrema(false);
    //make a normal attribute
    auto param2 = InstantInterface::AttributeFactory::makeAttribute(&j)->setMin(0)->setMax(500);



    //confManager will  manage/apply the transitions
    DynamicConfiguration confManager;




    /*
     *  SMOOTH TRANSITIONS
     */

    //create a configuration where the param1 has value 50 and param2 value 100
    DynamicConfiguration::ModifierVec conf1 = {
                  makeValueModifier(param1,50),
                  makeValueModifier(param2,100)
              };

    //create a configuration where the param1 has value 1 and param2 value 400
    DynamicConfiguration::ModifierVec conf2 = {
                  makeValueModifier(param1,1),
                  makeValueModifier(param2,400)
              };

    //create a configuration where the param2 has value 13 while no requirement is set on param1
    DynamicConfiguration::ModifierVec conf3 = {
                  makeValueModifier(param2,13)
              };

    //create the transition associated to each conguration
    //we decide that all the transition will last 3000ms (you can choose the time unit you
    //want, as long as it is coherent with the elapsed timed that you give in apply() )
    //for instance, when switchConfig1 is called, param1 and param2 will go smoothly from
    //their current state to the respective values 50 and 100 in a lapse of time of 3000ms.
    auto switchConfig1 = confManager.makeTransitionLambda(conf1,3000);
    auto switchConfig2 = confManager.makeTransitionLambda(conf2,3000);
    auto switchConfig3 = confManager.makeTransitionLambda(conf3,3000);


    /*
     * IMPULSES
     */

    //create impulses for param1 and param2. An impulse is a temporary smooth transition
    //to a given value (here 33) and with smooth coming back to the previous value. The hole back and forth
    //transition lasts the given amount of time (here 1000ms)
    auto impulse1 = makeImpulse(param1,33,1000);
    auto impulse2 = makeImpulse(param2,33,1000);

    //create the transition lambdas associated to each impulse
    auto activImpulse1 = confManager.makeTransitionLambda(impulse1);
    auto activImpulse2 = confManager.makeTransitionLambda(impulse2);

    /*
     * STATE BASED MODIFIERS
     */

    //we create a state manager for the param1. We define the states in the order {0, 25, 50, 75}. This means that param1 will have 4 states.
    //a state manager enables to create smooth transition between each state
    auto stateModifierManager = makeStateParameterManager(param1, std::vector<float>({0, 25, 50, 75}));

    //create action for a smooth sate incrementation. For instance, if value = 50 and incrState is called, then a smooth transition will be executed from 50 to 75.
    //the transition will last 3000ms
    auto incrState = stateModifierManager->makeDeltaTransition(confManager,1,3000);
    //create action for a smooth sate incrementation. For instance, if value = 50 and incrState is called, then a smooth transition will be executed from 50 to 25.
    //the transition will last 3000ms
    auto decrState = stateModifierManager->makeDeltaTransition(confManager,-1,3000);


    //create the actions associated to the lambdas functions that we have created
    auto action1 = InstantInterface::AttributeFactory::makeAction(switchConfig1);
    auto action2 = InstantInterface::AttributeFactory::makeAction(switchConfig2);
    auto action3 = InstantInterface::AttributeFactory::makeAction(switchConfig3);
    auto incrStateAction = InstantInterface::AttributeFactory::makeAction(incrState);
    auto decrStateAction = InstantInterface::AttributeFactory::makeAction(decrState);
    auto impulse1Action = InstantInterface::AttributeFactory::makeAction(activImpulse1);
    auto impulse2Action = InstantInterface::AttributeFactory::makeAction(activImpulse2);

    s.createGroup("Parameters")
            .addInteractionElement("MyParam1",param1)
            .addInteractionElement("MyParam2", param2)
            ;
    s.createGroup("Configurations")
            .addInteractionElement("Configuration1", action1)
            .addInteractionElement("Configuration2", action2)
            .addInteractionElement("Configuration3", action3)
            ;
    s.createGroup("Impulses")
            .addInteractionElement("Impulse 1", impulse1Action)
            .addInteractionElement("Impulse 2", impulse2Action)
            ;
    s.createGroup("State management")
            .addInteractionElement("Increment param1", incrStateAction)
            .addInteractionElement("Decrement param1", decrStateAction)
            ;

    uint16_t port = 9000;

    if (argc == 2) {
        int i = atoi(argv[1]);
        if (i <= 0 || i > 65535) {
            std::cout << "invalid port" << std::endl;
            return 1;
        }

        port = uint16_t(i);
    }

    s.init(port);

    auto lastApply = std::chrono::system_clock::now();
    auto currentApply  = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds;

    s.run();

    while(true)
    {
        //measure elapsed time since last apply
        currentApply = std::chrono::system_clock::now();
        elapsed_seconds = currentApply - lastApply;
        lastApply = currentApply;

        //apply further the transitions that have already started and start the transition that have
        //just been registered (when a lambda function is called)
        //give the elapsed time in ms since the last call
        confManager.apply(1000*elapsed_seconds.count());

        s.executeCommands();
        s.updateParameterCache();
        s.forceRefreshAll();

        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }

    s.stop();

    return 0;

}
