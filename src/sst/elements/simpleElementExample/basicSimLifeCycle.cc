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

#include "basicSimLifeCycle.h"
#include "basicSimLifeCycleEvent.h"


using namespace SST;
using namespace SST::simpleElementExample;

/* 
 * Lifecycle Phase #1: Construction
 * - Configure output object
 * - Ensure both links are connected and configure
 * - Read parameters
 * - Initialize internal state
 */
basicSimLifeCycle::basicSimLifeCycle(ComponentId_t id, Params& params) : Component(id) 
{

    // SST Output Object
    // Initialize with 
    // - no prefix ("")
    // - Verbose set to 1
    // - No mask
    // - Output to STDOUT (Output::STDOUT)
    out = new Output("", 1, 0, Output::STDOUT);
    
    out->output("Phase: Construction, %s\n", getName().c_str());

    // Get parameter from the Python input
    bool found;
    eventsToSend = params.find<unsigned>("eventsToSend", 0, found);

    // If parameter wasn't found, end the simulation with exit code -1.
    // Tell the user how to fix the error (set 'eventsToSend' parameter in the input) 
    // and which component generated the error (getName())
    if (!found) {
        out->fatal(CALL_INFO, -1, "Error in %s: the input did not specify 'eventsToSend' parameter\n", getName().c_str());
    }

    // Verbose parameter to control output further
    verbose = params.find<bool>("verbose", false);

    // configure our links with a callback function that will be called whenever an event arrives
    leftLink = configureLink("left", new Event::Handler<basicSimLifeCycle>(this, &basicSimLifeCycle::handleEvent));
    rightLink = configureLink("right", new Event::Handler<basicSimLifeCycle>(this, &basicSimLifeCycle::handleEvent));

    // Make sure we successfully configured the links
    // Failure usually means the user didn't connect the port in the input file
    sst_assert(leftLink, CALL_INFO, -1, "Error in %s: Left link configuration failed\n", getName().c_str());
    sst_assert(rightLink, CALL_INFO, -1, "Error in %s: Right link configuration failed\n", getName().c_str());

    // Register as primary and prevent simulation end until we've received all the events we need
    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

    // Initialize our variables that count events
    eventsReceived = 0;
    eventsForwarded = 0;
    eventsSent = 0;
}

/*
 * Lifecycle Phase #2: Init
 * Components in the ring will discover their neighbors names and agree to use the 
 * average number of eventsToSend from each neighbor. 
 * 
 * - Send our eventsToSend parameter and name around the ring
 * - Record information from other components
 * - During init(), the 'sendUntimedData' functions must be used instead of 'send'
 */
void basicSimLifeCycle::init(unsigned phase) 
{
    out->output("Phase: Init(%u), %s\n", phase, getName().c_str());
    
    // Only send our info on phase 0
    if (phase == 0) {
        basicLifeCycleEvent* event = new basicLifeCycleEvent(getName(), eventsToSend);
        leftLink->sendUntimedData(event);
    }

    // Check if an event is received. recvUntimedData returns nullptr if no event is available
    while (SST::Event* ev = rightLink->recvUntimedData()) {

        basicLifeCycleEvent* event = dynamic_cast<basicLifeCycleEvent*>(ev);
        if (event) {
            if (verbose)
                out->output("    %" PRIu64 " %s received %s\n", getCurrentSimCycle(), getName().c_str(), event->toString().c_str());

            if (event->getStr() == getName()) { // Event made it around the ring and back to this component
                delete event;
            } else { // Event is from another component
                neighbors.insert(event->getStr());
                eventsToSend += event->getNum();
                leftLink->sendUntimedData(event);
            }

        } else {
            out->fatal(CALL_INFO, -1, "Error in %s: Received an event during init() but it is not the expected type\n", getName().c_str());
        }
    }
}

/*
 * Lifecycle Phase #3: Setup
 * - Send first event
 *   This send should use the link->send function since it is intended for simulation rather than 
 *   pre-simulation (init) or post-simulation (complete)
 * - Initialize the 'iter' variable to point to next component to send to in eventRequests map
 */
void basicSimLifeCycle::setup() 
{
    out->output("Phase: Setup, %s\n", getName().c_str());
    
    // Use the average of each components' eventsToSend parameter to agree on eventsToSend
    // Then, total events to send during simulation is our neighbors * events to each
    eventsToSend /= (neighbors.size() + 1); // Plus one since I am not in the neighbor list

    out->output("    %s will send %u events to each other component.\n", getName().c_str(), eventsToSend);

    eventsToSend *= neighbors.size(); // Total number of events to send

    // Sanity check
    if (neighbors.empty()) {
        out->output("    My name is %s. It's very lonely here, I have no neighbors.\n", getName().c_str());
        primaryComponentOKToEndSim();
        return;
    } else if (eventsToSend == 0) {
        out->output("    My name is %s. I have neighbors but none of us want to talk.\n", getName().c_str());
        primaryComponentOKToEndSim();
        return;
    }

    // Since all the sets are ordered the same, stagger our starting neighbor to be the one after us
    iter = neighbors.upper_bound(getName());
    if (iter == neighbors.end()) iter = neighbors.begin();

    // Send first event
    leftLink->send(new basicLifeCycleEvent(*iter));
    
    // Record that we sent this event
    eventsSent++;

    // Update iter to prepare for next send
    iter++;
    if (iter == neighbors.end()) iter = neighbors.begin();

    out->output("Phase: Run, %s\n", getName().c_str());
}


