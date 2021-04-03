// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


// This include is ***REQUIRED*** 
// for ALL SST implementation files
#include "sst_config.h"

#include "basicParams.h"


using namespace SST;
using namespace SST::simpleElementExample;

/* 
 * During construction the basicParams component should prepare for simulation
 * - Read parameters
 * - Register clocks
 */
basicParams::basicParams(ComponentId_t id, Params& params) : Component(id) {

    // SST Output Object
    out = new Output("", 1, 0, Output::STDOUT);

    /*
     *  Parameter types can be any basic type (bool, int, uint, double, std::string, etc)
     *  OR any type with a constructor that takes a std::string
     *
     *  params.find<T>() has three arguments:
     *  parameter name: string name of the parameter to look for
     *  default value: value to return if the parameter isn't found
     *  bool indicating whether parameter was found: this is an optional parameter
     *
     *  Important: The default value provided in SST_ELI_DOCUMENT_PARAMS
     *  is not checked against the default value provided here. It is up
     *  to the component writer to make sure they match.
     */

    // Example 1: getting basic types from the parameters
    bool param0 = params.find<bool>("bool_param", false);
    uint32_t param1 = params.find<uint32_t>("uint32_param", 400);
    double param2 = params.find<double>("double_param", 12.5);
    std::string param3 = params.find<std::string>("string_param", "hello");

    out->output("Found basic parameters: bool_param = %d, uint32_param = %" PRIu32 ", double_param = %f, string_param = %s\n",
            param0, param1, param2, param3.c_str());

    // Example 2: discover whether a parameter was passed in from the python or not
    bool found;
    int param4 = params.find<int>("int_param", 0, found);
    
    if (!found) {
        out->fatal(CALL_INFO, -1, "Uh oh, in '%s', int_param is a required parameter, but it wasn't found in the parameter set.\n",
                getName().c_str());
    } else {
        out->output("Found the required parameter 'int_param' and got %d\n", param4);
    }

    // Example 3: more complex types that have constructors which take strings
    UnitAlgebra param5 = params.find<UnitAlgebra>("ua_param", "2TB/s");
    ExampleType param6 = params.find<ExampleType>("example_param", "key:5");

    out->output("Read ua_param = %s\n", param5.toStringBestSI().c_str());
    out->output("Read example_param. key = %s, value = %d\n", param6.key.c_str(), param6.value);

    // It is also possible to pass in arrays
    // The formatting is based on Python lists, see params.h/.cc in sst-core for a detailed description
    std::vector<int> intArray;
    params.find_array<int>("array_param", intArray);
    
    out->output("Read an array from array_param. Elements are: \n");
    for (std::vector<int>::iterator it = intArray.begin(); it != intArray.end(); it++) {
        out->output("%d, ", *it);
    }
    out->output("\n");

    // Tell the simulation not to end until we're ready
    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

    // Register a clock
    registerClock("1MHz", new Clock::Handler<basicParams>(this, &basicParams::tick));

}


/*
 * Destructor, clean up our output
 */
basicParams::~basicParams()
{
    delete out;
}


/* 
 * This example is about parameters so the clock doesn't do anything useful
 * Just exits after 20 cycles
 */
bool basicParams::tick( Cycle_t cycles)
{
    if (cycles == 20) {
        primaryComponentOKToEndSim();
        return true;
    }
    
    return false;
}

