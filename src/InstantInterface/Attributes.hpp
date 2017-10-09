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

#include "Attributes.h"

#include <algorithm>

namespace InstantInterface
{
template <class T>
int IndexedBase<T>::next_id = 0;


template <class T>
AttributeT<T>::AttributeT(AttributeT<T>& attribute):
    AttributeT<T>()
{
    _min = attribute._min;
    _max = attribute._max;
    _hasMin = attribute._hasMin;
    _hasMax = attribute._hasMax;
    _enforceExtrema = attribute._enforceExtrema;
    _isPeriodic = attribute._isPeriodic;
    _derivedAttributes = {};
    _name = attribute._name;
    listeners = {};
}

template <class T>
AttributeT<T>::AttributeT(std::vector<DerivedAttribute> derAtt):
    IndexedModifiable(),
    _min(0),
    _max(0),
    _hasMin(false),
    _hasMax(false),
    _enforceExtrema(true),
    _isPeriodic(false),
    _derivedAttributes(derAtt),
    _name("empty name")
{}



template <class T>
void AttributeT<T>::set(T value, bool notifyUpdate)
{
    T filteredValue = value;
    if(_enforceExtrema)
    {
        if(_hasMax)
            filteredValue = std::min(filteredValue,_max);
        if(_hasMin)
            filteredValue = std::max(_min, filteredValue);
    }

    _set(filteredValue);

    if(notifyUpdate)
        for(auto const& listener : listeners)
            listener.second(this->shared_from_this());

    for(auto& fun: _derivedAttributes)
        fun();
}

template <class T>
typename AttributeT<T>::Ptr AttributeT<T>::setMin(T value)
{
    _min = value;
    _hasMin = true;
    return this->shared_from_this();
}

template <class T>
typename AttributeT<T>::Ptr AttributeT<T>::setMax(T value)
{
    _max = value;
    _hasMax = true;
    return this->shared_from_this();
}

template <class T>
T AttributeT<T>::getPeriod() const
{
    return _max - _min;
}

template <class T>
T AttributeT<T>::getMin()
{
    return _min;
}

template <class T>
T AttributeT<T>::getMax()
{
    return _max;
}

template <class T>
bool AttributeT<T>::hasMin()
{
    return _hasMin;
}

template <class T>
bool AttributeT<T>::hasMax()
{
    return _hasMax;
}

template <class T>
bool AttributeT<T>::isPeriodic() const
{
    return _isPeriodic;
}

template <class T>
typename AttributeT<T>::Ptr AttributeT<T>::periodic(bool v)
{
    _isPeriodic = v;
    return this->shared_from_this();
}

template <class T>
typename AttributeT<T>::Ptr AttributeT<T>::enforceExtrema(bool v)
{
    _enforceExtrema = v;
    return this->shared_from_this();
}


template <class T>
typename AttributeT<T>::Ptr AttributeT<T>::setName(std::string name)
{
    _name = name;
    return this->shared_from_this();
}


template <class T>
const std::string& AttributeT<T>::getName() const
{
    return _name;
}

template <class T>
TypeValue AttributeT<T>::getTypeValue() const
{
    return getValueFromType<T>();
}

template <class T>
void AttributeT<T>::addListener(void * ptr, std::function<void(Ptr)> listener)
{
    listeners[ptr] = listener;
}

template <class T>
void AttributeT<T>::removeListener(void * ptr)
{
    auto it = listeners.find(ptr);
    if(it!=listeners.end())
        listeners.erase(it);
}

template <class T>
AttributePtr AttributeT<T>::makeFakeCopy() {
    return this->makeFakeCopyT();
}

template <class T>
typename AttributeT<T>::Ptr AttributeT<T>::makeFakeCopyT() {
    return std::make_shared<FakeAttributeT<T> > (*this);
}


}
