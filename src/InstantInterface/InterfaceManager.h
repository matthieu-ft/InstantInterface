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

#include <string>
#include <memory>
#include <vector>

namespace Json{
class Value;
}

namespace InstantInterface {

/**
 * @brief The InterfaceManager class is used to create a structured interface for a set of Attributes/Actions. The Attributes and Actions are added with addInteractionElement()
 * and the interface is structured with createGroup().
 */
class InterfaceManager
{
public:

    /**
     * @brief constructor
     */
    InterfaceManager();
    /**
     * @brief copy constructor. It makes no deep copy of the intern implementation, so the copy of the InterfaceManager still manages the same interface.
     * @param a
     */
    InterfaceManager(const InterfaceManager& a);
    ~InterfaceManager();

    /**
     * @brief adds the attribute \p elem to the interface at the current level with \p name as label
     * specialized for int, float, double, bool, std::string
     * @param name label
     * @param elem attribute to be controlled by the interface
     * @param returns the current interface (same as *this)
     */
    template <typename ParamType>
    InterfaceManager &addInteractionElement(const std::string& name, std::shared_ptr<AttributeT<ParamType> > elem);


    /**
     * @brief adds the attribute \p elem to the interface at the current level with \p name as label
     * specialized for int, float, double, bool, std::string
     * @param elem attribute to be controlled by the interface, already containing the label
     * @param returns the current interface (same as *this)
     */
    template <typename ParamType>
    InterfaceManager &addInteractionElement(std::shared_ptr<AttributeT<ParamType> > elem){
        return addInteractionElement(elem->getName(), elem);
    }

    InterfaceManager &addInteractionElement_generic(AttributePtr elem){
        switch (elem->getTypeValue()) {
        case TYPE_BOOL:
            addInteractionElement(std::static_pointer_cast<AttributeT<bool> >(elem));
            break;
        case TYPE_INT:
            addInteractionElement(std::static_pointer_cast<AttributeT<int> >(elem));
            break;
        case TYPE_FLOAT:
            addInteractionElement(std::static_pointer_cast<AttributeT<float> >(elem));
            break;
        case TYPE_DOUBLE:
            addInteractionElement(std::static_pointer_cast<AttributeT<double> >(elem));
            break;
        case TYPE_STRING:
            addInteractionElement(std::static_pointer_cast<AttributeT<std::string> >(elem));
            break;
        default:
            break;
        }

        return *this;
    }

    InterfaceManager &addInteractionElements(const std::vector<AttributePtr>& attributes)
    {
        for(auto ptr: attributes)
        {
            addInteractionElement_generic(ptr);
        }
        return *this;
    }

    /**
     * @brief adds the action \p elem to the interface at the current level with \p name as label
     * @param name label
     * @param elem attribute to be controlled by the interface
     * @return
     */
    InterfaceManager &addInteractionElement(const std::string& name, std::shared_ptr<Action> elem);

    /**
     * @brief creates a subgroup in the current interface with the label \p name
     * @param name label of the group
     * @return interface to the subgroup ( != *this )
     */
    InterfaceManager createGroup(const std::string& name);

    /**
     * @brief returns the structure of the interface formated as a json string
     * @return
     */
    std::string getStructureJsonString() const;

    /**
     * @brief update the element (the attribute) associated with the name \p name and with the value described in the json object \p val
     * @param name id of the attribute
     * @param val Json::Value object containing the new value of the attribute
     */
    void updateInterfaceElement(const std::string& name, const Json::Value& val);

    /**
     * @brief get the current values of all the registered attributes as a json string
     * @return
     */
    std::string getStateJsonString() const;

    /**
     * @brief clears the content of the interface
     */
    void clear();


private:

    class InterfaceImpl;
    class InterfaceRootImpl;
    class InterfaceRefImpl;

    InterfaceManager(std::unique_ptr<InterfaceImpl> pImpl);

    std::unique_ptr<InterfaceImpl> impl;
};

}
