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

#ifndef _EXAMPLE1_H
#define _EXAMPLE1_H

/*
 * This example builds on the first one with two changes:
 *  1. Event sizes are randomly selected between 0 and eventSize for each event
 *  2. The component uses a Statistic to count the number of payload bytes it received
 *
 * Concepts covered:
 *  - Using SST's random number generators
 *  - Statistics
 *
 */

#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/rng/marsaglia.h>

namespace SST {
namespace simpleElementExample {


// Components inherit from SST::Component
class example1 : public SST::Component
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
        example1,                           // Component class
        "simpleElementExample",             // Component library (for Python/library lookup)
        "example1",                         // Component name (for Python/library lookup)
        SST_ELI_ELEMENT_VERSION(1,0,0),     // Version of the component (not related to SST version)
        "Example #2, statistics & RNG",     // Description
        COMPONENT_CATEGORY_UNCATEGORIZED    // Category
    )

    // Document the parameters that this component accepts
    // { "parameter_name", "description", "default value or NULL if required" }
    SST_ELI_DOCUMENT_PARAMS(
        { "eventsToSend", "How many events this component should send.", NULL},
        { "eventSize",    "Payload size for each event, in bytes.", "16"}
    )

    // Document the ports that this component has
    // {"Port name", "Description", { "list of event types that the port can handle"} }
    SST_ELI_DOCUMENT_PORTS(
        {"port",  "Link to another component", { "simpleElementExample.basicEvent", ""} }
    )
    
    // Document the statistic that this component provides
    // { "statistic_name", "description", "units", enable_level }
    SST_ELI_DOCUMENT_STATISTICS( 
        {"EventSizeReceived", "Records the payload size of each event received", "bytes", 1}
    )

    // Optional since there is nothing to document - see SubComponent examples for more info
    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS( )

// Class members

    // Constructor. Components receive a unique ID and the set of parameters that were assigned in the Python input.
    example1(SST::ComponentId_t id, SST::Params& params);
    
    // Destructor
    ~example1();

private:
    // Event handler, called when an event is received on our link
    void handleEvent(SST::Event *ev);

    // Clock handler, called on each clock cycle
    virtual bool clockTic(SST::Cycle_t);

    // Parameters
    int64_t eventsToSend;
    int eventSize;
    bool lastEventReceived;

    // Statistic
    Statistic<uint64_t>* bytesReceived;

    // Random number generator
    SST::RNG::MarsagliaRNG* rng;

    // SST Output object, for printing, error messages, etc.
    SST::Output* out;

    // Links
    SST::Link* link;
};

} // namespace simpleElementExample
} // namespace SST

#endif /* _EXAMPLE1_H */
