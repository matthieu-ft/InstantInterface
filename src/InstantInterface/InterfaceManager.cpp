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

#include "InterfaceManager.h"
#include <json/json.h>
#include <vector>
#include <map>
#include <iostream>

using namespace std;

namespace InstantInterface {


/***
   ___       _             __                  _
  |_ _|_ __ | |_ ___ _ __ / _| __ _  ___ ___  | |_ _ __ ___  ___
   | || '_ \| __/ _ \ '__| |_ / _` |/ __/ _ \ | __| '__/ _ \/ _ \
   | || | | | ||  __/ |  |  _| (_| | (_|  __/ | |_| | |  __/  __/
  |___|_| |_|\__\___|_|  |_|  \__,_|\___\___|  \__|_|  \___|\___|
 *
 */

/**
 * @brief abstract class defining a node of the interface tree
 */
class JsonNode{
public:
    virtual Json::Value getJsonStructure() = 0;
};

/**
 * @brief interface node containing a list of interface elements
 */
class JsonGroupBase : public JsonNode
{
public:
    void add(std::shared_ptr<JsonNode> part);

protected:
    std::vector<std::shared_ptr<JsonNode> > tree;
};

/**
 * @brief root node of the interface
 */
class JsonTreeRoot : public JsonGroupBase
{
public:
    void clear();
    Json::Value getJsonStructure();
};

/**
 * @brief named group node
 */
class JsonGroup: public JsonGroupBase
{
public:
    JsonGroup(std::string nn);
    std::string getName();
    Json::Value getJsonStructure();

private:
    std::string name;
};

/**
 * @brief Leaf of the interface tree: it contains an Attribute or an Action
 */
class JsonElement: public JsonNode
{
public:
    JsonElement() : name("empty_name"), id("empty_id"){}
    virtual std::string getValueType() = 0;
    virtual void setFromJson(Json::Value val) = 0;
    virtual std::string getValueAsString() = 0;
    virtual Json::Value getJsonValue() = 0;
    virtual Json::Value getJsonStructure() = 0;

    const std::string& getName() const;
    void setName(const std::string& n);

    const std::string& getId() const;
    void setId(const std::string& i);

private:
    std::string name;
    std::string id;
};

/**
 * @brief Leaf of the interface tree containing an action
 */
class JsonAction : public JsonElement
{
public:
    JsonAction( std::shared_ptr<Action> a);

    std::string getValueType();

    void setFromJson(Json::Value val);

    std::string getValueAsString();

    Json::Value getJsonValue();

    Json::Value getJsonStructure();

