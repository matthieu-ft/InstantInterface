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

#include <InstantInterface/WebInterface.h>

#include <fstream>
#include <iostream>
#include <set>
#include <streambuf>
#include <string>

using namespace std;
using namespace InstantInterface;

/** @file
  * @brief this file describes a basic use of WebInterface for monitoring attributes.
  *
  */

//basic class for demonstrating how to create an attribute based on the getter and setter of an object
class Basic{
public:
    Basic() : a(0) {}
    float get() const {return a;}
    void set(float aa)  {a = aa;}
private:
    float a;
};

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

    //define the parameter that we want to make manageable through the interface
    int     i   = 0;
    float   j   = 1;
    float   k   = 0.3;
    bool    v   = false;
    bool    running = true;
    Basic obj;

    //create the attribute object that encapsulate these attributes
    auto param1 = InstantInterface::AttributeFactory::makeAttribute(&i)->setMin(-1)->setMax(5);
    auto param2 = InstantInterface::AttributeFactory::makeAttribute(&j)->setMin(-5)->setMax(3);
    auto param3 = InstantInterface::AttributeFactory::makeAttribute(&k)->setMin(0)->setMax(50)->periodic(true);
    auto param4 = InstantInterface::AttributeFactory::makeAttribute(&obj, &Basic::get, &Basic::set)->setMin(-1)->setMax(10);
    auto param5 = InstantInterface::AttributeFactory::makeAttribute(&v);

    //register the attributes in the interface in the group "Group of attributes"
    s.createGroup("Group of attributes")
            .addInteractionElement("1 - int attribute",param1)
            .addInteractionElement("2 - float attribute", param2)
            .addInteractionElement("3 - periodic float attribute", param3)
            .addInteractionElement("4 - float attribute from getter/setter", param4)
            .addInteractionElement("5 - bool attribute", param5);

    //create actions (that are activated by pressing a button)
    //the creation of actions is based on lambda functions
    auto toggle = InstantInterface::AttributeFactory::makeAction([&v](){v=!v;});
    auto reset = InstantInterface::AttributeFactory::makeAction([&]()
                {
                    i = 0;
                    j = 1;
                    k = 0.3;
                    v = false;
                    obj.set(0);
                });
    auto stop = InstantInterface::AttributeFactory::makeAction([&running]() {running = false;});


    s.createGroup("Group of actions")
            .addInteractionElement("toggle (attribute 5)", toggle)
            .addInteractionElement("reset all", reset)
            .addInteractionElement("STOP", stop);


    //about group hierarchy
    auto baseGroup = s.createGroup("Parent");
    auto sg1 = baseGroup.createGroup("Child 1");
    auto sg2 = baseGroup.createGroup("Child 2");
    auto ss1 = sg1.createGroup("Grandchild 1");
    auto ss2 = sg1.createGroup("Grandchild 2");

    //port on which the interface will be made available
    // for example, with port = 9000, you will be able to access the interface
    // on the browser on the same computer at the adress:   localhost:9000
    // The interface will be also accessible from any browser connected to your local network
    // at the adress:   YOUR_IP:9000    where YOUR_IP should be replaced by the IP adress of the computer
    // where the program is running.
    uint16_t port = 9000;

    if (argc == 2) {
        int i = atoi(argv[1]);
        if (i <= 0 || i > 65535) {
            std::cout << "invalid port" << std::endl;
            return 1;
        }

        port = uint16_t(i);
    }

    //start server
    s.init(port);

    // starts the server (it is a blocking call if the withThread == false)
    s.run();

    while(running)
    {
        //execute the modifications coming from the clients
        s.executeCommands();
        //update the cache of the parameters (it is required as the mode is threaded)
        s.updateParameterCache();
        //send the new values of the parameters to the clients (so that the modifications made by one
        // are visible by the others)
        s.forceRefreshAll();
        //limit the the frequency of the updates, because otherwise the browsers are overloaded
        std::this_thread::sleep_for(std::chrono::milliseconds(300));

        //There you can write your own code
    }

    //stop the server
    s.stop();

  return 0;
}
