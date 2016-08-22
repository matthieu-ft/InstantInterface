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

#include <cstdlib>
#include <memory>

namespace InstantInterface {

/**
 *  @brief IndexedBase is a class that gives an id to each new instance of the class.
 * The class is templated in order to have a different separate incrementation system for each child class.
 *
 * example:
 *  class A : public IndexedBase<A> {};
 *  class B : public IndexedBase<B> {};
 *
 *  A a1; //id = 0
 *  A a2; //id = 1
 *  B b; // id = 0
 *
 */

template <class T>
class IndexedBase
{
public:
    IndexedBase()
    {
        id = next_id;
        next_id++;
    }

    size_t getId() const {return id;}

private:
    static int next_id;
    int id;
};

/**
 *  @brief This class encapsulates/defines an interface for an attribute. It has a set() get(). They have to be implemented in subclasses depending of the
 * nature of the attribute (ptr to a variable, getter setter, lambda functions...)
 */
template <typename T>
class AttributeT : public IndexedBase<AttributeT<T> >, public std::enable_shared_from_this<AttributeT<T> >
{
public:
    typedef std::shared_ptr<AttributeT<T> > Ptr;

    AttributeT();

    /**
     * @brief set value of the attribute
     * @param value
     */
    void set(T value);

    /**
     * @brief get the value of the attribute
     * @return
     */
    virtual T get() = 0;

    /**
     * @brief defines a minimun value for the attribute
     * @param value
     * @return
     */
    Ptr min(T value);

    /**s
     * @brief defines a maximun value for the attribute
     * @param value
     * @return
     */
    Ptr max(T value);

    /**
     * @brief compute the period of the attribute based on the min, max value
     * @return
     */
    T getPeriod() const;

    /**
     * @brief get min value
     * @return
     */
    T min();

    /**
     * @brief get max value
     * @return
     */
    T max();

    /**
     * @brief returns true if a minimum value has been defined. False otherwise.
     * @return
     */
    bool hasMin();

    /**
     * @brief returns false if a maximum value has been defined. False otherwise.
     * @return
     */
    bool hasMax();

    /**
     * @brief returns true if the attribute has been defined as periodic. False otherwise
     * @return
     */
    bool isPeriodic() const;

    /**
     * @brief defines the periodicity of the attribute
     * @param v
     * @return
     */
    Ptr periodic(bool v);

    /**
     * @brief if the enforceExtrema modus is actived, then when set() is being called and the value is out of bound, the value is truncated to the min or the max.
     * Activated per default.
     * @param v
     * @return
     */
    Ptr enforceExtrema(bool v);

protected:
    virtual void _set(T value) = 0;

private:
    T _min, _max;
    bool _hasMin, _hasMax, _enforceExtrema;
    bool _isPeriodic;
};



/**
 *  @brief AttributeT_Raw implements the AttributeT interface when the attribute is given by a pointer to a variable.
 */
template <class ParamType>
class AttributeT_Raw : public AttributeT<ParamType>
{
public:
    AttributeT_Raw(ParamType* pParam):
        ptr(pParam)
    {}

    ParamType get()
    {
        return *ptr;
    }

protected:
    void _set(ParamType val)
    {
        *ptr = val;
    }

private:
    ParamType* ptr;
};


/**
 *  @brief AttributeT_Method1 defines the AttributeT interface for the case when the attribute is accessible through getter setter methods (with constness)
 */
template <class ObjType, class ParamType>
class AttributeT_Method_1 : public AttributeT<ParamType>
{
public:

    AttributeT_Method_1(
            ObjType* obj,
            const ParamType& (ObjType::* get)() const,
            void (ObjType::* set)(const ParamType&)):
        object(obj),
        getter(get),
        setter(set)
    {}

    ParamType get()
    {
        return (object->*getter)();
    }

protected:
    void _set(ParamType val)
    {
        (object->*setter)(val);
    }

private:
    ObjType* object;
    const ParamType& (ObjType::* getter)() const;
    void (ObjType::* setter)(const ParamType&);
};


/**
 *  @brief AttributeT_Method_2 defines the AttributeT interface for the case when the attribute is accessible through getter setter methods (with semi constness)
 */
template <class ObjType, class ParamType>
class AttributeT_Method_2 : public AttributeT<ParamType>
{
public:

    AttributeT_Method_2(
            ObjType* obj,
            ParamType (ObjType::* get)() const,
            void (ObjType::* set)(const ParamType&)):
        object(obj),
        getter(get),
        setter(set)
    {}

    ParamType get()
    {
        return (object->*getter)();
    }

protected:
    void _set(ParamType val)
    {
        (object->*setter)(val);
    }


private:
    ObjType* object;
    ParamType (ObjType::* getter)() const;
    void (ObjType::* setter)(const ParamType&);
};


/**
 *  @brief AttributeT_Method_3 defines the AttributeT interface for the case when the attribute is accessible through getter setter methods (without constness)
 */
template <class ObjType, class ParamType>
class AttributeT_Method_3 : public AttributeT<ParamType>
{
public:

