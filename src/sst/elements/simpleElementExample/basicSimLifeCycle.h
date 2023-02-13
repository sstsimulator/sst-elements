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

#ifndef _BASIC_SIM_LIFECYCLE_H
#define _BASIC_SIM_LIFECYCLE_H

/*
 * This Component example demonstrates the use of SST's "lifecycle" functions.
 *
 * These components can be instantiated in a ring and each
 * component has a right and a left link. Components let each
 * other know how many events they want to receive and each component
 * sends each other the requested number of events. At the end of simulation,
 * components notify each other of this information and print it.
 * 
 * Simulation lifecycle
 * 1) Construction
 *      Components ensure both their links are connected
 *      Components read params
 * 2) Init
 *      Components discover the names of the other components in the ring
 * 3) Setup
 *      Components report the names of the components they discovered
 *      Components send an initial event to start the simulation. 
 *      This is required because this is a purely event-based simulation
 *      so an event is needed to start it.
 * 4) Run
 *      Components send and receive messages
 *      Components send events to the left
 *          - If a component receives an event for itself, it deletes the event and sends a new event if is has not sent enough events yet 
 *          - If a component receives an event for a different component, it forwards the event to the left
 *      Simulation ends when there are no events left in the system
 * 5) Complete
 *      Components tell their left neighbor goodbye and their right neighbor farewell
 * 6) Finish
 *      Components print who told them what during complete()
 * 7) Destruction
 *      Components clean up memory
 *
 *  If SST receives a SIGINT or SIGTERM, each component reports how many events are left to send before it terminates
 *  If SST receives a SIGUSR2, each component reports how many events are left to send and continues
 *
 * Concepts covered:
 *  - Use of init(), setup(), complete(), and finish()
 *  - Use of printStatus() and emergencyShutdown()
 */

#include <sst/core/component.h>
#include <sst/core/link.h>


namespace SST {
namespace simpleElementExample {


// Components inherit from SST::Component
class basicSimLifeCycle : public SST::Component
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
        basicSimLifeCycle,              // Component class
        "simpleElementExample",         // Component library (for Python/library lookup)
        "basicSimLifeCycle",            // Component name (for Python/library lookup)
        SST_ELI_ELEMENT_VERSION(1,0,0), // Version of the component (not related to SST version)
        "Component to demonstrate SST simulation lifecycle", // Description
        COMPONENT_CATEGORY_PROCESSOR    // Category
    )

    // Document the parameters that this component accepts
    SST_ELI_DOCUMENT_PARAMS(
        { "eventsToSend",   "How many events this component should send to each other component in the ring.", NULL},
        { "verbose",        "Set to true to print every event this component receives.", "false"}
    )

    // Document the ports that this component has
    // {"Port name", "Description", { "list of event types that the port can handle"} }
    SST_ELI_DOCUMENT_PORTS(
        {"left",  "Left link to another component", { "simpleElementExample.basicLifeCycleEvent" } },
        {"right", "Right link to another component", { "simpleElementExample.basicLifeCycleEvent" } }
    )
    
    // SST_ELI_DOCUMENT_STATISTICS and SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS are not declared since they are not used


// Class members

    // Constructor. Components receive a unique ID and the set of parameters that were assigned in the Python input.
    basicSimLifeCycle(SST::ComponentId_t id, SST::Params& params);
    
    // Destructor
    virtual ~basicSimLifeCycle();

    // Called by SST during SST's init() lifecycle phase
    virtual void init(unsigned phase) override;

    // Called by SST during SST's setup() lifecycle phase
    virtual void setup() override;
    
    // Called by SST during SST's complete() lifecycle phase
    virtual void complete(unsigned phase) override;

    // Called by SST during SST's finish() lifecycle phase
    virtual void finish() override;

    // Called by SST if it detects SIGINT or SIGTERM or if a fatal error occurs
    virtual void emergencyShutdown() override;

    // Called by SST if it receives a SIGUSR2
    virtual void printStatus(Output& out) override;

    // Event handler, called when an event is received
    void handleEvent(SST::Event *ev);


private:
    // Parameters
    unsigned eventsToSend;
    bool verbose;

    // Component state
    unsigned eventsReceived;                // Number of events we've received
    unsigned eventsForwarded;               // Number of events we've forwarded
    unsigned eventsSent;                    // Number of events we've sent (initiated)
    std::set<std::string> neighbors;        // Set of all neighbors on the ring
    std::set<std::string>::iterator iter;   // Next component to send to in the eventRequests map 

    // Additional state reported during finish
    std::string leftMsg, rightMsg;

    // SST Output object, for printing, error messages, etc.
    SST::Output* out;

    // Links
    SST::Link* leftLink;
    SST::Link* rightLink;
};

} // namespace simpleElementExample
} // namespace SST

#endif /* _BASIC_SIM_LIFECYCLE_H */
