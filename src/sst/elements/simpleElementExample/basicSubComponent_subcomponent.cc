// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


// This include is ***REQUIRED*** 
// for ALL SST implementation files
#include "sst_config.h"

#include "basicSubComponent_subcomponent.h"


using namespace SST;
using namespace SST::simpleElementExample;

/***********************************************************************************/
// Since the classes are brief, this file has the implementation for all four 
// basicSubComponentAPI subcomponents declared in basicSubComponent_subcomponent.h
/***********************************************************************************/

// basicSubComponentIncrement

basicSubComponentIncrement::basicSubComponentIncrement(ComponentId_t id, Params& params) :
    basicSubComponentAPI(id, params) 
{
    amount = params.find<int>("amount",  1);
}

basicSubComponentIncrement::~basicSubComponentIncrement() { }

int basicSubComponentIncrement::compute( int num )
{
    return num + amount;
}

std::string basicSubComponentIncrement::compute ( std::string comp )
{
    return "(" + comp + ")" + " + " + std::to_string(amount);
}
/***********************************************************************************/
// basicSubComponentDecrement

basicSubComponentDecrement::basicSubComponentDecrement(ComponentId_t id, Params& params) :
    basicSubComponentAPI(id, params) 
{
    amount = params.find<int>("amount",  1);
}

basicSubComponentDecrement::~basicSubComponentDecrement() { }

int basicSubComponentDecrement::compute( int num )
{
    return num - amount;
}

std::string basicSubComponentDecrement::compute ( std::string comp )
{
    return "(" + comp + ")" + " - " + std::to_string(amount);
}
/***********************************************************************************/
// basicSubComponentMultiply

basicSubComponentMultiply::basicSubComponentMultiply(ComponentId_t id, Params& params) :
    basicSubComponentAPI(id, params) 
{
    amount = params.find<int>("amount",  1);
}

basicSubComponentMultiply::~basicSubComponentMultiply() { }

int basicSubComponentMultiply::compute( int num )
{
    return num * amount;
}

std::string basicSubComponentMultiply::compute ( std::string comp )
{
    return "(" + comp + ")" + " * " + std::to_string(amount);
}
/***********************************************************************************/
// basicSubComponentDivide

basicSubComponentDivide::basicSubComponentDivide(ComponentId_t id, Params& params) :
    basicSubComponentAPI(id, params) 
{
    amount = params.find<int>("amount",  1);

    if ( amount == 0 ) 
        fatal(CALL_INFO, -1, "%s, Error: divide compute unit cannot divide by 0. Fix the 'amount' parameter.\n",
                getName().c_str());
}

basicSubComponentDivide::~basicSubComponentDivide() { }

int basicSubComponentDivide::compute( int num )
{
    return num / amount;
}

std::string basicSubComponentDivide::compute ( std::string comp )
{
    return "(" + comp + ")" + " / " + std::to_string(amount);
}