    AttributeT_Method_3(
            ObjType* obj,
            ParamType (ObjType::* get)() const,
            void (ObjType::* set)(ParamType)):
        object(obj),
        getter(get),
        setter(set)
    {}

    ParamType get()
    {
        return (object->*getter)();
    }

protected:
    void _set(ParamType val)
    {
        (object->*setter)(val);
    }

private:
    ObjType* object;
    ParamType (ObjType::* getter)() const;
    void (ObjType::* setter)(ParamType);
};

/**
 * @brief AttributeT_Lambda defines the AttributeT interface for the case when the attribute is accessible through lambda getter setters
 */
template <class ParamType, class LambdaGetter, class LambdaSetter>
struct AttributeT_lambda : public AttributeT<ParamType>
{
public:
    AttributeT_lambda(LambdaGetter g, LambdaSetter s):
        getter(g),
        setter(s)
    {}

    //this constructor is used so that we force the write type for objtype with some value
    AttributeT_lambda(LambdaGetter g, LambdaSetter s, const ParamType& value):
        getter(g),
        setter(s)
    {}

    ParamType get()
    {
        return getter();
    }

protected:
    void _set(ParamType val)
    {
        setter(val);
    }

private:
    LambdaGetter getter;
    LambdaSetter setter;
};


/**
 * @brief The Action class is a generic interface for triggering an action (the call of a function without parameter)
 */
class Action
{
public:
    virtual void applyAction() = 0;
};

/**
 *@brief implements the Action interface. Typically action will be defined by a lambda function.
 */
template <class T>
class ActionT: public Action
{
public:

    ActionT(T a) : action(a)
    { }

    void applyAction()
    {
        action();
    }

private:
    T action;
};


/**
 * @brief the helper functions defined in AttributeFactory enable to create Attributes with taking little care to the template parameter, as most of the work is done automatically
 * at the function call with template argument resolution.
 */
namespace AttributeFactory{

    /**
     * @brief create action from lambda
     */
    template <class T>
    std::shared_ptr<ActionT<T> > makeAction(T lambda)
    {
    auto p = std::make_shared<ActionT<T> >(lambda);
    return p;
    }

    /**
     * @brief create Attribute from pointer to a variable
     */
    template <class T>
    std::shared_ptr<AttributeT<T> > makeAttribute(T* ptr)
    {
        auto p = std::make_shared<AttributeT_Raw<T> >(ptr);
        return p;
    }

    /**
     * @brief create Attribue based on lambda setter and getter. The resolution of the attribute type is done with the value given
     * as last parameter (but its value is not used)
     */
    template <class T, class LambdaGetter, class LambdaSetter>
    std::shared_ptr<AttributeT<T> > makeAttribute(LambdaGetter getter, LambdaSetter setter, T value)
    {
        auto p = std::make_shared<AttributeT_lambda<T,LambdaGetter,LambdaSetter> >(getter,setter);
        return p;
    }

    /**
     * @brief create Attribue based on lambda setter and getter. The type of the attribute can't be resolved automatically.
     * It has to be precised as type parameter. For instance makeAttribute<float> will have to be used for a float attribute
     */
    template <class T, class LambdaGetter, class LambdaSetter>
    std::shared_ptr<AttributeT<T> > makeAttribute(LambdaGetter getter, LambdaSetter setter)
    {
        auto p = std::make_shared<AttributeT_lambda<T,LambdaGetter,LambdaSetter> >(getter,setter);
        return p;
    }

    /**
     * @brief create Attribue based on object and a getter/setter of this object
     */
    template <class ObjType, class T>
    std::shared_ptr<AttributeT<T> > makeAttribute(
            ObjType* obj,
            const T& (ObjType::* get)() const,
            void (ObjType::* set)(const T&))
    {
        auto p = std::make_shared<AttributeT_Method_1<ObjType,T> >(obj, get, set);
        return p;
    }

    /**
     * @brief create Attribue based on object and a getter/setter of this object
     */
    template <class ObjType, class T>
    std::shared_ptr<AttributeT<T> > makeAttribute(
            ObjType* obj,
            T (ObjType::* get)() const,
            void (ObjType::* set)(const T&))
    {
        auto p = std::make_shared<AttributeT_Method_2<ObjType,T> >(obj, get, set);
        return p;
    }

    /**
     * @brief create Attribue based on object and a getter/setter of this object
     */
    template <class ObjType, class T>
    std::shared_ptr<AttributeT<T> > makeAttribute(
            ObjType* obj,
            T (ObjType::* get)() const,
            void (ObjType::* set)(T))
    {
        auto p = std::make_shared<AttributeT_Method_3<ObjType,T> >(obj, get, set);
        return p;
    }
}

}

#include "Attributes.hpp"