/* 
 * Lifecycle Phase #4: Run
 *
 * During the run phase, SST will call this event handler anytime an
 * event is received on a link
 * - Record that we received an event (eventsForwarded or eventsReceived)
 * - Delete event if it was intended for us
 * - Forward event if it was intended for someone else
 * - Send a new event
 */
void basicSimLifeCycle::handleEvent(SST::Event *ev)
{
    basicLifeCycleEvent *event = dynamic_cast<basicLifeCycleEvent*>(ev);
    
    if (event) {
        if (verbose) out->output("    %" PRIu64 " %s received %s\n", getCurrentSimCycle(), getName().c_str(), event->toString().c_str());
       
        // Determine whether event is intended for us and handle accordingly
        if (event->getStr() == getName()) {
            eventsReceived++;
            delete event;
            
            if (eventsReceived == eventsToSend) 
                primaryComponentOKToEndSim();

            // Send a new event if we need to
            if (eventsSent != eventsToSend) {
                leftLink->send(new basicLifeCycleEvent(*iter));
                eventsSent++;
                iter++;
                if (iter == neighbors.end()) iter = neighbors.begin();
            } 

        } else {
            /* Otherwise, event wasn't for us. Forward it. */
            eventsForwarded++;
            leftLink->send(event);
        }
        
    } else {
        out->fatal(CALL_INFO, -1, "Error in %s: Received an event during simulation but it is not the expected type\n", getName().c_str());
    }
}

/*
 * Lifecycle Phase #5: Complete
 *
 * During the complete phase, say goodbye to our left neighbor and farewell to our right
 */
void basicSimLifeCycle::complete(unsigned phase)
{
    out->output("Phase: Complete(%u), %s\n", phase, getName().c_str());
    
    if (phase == 0) {
        std::string goodbye = "Goodbye from " + getName();
        std::string farewell = "Farewell from " + getName();
        leftLink->sendUntimedData( new basicLifeCycleEvent(goodbye) );
        rightLink->sendUntimedData( new basicLifeCycleEvent(farewell) );
    }

    // Check for an event on the left link
    while (SST::Event* ev = leftLink->recvUntimedData()) {
        basicLifeCycleEvent* event = dynamic_cast<basicLifeCycleEvent*>(ev);
        if (event) {
            if (verbose) out->output("    %" PRIu64 " %s received %s\n", getCurrentSimCycle(), getName().c_str(), event->toString().c_str());
            leftMsg = event->getStr();
            delete event;
        } else {
            out->fatal(CALL_INFO, -1, "Error in %s: Received an event during complete() but it is not the expected type\n", getName().c_str());
        }
    }

    // Check for an event on the right link
    while (SST::Event* ev = rightLink->recvUntimedData()) {
        basicLifeCycleEvent* event = dynamic_cast<basicLifeCycleEvent*>(ev);
        if (event) {
            if (verbose) out->output("    %" PRIu64 " %s received %s\n", getCurrentSimCycle(), getName().c_str(), event->toString().c_str());
            rightMsg = event->getStr();
            delete event;
        } else {
            out->fatal(CALL_INFO, -1, "Error in %s: Received an event during complete() but it is not the expected type\n", getName().c_str());
        }
    }
}

/*
 * Lifecycle Phase #6: Finish
 * During the finish() phase, output the info that we received during complete()
 */
void basicSimLifeCycle::finish() 
{
    out->output("Phase: Finish, %s\n", getName().c_str());
    
    out->output("    My name is %s and I sent %u messages. I received %u messages and forwarded %u messages.\n"
            "    My left neighbor said: %s\n"
            "    My right neighbor said: %s\n",
            getName().c_str(), eventsSent, eventsReceived, eventsForwarded,
            leftMsg.c_str(), rightMsg.c_str());
}

/*
 * Lifecycle Phase #7: Destruction
 * Clean up our output object
 * SST will delete links
 */
basicSimLifeCycle::~basicSimLifeCycle()
{
    // Component info has been deleted by core already so getName() won't work
    out->output("Phase: Destruction\n");
    delete out;
}

/*
 * Emergency Shutdown
 * Try sending the simulation a SIGTERM
 */
void basicSimLifeCycle::emergencyShutdown() {
    out->output("Uh-oh, my name is %s and I have to quit. I sent %u messages.\n", getName().c_str(), eventsSent);
}

/* 
 * PrintStatus
 * Try sending the simulation a SIGUSR2
 */
void basicSimLifeCycle::printStatus(Output& sim_out) {
    sim_out.output("%s reporting. I have sent %u messages, received %u, and forwarded %u.\n", 
            getName().c_str(), eventsSent, eventsReceived, eventsForwarded);
}
