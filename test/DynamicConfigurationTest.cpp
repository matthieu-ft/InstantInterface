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

//Link to Boost
 #define BOOST_TEST_DYN_LINK

//Define our Module name (prints at testing)
 #define BOOST_TEST_MODULE "DynamicConfigurationTest"

#include <boost/test/unit_test.hpp>

#include <InstantInterface/AttributeManagement.h>
#include <InstantInterface/Attributes.h>

#include <vector>
#include <cmath>

using namespace std;
using namespace InstantInterface;
using namespace InstantInterface::AttributeFactory;

BOOST_AUTO_TEST_SUITE(TestOfDynamicConfiguration)

#define EPSL 0.00001f

class MultiStateModifier : public StateModifier
{
public:
    MultiStateModifier(std::vector<std::shared_ptr<AttributeT<float> > > attrs_, float value):
        StateModifier(),
        value(value)
    {
        attrs.reserve(attrs_.size());
        ids.reserve(attrs_.size());
        for(auto attr: attrs_)
        {
           attrs.push_back(attr);
           ids.push_back(attr->getId());
        }
    }

    virtual float aimedValue() const {
      return value;
    }

    std::shared_ptr<StateModifier> getEquivalentStaticModifier_currentState()
    {
        return std::make_shared<MultiStateModifier> (vector<FloatAttribute>(), 0);
    }

    std::shared_ptr<StateModifier> getEquivalentStaticModifier_aimedState()
    {
        return std::make_shared<MultiStateModifier> (vector<FloatAttribute>(), 0);
    }

    std::vector<int> getParameterIds() const
    {
        return ids;
    }

    void mix(float transitionFactor) {
      if(transitionFactor < 0)
          return;

        for(auto attr : attrs)
        {
            auto pAttr = attr.lock();


            if(transitionFactor >= 1)
                pAttr->set(aimedValue());
            else
                pAttr->set((1-transitionFactor)*pAttr->get() + transitionFactor * aimedValue());
        }
    }


private:
    std::vector<std::weak_ptr<AttributeT<float> > > attrs;
    float value;
    std::vector<int> ids;
};


BOOST_AUTO_TEST_CASE(MultiIdManagement)
{
    float v1 = 0;
    float v2 = 0;
    float v3 = 0;

    auto a1 = makeAttribute(&v1);
    auto a2 = makeAttribute(&v2);
    auto a3 = makeAttribute(&v3);

    DynamicConfiguration dc;

    auto modif1 = makeValueModifier(a1, 1);

    float ts1 = 2;
    float ts2 = 1;
    auto tsa1 = makeAttribute(&ts1);
    auto tsa2 = makeAttribute(&ts2);

    auto action1 = makeTransitionAction(dc, {modif1}, tsa1);
    action1->applyAction();

    dc.apply(250);
    BOOST_CHECK(abs(v1-0.5f)<EPSL);
    dc.apply(250);
    BOOST_CHECK(abs(v1-1.0f)<EPSL);

    v1 = 0;
    v2 = 0;
    dc.reset();

    auto modif2 = std::make_shared<MultiStateModifier>(vector<FloatAttribute>{a1,a2}, 10);
    auto modif3 = makeValueModifier(a1, 0)->setPersistence(true);
    auto modif4 = makeValueModifier(a2, 0)->setPersistence(true);

    float ts3 = 10000;
    auto tsa3 = makeAttribute(&ts3);

    auto action2 = makeTransitionAction(dc, {modif2}, tsa2);
    auto action3 = makeTransitionAction(dc, {modif3}, tsa3);
    auto action4 = makeTransitionAction(dc, {modif4}, tsa3);

    //we do this first step so that the two attributes already have a modifier to maintain a linear modification for the next action
    action3->applyAction();
    action4->applyAction();
    dc.apply(1000);


    action2->applyAction();
    action1->applyAction();
    dc.apply(250);
    BOOST_CHECK(abs(v2-10.0f*250.0f/1000.0f) < EPSL);
    BOOST_CHECK(v1<v2);
    dc.apply(250);
    BOOST_CHECK(abs(v1 - 1.0f) < EPSL);
    BOOST_CHECK(abs(v2-10.0f*500.0f/1000.0f) < EPSL);
    dc.apply(500);
    BOOST_CHECK(std::abs(v1-1.0f) < EPSL);
    BOOST_CHECK(abs(v2-10.0f) < EPSL);

}

BOOST_AUTO_TEST_SUITE_END()
