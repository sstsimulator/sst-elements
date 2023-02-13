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

#ifndef _BASICSUBCOMPONENT_COMPONENT_H
#define _BASICSUBCOMPONENT_COMPONENT_H

/*
 * This example demonstrates the use of subcomponents.
 *
 * Concepts covered:
 *  - ELI macros for defining subcomponent slots
 *  - Loading a user vs anonymous subcomponent
 *  - Declaring a subcomponent api
 *  - Defining subcomponents
 *  
 *
 *  This file defines a Component with a SubComponent slot.
 *
 *
 *  The simulation works this way:
 *      - The components are connected in a ring
 *      - Each component has a "compute" SubComponent that controls what kind of computation it does
 *      - Each component sends one message around the ring which contains an integer and a string describing the computation
 *      - When a component receives a message, it has its "compute" SubComponent operate on the integer
 *          and then forwards the integer to its neighbor. It also updates the string to describe what it did.
 *      - When a component receives the message it sent, it prints the string and the result
 *      - Whan all components have received the messages they sent, the simulation ends
 */

#include <sst/core/component.h>
#include <sst/core/link.h>

// Include file for the SubComponent API we'll use
#include "basicSubComponent_subcomponent.h"

namespace SST {
namespace simpleElementExample {


class basicSubComponent_Component : public SST::Component
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
        basicSubComponent_Component,        // Component class
        "simpleElementExample",             // Component library (for Python/library lookup)
        "basicSubComponent_comp",           // Component name (for Python/library lookup)
        SST_ELI_ELEMENT_VERSION(1,0,0),     // Version of the component (not related to SST version)
        "Basic: Using subcomponents",       // Description
        COMPONENT_CATEGORY_UNCATEGORIZED    // Category
    )

    SST_ELI_DOCUMENT_PARAMS(
        { "value",      "Integer starting value for this component", NULL }
    )

    SST_ELI_DOCUMENT_PORTS(
            {"left", "Link to left neighbor"},
            {"right", "Link to right neighbor"}
    )
    
    SST_ELI_DOCUMENT_STATISTICS()

    // This Macro informs SST that this Component can load a SubComponent.
    // Parameter 1: Name of the location for the subcomponent
    // Parameter 2: Description of the purpose/use/etc. of the slot 
    // Parameter 3: The API the subcomponent slot will use
    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS( 
            { "compute_unit", 
            "The compute unit that this component will use to operate on events", 
            "SST::simpleElementExample::basicSubComponentAPI" } 
            )

    // Constructor & destructor
    basicSubComponent_Component(SST::ComponentId_t id, SST::Params& params);
    ~basicSubComponent_Component();
    
    // We will use the 'setup' function from the simulation lifecycle to send our event
    virtual void setup() override;

    // Event handler, called when an event is received on either link
    void handleEvent(SST::Event* ev);

private:
   
    // SST Output object, for printing, error messages, etc.
    SST::Output* out;
    
    // Input parameter: the value this component will send in its event
    int value;

    // Links
    SST::Link* leftLink;
    SST::Link* rightLink;

    // SubComponent: Our compute unit
    SST::simpleElementExample::basicSubComponentAPI* computeUnit;
     
};

} // namespace simpleElementExample
} // namespace SST

#endif /* _BASICSUBCOMPONENT_COMPONENT_H */
