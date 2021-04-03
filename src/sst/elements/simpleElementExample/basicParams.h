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

#ifndef _BASICPARAMS_H
#define _BASICPARAMS_H

/*
 * This example demonstrates parsing parameters from the Python input
 *
 * Concepts covered:
 *  - Reading basic types from Params
 *  - Reading more complex types from Params, including UnitAlgebra
 *  - Reading arrays from Params
 *  - Required and default parameter values
 *
 */

#include <sst/core/component.h>

namespace SST {
namespace simpleElementExample {


// Components inherit from SST::Component
class basicParams : public SST::Component
{
public:

/*
 *  SST Registration macros register Components with the SST Core and 
 *  document their parameters, ports, etc.
 *  SST_ELI_REGISTER_COMPONENT is required, the documentation macros
 *  are only required if relevant
 */
    // REGISTER THIS COMPONENT INTO THE ELEMENT LIBRARY
    SST_ELI_REGISTER_COMPONENT(
        basicParams,                        // Component class
        "simpleElementExample",             // Component library (for Python/library lookup)
        "basicParams",                      // Component name (for Python/library lookup)
        SST_ELI_ELEMENT_VERSION(1,0,0),     // Version of the component (not related to SST version)
        "Basic: managing clocks example",   // Description
        COMPONENT_CATEGORY_UNCATEGORIZED    // Category
    )

    // Document the parameters that this component accepts
    // { "parameter_name", "description", "default value or NULL if required" }
    //
    // Run 'sst-info simpleElementExample.basicParams' at the command line
    // to see how these are reported to users
    //
    // The default provided is just a string. It is for communicating to the user 
    // what will happen if they don't provide a value.
    // Thus, in cases where the default is not simple you can provide whatever makes sense.
    // Example: param0's default is 3*param1, then you could provide a default of "3 * param1" or "3 times param1".
    //
    SST_ELI_DOCUMENT_PARAMS(
        { "int_param",      "Integer parameter",    NULL },     // Required parameter
        { "bool_param",     "Boolean parameter",    "false"},   // Optional parameter
        { "string_param",   "String parameter",     "hello"},   
        { "uint32_param",   "uint32_t parameter",   "400"},
        { "double_param",   "double parameter",     "12.5"},
        { "ua_param",       "UnitAlgebra parameter","2TB/s"},
        { "example_param",  "ExampleType parameter","key:5"},
        { "array_param",    "array parameter",      "[]"},
    )

    // Optional since there is nothing to document
    SST_ELI_DOCUMENT_PORTS()
    
    // Optional since there is nothing to document
    SST_ELI_DOCUMENT_STATISTICS()

    // Optional since there is nothing to document - see SubComponent examples for more info
    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS( )

// Class members

    // Constructor. Components receive a unique ID and the set of parameters that were assigned in the Python input.
    basicParams(SST::ComponentId_t id, SST::Params& params);
    
    // Destructor
    ~basicParams();

    /* 
     * Define the 'ExampleType' which is a key,value pair where
     * the key is a string and the value is an integer.
     *
     * To use ExampleType directly with params.find<T>, we add
     * a constructor that takes a std::string of the form key:value.
     */
    class ExampleType {
        public:
            ExampleType(std::string str) {
                size_t pos = str.find(":"); // Find the delimeter
                if (pos == std::string::npos) {
                    Output * out = new Output("", 1, 0, Output::STDOUT);
                    out->fatal(CALL_INFO, -1, "Error initializing ExampleType. No delimeter (':') found in string '%s'\n", str.c_str());
                } else if (pos == 0) {
                    Output * out = new Output("", 1, 0, Output::STDOUT);
                    out->fatal(CALL_INFO, -1, "Error initializing ExampleType. No key found in string '%s'\n", str.c_str());
                } else if (pos == (str.size() - 1)) {
                    Output * out = new Output("", 1, 0, Output::STDOUT);
                    out->fatal(CALL_INFO, -1, "Error initializing ExampleType. No value found in string '%s'\n", str.c_str());
                }

               key = str.substr(0, pos);             // Everything before ':'
               value = std::stoi(str.substr(pos+1)); // Everything after ':'
            }

            ExampleType(std::string k, int v) : key(k), value(v) { }

            std::string key;
            int value;
    };

private:
   
    // Clock handler
    bool tick(SST::Cycle_t cycles);

    // SST Output object, for printing, error messages, etc.
    SST::Output* out;
     
};

} // namespace simpleElementExample
} // namespace SST

#endif /* _BASICPARAMS_H */
