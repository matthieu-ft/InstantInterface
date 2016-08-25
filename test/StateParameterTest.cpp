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
 #define BOOST_TEST_MODULE "TestPlane"

#include <boost/test/unit_test.hpp>

#include <InstantInterface/AttributeManagement.h>

#include <vector>

using namespace std;
using namespace InstantInterface;

BOOST_AUTO_TEST_SUITE(ParameterModifier)

BOOST_AUTO_TEST_CASE(StateParameterModifier)
{

    float v = 0;
    float period = 1;
    vector<float> states = {0.0f,0.12f,0.3f,0.45f,0.60f,0.91f};
    auto param = InstantInterface::AttributeFactory::makeAttribute(&v)->setMin(0)->setMax(period)->periodic(true)->enforceExtrema(false);
    auto modifier = makeStateValueModifier(param,states);

    int index, closestIndex, closestModuloIndex, state, periodPower;
    float value, computedValue;

    for(int periodPower = -10; periodPower<10; periodPower++)
    {
        for(int state = 0; state<states.size(); state++)
        {
            value = states[state] + (float)periodPower*period;
            index = periodPower*(int)states.size() + state;
            closestModuloIndex = modifier->closestModuloIndex(value);
            closestIndex = modifier->closestIndex(value);
            computedValue = modifier->getValueAtIndex(index);

            BOOST_CHECK_MESSAGE( state == closestModuloIndex, "The closest modulo index ("<<closestModuloIndex
                                 <<") is different from the state ("<<index
                                 <<"), for period ("<<periodPower<<")");
            BOOST_CHECK_MESSAGE( index == closestIndex, "The closest index ("<<closestIndex
                                 <<") is different from the index ("<<index
                                 <<"), for state ("<<state<<") and period ("<<periodPower<<")");
            BOOST_CHECK_MESSAGE( value == computedValue, "The computed value ("<<computedValue<<") is different from value ("<<value
                                 <<"), for state ("<<state<<") and period ("<<periodPower<<")");
        }
    }


}

BOOST_AUTO_TEST_SUITE_END()
