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

#include "example1.h"
#include "basicEvent.h"


using namespace SST;
using namespace SST::simpleElementExample;

/* 
 * During construction the example component should prepare for simulation
 * - Read parameters
 * - Configure link
 * - Register its clock
 * - Initialize the RNG
 * - Register statistics
 */
example1::example1(ComponentId_t id, Params& params) : Component(id) {

    // SST Output Object
    // Initialize with 
    // - no prefix ("")
    // - Verbose set to 1
    // - No mask
    // - Output to STDOUT (Output::STDOUT)
    out = new Output("", 1, 0, Output::STDOUT);

    // Get parameter from the Python input
    bool found;
    eventsToSend = params.find<int64_t>("eventsToSend", 0, found);

    // If parameter wasn't found, end the simulation with exit code -1.
    // Tell the user how to fix the error (set 'eventsToSend' parameter in the input) 
    // and which component generated the error (getName())
    if (!found) {
        out->fatal(CALL_INFO, -1, "Error in %s: the input did not specify 'eventsToSend' parameter\n", getName().c_str());
    }

    // This parameter controls how big the messages are
    // If the user didn't set it, have the parameter default to 16 (bytes)
    eventSize = params.find<int64_t>("eventSize", 16);

    // Tell the simulation not to end until we're ready
    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

    // configure our link with a callback function that will be called whenever an event arrives
    // Callback function is optional, if not provided then component must poll the link
    link = configureLink("port", new Event::Handler<example1>(this, &example1::handleEvent));

    // Make sure we successfully configured the links
    // Failure usually means the user didn't connect the port in the input file
    sst_assert(link, CALL_INFO, -1, "Error in %s: Link configuration failed\n", getName().c_str());

    //set our clock. The simulator will call 'clockTic' at a 1GHz frequency
    registerClock("1GHz", new Clock::Handler<example1>(this, &example1::clockTic));

    // This simulation will end when we have sent 'eventsToSend' events and received a 'LAST' event
    lastEventReceived = false;

    // Initialize the random number generator
    rng = new SST::RNG::MarsagliaRNG(11, 272727);

    // Register the statistic to link our variable to the documented statistic name
    bytesReceived = registerStatistic<uint64_t>("EventSizeReceived");
}


/*
 * Destructor, clean up our output and RNG
 */
example1::~example1()
{
    delete out;
    delete rng;
}


/* Event handler
 * Incoming events are scanned and deleted
 * Record if the event received is the last one our neighbor will send 
 */
void example1::handleEvent(SST::Event *ev)
{
    basicEvent *event = dynamic_cast<basicEvent*>(ev);
    
    if (event) {
        // Record the size of the payload
        bytesReceived->addData(event->payload.size());
        
        // Check if this is the last event our neighbor will send us
        if (event->last) {
            lastEventReceived = true;
        }
        
        // Receiver has the responsiblity for deleting events
        delete event;

    } else {
        out->fatal(CALL_INFO, -1, "Error! Bad Event Type received by %s!\n", getName().c_str());
    }
}


/* 
 * On each clock cycle we will send an event to our neighbor until we've sent our last event
 * Then we will check for the exit condition and notify the simulator when the simulation is done
 */
bool example1::clockTic( Cycle_t cycleCount)
{
    // Send an event if we need to 
    if (eventsToSend > 0) {
        basicEvent *event = new basicEvent();
        
        // Use the RNG to pick a payload size between 1 and eventSize
        uint32_t size = (rng->generateNextUInt32() % eventSize) + 1;
        // Create a dummy payload with of size bytes
        for (int i = 0; i < size; i++) {
            event->payload.push_back(1);
        }

        // This is the last event we'll send
        if (eventsToSend == 1) {
            event->last = true;
        }
        
        eventsToSend--;

        // Send the event
        link->send(event);
    }

    // Check if the exit conditions are met
    if (eventsToSend == 0 && lastEventReceived == true) {
        
        // Tell SST that it's OK to end the simulation (once all primary components agree, simulation will end)
        primaryComponentOKToEndSim(); 

        // Retrun true to indicate that this clock handler should be disabled
        return true;
    }

    // Return false to indicate the clock handler should not be disabled
    return false;
}