    virtual void applyAction();

private:
    std::shared_ptr<Action> action;
};


/**
 * @brief Leaf of the interface tree containing an attribute
 * AttributeT has been specialized for types: float, int, bool and string
 */
template <class ParamType>
class JsonAttributeT: public JsonElement
{
public:
    JsonAttributeT(std::weak_ptr<AttributeT<ParamType> > wp);
    void setFromJson (Json::Value val);
    virtual void set(ParamType v);
    virtual ParamType get();
    virtual std::string getValueAsString();
    Json::Value getJsonValue();
    Json::Value getJsonStructure();
    std::string getValueType();
    bool getMinMax(ParamType& minVal, ParamType& maxVal);

private:
    std::weak_ptr<AttributeT<ParamType> > _attr;
};


namespace factory{
    /**
     * @brief function for creating a JsonAttributeT<T> object that encapsulates the given AttributeT<T> instance
     * @param wp
     * @return shared_ptr to an encapsulting JsonAttributeT instance
     */
    template <class ParamType>
    std::shared_ptr<JsonElement> makeJson(std::shared_ptr<AttributeT<ParamType> > wp)
    {
        return std::make_shared<JsonAttributeT<ParamType> > (wp);
    }
    /**
     * @brief function for creating a JsonAction obect that encapsulates the given Action instance
     * @param wp
     * @return shared_ptr to an encapsulating Action instance
     */
    std::shared_ptr<JsonAction> makeJson(std::shared_ptr<Action> wp);
}


typedef std::map<std::string,std::shared_ptr<JsonElement> > JsonElementMap;


/***
 *
  _   _ _     _     _              _                 _                                 _        _   _
 | | | (_) __| | __| | ___ _ __   (_)_ __ ___  _ __ | | ___ _ __ ___  _ __   ___ _ __ | |_ __ _| |_(_) ___  _ __
 | |_| | |/ _` |/ _` |/ _ \ '_ \  | | '_ ` _ \| '_ \| |/ _ \ '_ ` _ \| '_ \ / _ \ '_ \| __/ _` | __| |/ _ \| '_ \
 |  _  | | (_| | (_| |  __/ | | | | | | | | | | |_) | |  __/ | | | | | | | |  __/ | | | || (_| | |_| | (_) | | | |
 |_| |_|_|\__,_|\__,_|\___|_| |_| |_|_|_|_| |_| .__/|_|\___|_| |_| |_|_| |_|\___|_| |_|\__\__,_|\__|_|\___/|_| |_|
   ___  / _| |_ _|_ __ | |_ ___ _ __ / _| __ _|_|__ ___|  \/  | __ _ _ __   __ _  __ _  ___ _ __
  / _ \| |_   | || '_ \| __/ _ \ '__| |_ / _` |/ __/ _ \ |\/| |/ _` | '_ \ / _` |/ _` |/ _ \ '__|
 | (_) |  _|  | || | | | ||  __/ |  |  _| (_| | (_|  __/ |  | | (_| | | | | (_| | (_| |  __/ |
  \___/|_|   |___|_| |_|\__\___|_|  |_|  \__,_|\___\___|_|  |_|\__,_|_| |_|\__,_|\__, |\___|_|
 *
 */



/**
 * @brief Hidden implementation of InterfaceManager (PIMPL idiom)
 */
class InterfaceManager::InterfaceImpl
{
public:
    InterfaceImpl(){}
    ~InterfaceImpl(){}
    Json::Value getJsonStructure();
    void addInteractionElement(const std::string &name, std::shared_ptr<JsonElement> ie);
    virtual std::shared_ptr<JsonGroupBase> getTree() = 0;
    virtual JsonElementMap& getMap() = 0;
    virtual void clear() = 0;
private:

};


/**
 * @brief Hidden implementation of InterfaceManager corresponding to the root level of the interface
 */
class InterfaceManager::InterfaceRootImpl: public InterfaceManager::InterfaceImpl
{
public:

    InterfaceRootImpl();
    ~InterfaceRootImpl(){}
    JsonElementMap& getMap();
    std::shared_ptr<JsonGroupBase> getTree();
    void clear();

private:
    std::shared_ptr<JsonTreeRoot> structure;
    JsonElementMap attributes;
};

/**
 * @brief Hidden implementation of InterfaceManager corresponding to a copy of the root level of the interface or to a subgroup of the interface
 */
class InterfaceManager::InterfaceRefImpl: public InterfaceManager::InterfaceImpl
{
public:

