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

#include <InstantInterface/InterfaceManager.h>

#include <websocketpp/server.hpp>
#include <websocketpp/config/asio_no_tls.hpp>

#include <set>
#include <string>


namespace InstantInterface
{

/**
 * @brief The WebInterface class implement the InterfaceManager to the case of web interface. It makes the interface available on a web page on the local network.
 */
class WebInterface: public InterfaceManager {
public:


    typedef websocketpp::connection_hdl connection_hdl;
    typedef websocketpp::server<websocketpp::config::asio> server;
    typedef std::lock_guard<std::mutex> scoped_lock;

    WebInterface(bool withThread = false);

    /**
     * @brief closes all connections to clients and disconnects the server.
     */
    void stop();

    /**
     * @brief starts server on the specified port \p port that will delivers the content found at the path \p docroot
     * @param port server port
     * @param docroot path to the content to be delivered. If docroot == "#" then the path will be looked for in a file named pathToWebInterface.txt
     */
    void init(uint16_t port, std::string docroot = "#");

    /**
     * @brief retrieves messages sent to the server (alternative to run, when withThreaded=false)
     * @return returns true if the server is still running, false if it has been stopped
     */
    bool poll();

    /**
     * @brief runs the server. The server is started in a thread if \p withThread has been set to \p true in the constructor.
     * Otherwise, calling run() is blocking.
     */
    void run();

    /**
     * @brief reads the messages received from the clients and execute the associated commands.
     */
    void executeCommands();

    /**
     * @brief updates the cache of the structure of the interface. Required only in threaded mode.
     */
    void updateStructureCache();

    /**
     * @brief updates the cache of the values of all the registered parameters. Required only in threaded mode.
     */
    void updateParameterCache();

    /**
     * @brief refreshes the parameter values for the connected clients.
     */
    void forceRefreshAll();

    /**
     * @brief refreshes the interface for the connected clients.
     */
    void forceRefreshStructureAll();

protected:

    /**
     * @brief executes the command contained in the string \p command
     * @param command to be executed
     * @return true if the command could be identified
     */
    virtual bool executeSingleCommand(const std::string& command);

private:

    void on_message(websocketpp::connection_hdl hdl, server::message_ptr msg);

    void send_interface(websocketpp::connection_hdl hdl );

    void on_http(connection_hdl hdl);

    void on_open(connection_hdl hdl);

    void on_close(connection_hdl hdl);

    void send_values_update(websocketpp::connection_hdl hdl);


    typedef std::set<connection_hdl,std::owner_less<connection_hdl>> con_list;

    void addCommand(std::string const& command);


    server m_endpoint;
    con_list m_connections;
    server::timer_ptr m_timer;

    std::string m_docroot;

    // Telemetry data
    uint64_t m_count;

    std::queue<std::string> mainPrgMessages;
    std::mutex mainPrgMessagesMutex;

    std::mutex parametersMutex;
    bool m_stopped;

    std::thread thread;
    bool threaded;


    std::string structureCache;
    std::string valuesCache;
};

}
