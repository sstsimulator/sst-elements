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

#ifndef _BASIC_SUBCOMPONENT_SUBCOMPONENT_H
#define _BASIC_SUBCOMPONENT_SUBCOMPONENT_H

/*
 * This is an example of a simple subcomponent that can take a number and do a computation on it. 
 * This file happens to have multiple classes declaring both the SubComponentAPI 
 * as well as a few subcomponents that implement the API.
 *  
 *  Classes:
 *      basicSubComponentAPI - inherits from SST::SubComponent. Defines the API for the compute units.
 *      basicSubComponentIncrement - inherits from basicSubComponentAPI. A compute unit that increments the input.
 *      basicSubComponentDecrement - inherits from basicSubComponentAPI. A compute unit that decrements the input.
 *      basicSubComponentMultiply - inherits from basicSubComponentAPI. A compute unit that multiplies the input.
 *      basicSubComponentDivide - inherits from basicSubComponentAPI. A compute unit that divides the input.
 *
 * See 'basicSubComponent_component.h' for more information on how the example simulation works
 */

#include <sst/core/subcomponent.h>

namespace SST {
namespace simpleElementExample {

/*****************************************************************************************************/

// SubComponent API for a SubComponent that has two functions:
// 1) It does a specific computation on a number
//      Ex. Given 5, if the computation is 'x4', the subcomponent should return 20
// 2) Given a formula string, it will update the formula with the computation it does
//      Ex. Given '3+2', if the computation is 'x4', the subcomponent should return '(3+2)x4'

class basicSubComponentAPI : public SST::SubComponent
{
public:
    /* 
     * Register this API with SST so that SST can match subcomponent slots to subcomponents 
     */
    SST_ELI_REGISTER_SUBCOMPONENT_API(SST::simpleElementExample::basicSubComponentAPI)
    
    basicSubComponentAPI(ComponentId_t id, Params& params) : SubComponent(id) { }
    virtual ~basicSubComponentAPI() { }

    // These are the two functions described in the comment above
    virtual int compute( int num ) =0;
    virtual std::string compute( std::string comp) =0;
    
};

/*****************************************************************************************************/

/* SubComponent that does an 'increment' computation */
class basicSubComponentIncrement : public basicSubComponentAPI {
public:
    
    // Register this subcomponent with SST and tell SST that it implements the 'basicSubComponentAPI' API
    SST_ELI_REGISTER_SUBCOMPONENT(
            basicSubComponentIncrement,     // Class name
            "simpleElementExample",         // Library name, the 'lib' in SST's lib.name format
            "basicSubComponentIncrement",   // Name used to refer to this subcomponent, the 'name' in SST's lib.name format
            SST_ELI_ELEMENT_VERSION(1,0,0), // A version number
            "SubComponent that increments a value", // Description
            SST::simpleElementExample::basicSubComponentAPI // Fully qualified name of the API this subcomponent implements
                                                            // A subcomponent can implment an API from any library
            )

    // Other ELI macros as needed for parameters, ports, statistics, and subcomponent slots
    SST_ELI_DOCUMENT_PARAMS( { "amount", "Amount to increment by", "1" } )

    basicSubComponentIncrement(ComponentId_t id, Params& params);
    ~basicSubComponentIncrement();

    int compute( int num) override;
    std::string compute( std::string comp ) override;

private:
    int amount;
};

/*****************************************************************************************************/

/* SubComponent that does a 'decrement' computation */
class basicSubComponentDecrement : public basicSubComponentAPI {
public:
    
    // Register this subcomponent with SST and tell SST that it implements the 'basicSubComponentAPI' API
    SST_ELI_REGISTER_SUBCOMPONENT(
            basicSubComponentDecrement,     // Class name
            "simpleElementExample",         // Library name, the 'lib' in SST's lib.name format
            "basicSubComponentDecrement",   // Name used to refer to this subcomponent, the 'name' in SST's lib.name format
            SST_ELI_ELEMENT_VERSION(1,0,0), // A version number
            "SubComponent that decrements a value", // Description
            SST::simpleElementExample::basicSubComponentAPI // Fully qualified name of the API this subcomponent implements
            )

    // Other ELI macros as needed for parameters, ports, statistics, and subcomponent slots
    SST_ELI_DOCUMENT_PARAMS( { "amount", "Amount to decrement by", "1" } )

    basicSubComponentDecrement(ComponentId_t id, Params& params);
    ~basicSubComponentDecrement();

    int compute( int num) override;
    std::string compute( std::string comp ) override;

private:
    int amount;
};
/*****************************************************************************************************/
/* SubComponent that does a 'multiply' computation */
class basicSubComponentMultiply : public basicSubComponentAPI {
public:
    
    // Register this subcomponent with SST and tell SST that it implements the 'basicSubComponentAPI' API
    SST_ELI_REGISTER_SUBCOMPONENT(
            basicSubComponentMultiply,      // Class name
            "simpleElementExample",         // Library name, the 'lib' in SST's lib.name format
            "basicSubComponentMultiply",    // Name used to refer to this subcomponent, the 'name' in SST's lib.name format
            SST_ELI_ELEMENT_VERSION(1,0,0), // A version number
            "SubComponent that multiplies a value", // Description
            SST::simpleElementExample::basicSubComponentAPI // Fully qualified name of the API this subcomponent implements
            )

    // Other ELI macros as needed for parameters, ports, statistics, and subcomponent slots
    SST_ELI_DOCUMENT_PARAMS( { "amount", "Amount to multiply by", "1" } )

    basicSubComponentMultiply(ComponentId_t id, Params& params);
    ~basicSubComponentMultiply();

    int compute( int num) override;
    std::string compute( std::string comp ) override;

private:
    int amount;
};
/*****************************************************************************************************/
/* SubComponent that does an 'divide' computation */
class basicSubComponentDivide : public basicSubComponentAPI {
public:
    
    // Register this subcomponent with SST and tell SST that it implements the 'basicSubComponentAPI' API
    SST_ELI_REGISTER_SUBCOMPONENT(
            basicSubComponentDivide,        // Class name
            "simpleElementExample",         // Library name, the 'lib' in SST's lib.name format
            "basicSubComponentDivide",      // Name used to refer to this subcomponent, the 'name' in SST's lib.name format
            SST_ELI_ELEMENT_VERSION(1,0,0), // A version number
            "SubComponent that divides a value", // Description
            SST::simpleElementExample::basicSubComponentDivide // Fully qualified name of the API this subcomponent implements
            )

    // Other ELI macros as needed for parameters, ports, statistics, and subcomponent slots
    SST_ELI_DOCUMENT_PARAMS( { "amount", "Amount to divide by", "1" } )

    basicSubComponentDivide(ComponentId_t id, Params& params);
    ~basicSubComponentDivide();

    int compute( int num) override;
    std::string compute( std::string comp ) override;

private:
    int amount;
};
/*****************************************************************************************************/

} } /* Namspaces */

#endif /* _BASIC_SUBCOMPONENT_SUBCOMPONENT_H */
