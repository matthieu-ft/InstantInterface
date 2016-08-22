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

#include "WebInterface.h"
#include <json/json.h>

#include <fstream>

using namespace std;

namespace InstantInterface
{

WebInterface::WebInterface(bool withThread) :
    m_count(0),
    threaded(withThread),
    structureCache(""),
    valuesCache(""),
    m_stopped(false)
{
    // set up access channels to only log interesting things
    m_endpoint.clear_access_channels(websocketpp::log::alevel::all);
    m_endpoint.set_access_channels(websocketpp::log::alevel::access_core);
    m_endpoint.set_access_channels(websocketpp::log::alevel::app);

    m_endpoint.set_reuse_addr(true);

    // Initialize the Asio transport policy
    m_endpoint.init_asio();

    // Bind the handlers we are using
    using websocketpp::lib::placeholders::_1;
    using websocketpp::lib::placeholders::_2;
    using websocketpp::lib::bind;
    m_endpoint.set_open_handler(bind(&WebInterface::on_open,this,_1));
    m_endpoint.set_close_handler(bind(&WebInterface::on_close,this,_1));
    m_endpoint.set_http_handler(bind(&WebInterface::on_http,this,_1));
    m_endpoint.set_message_handler(bind(&WebInterface::on_message,this,_1,_2));

}


bool WebInterface::poll()
{
    try {
        m_endpoint.poll();
    } catch (websocketpp::exception const & e) {
        std::cout << e.what() << std::endl;
    }

    return !m_stopped;
}

void WebInterface::stop()
{
    //do not accept new connection
    m_endpoint.stop_listening();

    //close properly all existing connections
    for(auto& it: m_connections)
    {
        m_endpoint.close(it,websocketpp::close::status::normal, "close button pressed");
    }

    m_stopped = true;

    thread.join();
}

void WebInterface::init(uint16_t port, std::string docroot) {
    std::stringstream ss;

    if(docroot == "#")
    {
        //in this case we look for the path in the file pathToWebInterface.txt

        std::ifstream ifstr ("pathToWebInterface.txt");
        if(!std::getline(ifstr, docroot))
        {
            std::cout<<"InstantInterface::WebInterface::init(), couldn't find the path to the web interface. "
                       "Make sure that the file pathToWebInterface.txt exists in the same directory where the program is executed"
                       " and make sure that there are no additional space or lines in the file."
                       " Also, don't forget to add a directory separator at the end of the file"<<std::endl;
            return;
        }
    }

    ss << "Running telemetry server on port "<< port <<" using docroot=" << docroot;
    m_endpoint.get_alog().write(websocketpp::log::alevel::app,ss.str());

    m_docroot = docroot;

    // listen on specified port
    m_endpoint.listen(port);

    // Start the server accept loop
    m_endpoint.start_accept();


}

void WebInterface::run()
{
    updateStructureCache();
    updateParameterCache();

    // Start the ASIO io_service run loop
    try {
        if(threaded)
        {
            thread = std::thread(&server::run,&m_endpoint);
        }
        else
        {
            m_endpoint.run();
        }
    } catch (websocketpp::exception const & e) {
        std::cout << e.what() << std::endl;
    }
}

void WebInterface::on_message(websocketpp::connection_hdl hdl, server::message_ptr msg) {


    if (msg->get_opcode() == websocketpp::frame::opcode::text) {
        std::string content = msg->get_payload();
        if(content == "send_interface")
        {
            send_interface(hdl);
        }
        else if (content == "update")
        {
            send_values_update(hdl);
        }
        else
        {
            //this must contain json, so we try to do the udpate

            if(threaded)
            {
                addCommand(content);
            }
            else
            {
                executeSingleCommand(content);
            }

        }
    }
    else
    {
        std::cout<<"we don't know what to do with this message, because this is no text"<<std::endl;
    }
}

void WebInterface::send_interface(websocketpp::connection_hdl hdl)
{
    if (threaded)
    {
        m_endpoint.send(hdl,structureCache,websocketpp::frame::opcode::text);
    }
    else
    {
        scoped_lock lock(parametersMutex);
        m_endpoint.send(hdl,getStructureJsonString(),websocketpp::frame::opcode::text);
    }
    //after sending the interface we send the update of all the parameters, because the structure of the interface
    //is stored in json::value that is not synchronized with the actual values of the parameters
    send_values_update(hdl);
}

void WebInterface::send_values_update(websocketpp::connection_hdl hdl)
{
    if(threaded)
    {
        scoped_lock lock(parametersMutex);
        m_endpoint.send(hdl,valuesCache,websocketpp::frame::opcode::text);
    }
    else
    {
        m_endpoint.send(hdl,getStateJsonString(),websocketpp::frame::opcode::text);
    }
}

void WebInterface::addCommand(const std::string &command)
{
    scoped_lock lock (mainPrgMessagesMutex);
    mainPrgMessages.push(command);
}

void WebInterface::executeCommands()
{

    std::queue<std::string> queueCopy;

    {
        scoped_lock lock (mainPrgMessagesMutex);
        queueCopy = std::move(mainPrgMessages);
    }


    std::string content;

    while(!queueCopy.empty())
    {
        content = queueCopy.front();
        queueCopy.pop();

        executeSingleCommand(content);
    }

}

bool WebInterface::executeSingleCommand(const string &content)
{
    Json::Reader reader;
    Json::Value messageJson;
    bool success = reader.parse(content.c_str(), messageJson);
    if(!success)
    {
        std::cout<<"Couldn't parse received message to json."<<std::endl;
        return false;
    }

    if(messageJson["type"].asString()=="update")
    {
        Json::Value updates = messageJson["content"];
        for(Json::ValueIterator itr = updates.begin(); itr != updates.end(); itr++)
        {
            std::string paramId = (*itr)["id"].asString();
            updateInterfaceElement(paramId,(*itr)["value"]);
        }
        return true;
    }

    return false;
}

void WebInterface::updateStructureCache()
{
    scoped_lock lock (parametersMutex);
    structureCache = this->getStructureJsonString();
}

void WebInterface::updateParameterCache()
{
    scoped_lock lock (parametersMutex);
    valuesCache = this->getStateJsonString();
}

void WebInterface::forceRefreshAll()
{
    updateParameterCache();

    for(auto& it: m_connections)
    {
        send_values_update(it);
    }
}

void WebInterface::forceRefreshStructureAll()
{
    updateStructureCache();

    for(auto& it: m_connections)
    {
        send_interface(it);
    }
}


void WebInterface::on_http(WebInterface::connection_hdl hdl) {
    // Upgrade our connection handle to a full connection_ptr
    server::connection_ptr con = m_endpoint.get_con_from_hdl(hdl);

    std::ifstream file;
    std::string filename = con->get_uri()->get_resource();
    std::string response;

    m_endpoint.get_alog().write(websocketpp::log::alevel::app,
                                "http request1: "+filename);


    if (filename == "/") {
        filename = m_docroot+"index.html";
    } else {
        filename = m_docroot+filename.substr(1);
    }

    m_endpoint.get_alog().write(websocketpp::log::alevel::app,
                                "http request2: "+filename);

    file.open(filename.c_str(), std::ios::in);
    if (!file) {
        // 404 error
        std::stringstream ss;

        ss << "<!doctype html><html><head>"
           << "<title>Error 404 (Resource not found)</title><body>"
           << "<h1>Error 404</h1>"
           << "<p>The requested URL " << filename << " was not found on this server.</p>"
           << "</body></head></html>";

        con->set_body(ss.str());
        con->set_status(websocketpp::http::status_code::not_found);
        return;
    }

    file.seekg(0, std::ios::end);
    response.reserve(file.tellg());
    file.seekg(0, std::ios::beg);

    response.assign((std::istreambuf_iterator<char>(file)),
                    std::istreambuf_iterator<char>());

    con->set_body(response);
    con->set_status(websocketpp::http::status_code::ok);
}

void WebInterface::on_open(WebInterface::connection_hdl hdl) {
    m_connections.insert(hdl);
}

void WebInterface::on_close(WebInterface::connection_hdl hdl) {
    m_connections.erase(hdl);
}



}