    InterfaceRefImpl(std::shared_ptr<JsonGroupBase> s, JsonElementMap& m);
    ~InterfaceRefImpl(){}
    JsonElementMap& getMap();
    std::shared_ptr<JsonGroupBase> getTree();
    void clear();

private:
    std::shared_ptr<JsonGroupBase> structure;
    JsonElementMap& attributes;
};




/***
 *
  ____        __ _       _ _   _
 |  _ \  ___ / _(_)_ __ (_) |_(_) ___  _ __  ___
 | | | |/ _ \ |_| | '_ \| | __| |/ _ \| '_ \/ __|
 | |_| |  __/  _| | | | | | |_| | (_) | | | \__ \
 |____/ \___|_| |_|_| |_|_|\__|_|\___/|_| |_|___/

 *
 */



InterfaceManager::InterfaceManager() : impl(new InterfaceRootImpl())
{}

InterfaceManager::InterfaceManager(const InterfaceManager &a):
    impl(new InterfaceRefImpl(a.impl->getTree(), a.impl->getMap()))
{}

InterfaceManager::~InterfaceManager()
{

}

InterfaceManager &InterfaceManager::addInteractionElement(const string &name, std::shared_ptr<Action> elem)
{
    impl->addInteractionElement(name, factory::makeJson(elem));
    return *this;
}



template <>
InterfaceManager &InterfaceManager::addInteractionElement<float>(const std::string& name, std::shared_ptr<AttributeT<float> > elem){
    impl->addInteractionElement(name, factory::makeJson(elem));
    return *this;
}
template <>
InterfaceManager &InterfaceManager::addInteractionElement<double>(const std::string& name, std::shared_ptr<AttributeT<double> > elem){
    impl->addInteractionElement(name, factory::makeJson(elem));
    return *this;
}
template <>
InterfaceManager &InterfaceManager::addInteractionElement<int>(const std::string& name, std::shared_ptr<AttributeT<int> > elem){
    impl->addInteractionElement(name, factory::makeJson(elem));
    return *this;
}
template <>
InterfaceManager &InterfaceManager::addInteractionElement<std::string>(const std::string& name, std::shared_ptr<AttributeT<std::string> > elem){
    impl->addInteractionElement(name, factory::makeJson(elem));
    return *this;
}
template <>
InterfaceManager &InterfaceManager::addInteractionElement<bool>(const std::string& name, std::shared_ptr<AttributeT<bool> > elem){
    impl->addInteractionElement(name, factory::makeJson(elem));
    return *this;
}


InterfaceManager InterfaceManager::createGroup(const std::string &name)
{
    auto group = std::make_shared<JsonGroup>(name);
    impl->getTree()->add(group);
    auto pImpl = std::unique_ptr<InterfaceImpl>(new InterfaceRefImpl(group,impl->getMap()));
    return InterfaceManager(std::move(pImpl));
}


std::string InterfaceManager::getStructureJsonString() const
{
    Json::Value interfaceMessage;
    interfaceMessage["type"] = "interface";
    interfaceMessage["content"] = impl->getJsonStructure();
    return interfaceMessage.toStyledString();
}

void InterfaceManager::updateInterfaceElement(const std::string &name, const Json::Value &val)
{
    auto elem = impl->getMap().find(name);
    if(elem == impl->getMap().end())
    {
        std::cout<<"There is no attribute named "<<name<<" in the attribute map."<<std::endl;
    }
    else
    {
        elem->second->setFromJson(val);
    }
}

std::string InterfaceManager::getStateJsonString() const
{
    Json::Value state;
    state["type"] = "update";
    Json::Value content;
    for(auto it = impl->getMap().begin(); it!=impl->getMap().end(); it++)
    {
        content.append(it->second->getJsonStructure());
    }
    state["content"] = content;

    return state.toStyledString();
}

void InterfaceManager::clear()
{
    impl->clear();
}

InterfaceManager::InterfaceManager(std::unique_ptr<InterfaceManager::InterfaceImpl> pImpl):
    impl(std::move(pImpl))
{}


JsonAction::JsonAction(std::shared_ptr<Action> a):
    action(a)
{}

std::string JsonAction::getValueType()
{
    return "a";
}

void JsonAction::setFromJson(Json::Value val)
{
    applyAction();
}

std::string JsonAction::getValueAsString()
{
    return "no value";
}

Json::Value JsonAction::getJsonValue()
{
    Json::Value state;
    state["id"] = getId();
    state["value"] = Json::Value();
    return state.toStyledString();
}

Json::Value JsonAction::getJsonStructure()
{
    Json::Value def;

    def["type"] = "parameter";
    def["name"] = getName();
    def["id"] = getId();
    def["valueType"] = getValueType();

    return def;
}

void JsonAction::applyAction()
{
    action->applyAction();
}

template<class ParamType>
JsonAttributeT<ParamType>::JsonAttributeT(std::weak_ptr<AttributeT<ParamType> > wp):
    _attr(wp)
{}



template<class T>
Json::Value JsonAttributeT<T>::getJsonStructure()
{
    Json::Value paramJson;

    paramJson["type"] = "parameter";
    paramJson["name"] = getName();
    paramJson["id"] = getId();
    paramJson["value"] = get();
    paramJson["valueType"] = getValueType();

    T  minVal, maxVal;

    if(getMinMax(minVal,maxVal))
    {
        paramJson["min"] = minVal;
        paramJson["max"] = maxVal;
    }

    return paramJson;
}

template <class T>
Json::Value JsonAttributeT<T>::getJsonValue()
{
    Json::Value state;
    state["id"] = getId();
    state["value"] = get();

    return state;
}


void msgDanglingWeakPtr()
{
    std::cout<<"Dangling weak ptr"<<std::endl;
}

template <class ParamType>
void JsonAttributeT<ParamType>::set(ParamType v)
{
    if(auto attr = _attr.lock())
    {
        attr->set(v);
    }
    else
    {
        msgDanglingWeakPtr();
    }
}

template <class ParamType>
ParamType JsonAttributeT<ParamType>::get()
{
    ParamType output;
    if(auto attr = _attr.lock())
    {
        output = attr->get();
    }
    else
    {
        msgDanglingWeakPtr();
    }
    return output;
}

template <class ParamType>
std::string JsonAttributeT<ParamType>::getValueAsString()
{
    std::stringstream ss;
    ss<<get();
    return ss.str();
}



template <class ParamType>
bool JsonAttributeT<ParamType>::getMinMax(ParamType& minVal, ParamType& maxVal)
{
    bool output = false;
    if(auto attr = _attr.lock())
    {
        output = attr->hasMin() && attr->hasMax();
        if(output)
        {
            minVal = attr->getMin();
            maxVal = attr->getMax();
        }
    }
    else
    {
        msgDanglingWeakPtr();
    }
    return output;
}


template <>
inline void JsonAttributeT<float>::setFromJson(Json::Value val) {set(val.asFloat());}
template <>
inline void JsonAttributeT<double>::setFromJson(Json::Value val) {set(val.asDouble());}
template <>
inline void JsonAttributeT<int>::setFromJson(Json::Value val) {set(val.asInt());}
template <>
inline void JsonAttributeT<std::string>::setFromJson(Json::Value val) {set(val.asString());}
template <>
inline void JsonAttributeT<bool>::setFromJson(Json::Value val) {set(val.asBool());}


template <>
inline std::string JsonAttributeT<float>::getValueType() {return "f";}
template <>
inline std::string JsonAttributeT<double>::getValueType() {return "d";}
template <>
inline std::string JsonAttributeT<int>::getValueType() {return "i";}
template <>
inline std::string JsonAttributeT<std::string>::getValueType() {return "s";}
template <>
inline std::string JsonAttributeT<bool>::getValueType() {return "b";}

template class JsonAttributeT<float>;
template class JsonAttributeT<double>;
template class JsonAttributeT<int>;
template class JsonAttributeT<std::string>;
template class JsonAttributeT<bool>;

void JsonGroupBase::add(std::shared_ptr<JsonNode> part)
{
    tree.push_back(part);
}

void JsonTreeRoot::clear()
{
    tree.clear();
}

Json::Value JsonTreeRoot::getJsonStructure()
{
    Json::Value  rootJson;
    for(auto item: tree)
    {
        rootJson.append(item->getJsonStructure());
    }
    return rootJson;
}

JsonGroup::JsonGroup(string nn): name(nn)
{}

string JsonGroup::getName()
{
    return name;
}

Json::Value JsonGroup::getJsonStructure()
{
    Json::Value groupJson;
    groupJson["type"] = "group";
    groupJson["name"] = name;
    Json::Value& subStructure = groupJson["content"];
    for(auto item: tree)
    {
        subStructure.append(item->getJsonStructure());
    }
    return groupJson;
}

std::shared_ptr<JsonAction> factory::makeJson(std::shared_ptr<Action> wp)
{
    return std::make_shared<JsonAction>(wp);
}

const string &JsonElement::getName() const { return name;}

void JsonElement::setName(const string &n)  { name = n;}

const string &JsonElement::getId() const { return id;}

void JsonElement::setId(const string &i)  { id = i;}




Json::Value InterfaceManager::InterfaceImpl::getJsonStructure()
{
    return getTree()->getJsonStructure();
}

void InterfaceManager::InterfaceImpl::addInteractionElement(const string &name, std::shared_ptr<JsonElement> ie)
{
    std::string id = name;

    while(getMap().find(id) != getMap().end())
    {
        std::cout<<"Parameter with id "<<name<<" already exists"<<std::endl;
        std::stringstream ss;
        ss<<name<<rand();
        id = ss.str();
    }

    ie->setId(id);
    ie->setName(name);
    getMap()[id] = ie;
    getTree()->add(ie);
}




InterfaceManager::InterfaceRootImpl::InterfaceRootImpl():
    structure(new JsonTreeRoot()){}

JsonElementMap &InterfaceManager::InterfaceRootImpl::getMap()
{
    return attributes;
}

std::shared_ptr<JsonGroupBase> InterfaceManager::InterfaceRootImpl::getTree()
{
    return structure;
}

void InterfaceManager::InterfaceRootImpl::clear()
{
    attributes.clear();
    structure->clear();
}

InterfaceManager::InterfaceRefImpl::InterfaceRefImpl(std::shared_ptr<JsonGroupBase> s, JsonElementMap &m):
    structure(s),
    attributes(m)
{ }

JsonElementMap &InterfaceManager::InterfaceRefImpl::getMap()
{return attributes;}

std::shared_ptr<JsonGroupBase> InterfaceManager::InterfaceRefImpl::getTree()
{
    return structure;
}

void InterfaceManager::InterfaceRefImpl::clear()
{
    std::cout<<"Impossible to clear InterfaceManager::InterfaceRefImpl"<<std::endl;
}

}
