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

#include "basicLinks.h"
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
basicLinks::basicLinks(ComponentId_t id, Params& params) : Component(id) {

    // SST Output Object
    // Initialize with 
    // - no prefix ("")
    // - Verbose set to 1
    // - No mask
    // - Output to STDOUT (Output::STDOUT)
    out = new Output("", 1, 0, Output::STDOUT);

    // Get parameters from the Python input
    bool found;
    eventsToSend = params.find<int64_t>("eventsToSend", 0, found);

    // If parameter wasn't found, end the simulation with exit code -1.
    // Tell the user what went wrong (didn't find 'eventsToSend' parameter in the input) 
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

    //set our clock. The simulator will call 'clockTic' at a 1GHz frequency
    registerClock("1GHz", new Clock::Handler<basicLinks>(this, &basicLinks::clockTic));

    // This simulation will end when we have sent 'eventsToSend' events and received a 'LAST' event on every link
    lastEventReceived = 0;

    // Initialize the random number generator
    rng = new SST::RNG::MarsagliaRNG(11, 272727);

    
    /*
     *  Links for this example
     */
    
    // 1. These links share
    linkHandler = configureLink("port_handler", new Event::Handler<basicLinks>(this, &basicLinks::handleEvent));
    sst_assert(linkHandler, CALL_INFO, -1, "Error in %s: Link configuration for 'port_handler' failed\n", getName().c_str());


    // 2. Links that share an event handler.
    // We optionally specify an ID so we can determine which link an event arrived on
    // Additionally, this demonstrates a variable number of ports. See SST_ELI_DOCUMENT_PORTS for how to specify this kind of a port
    std::string linkprefix = "port_vector";
    std::string linkname = linkprefix + "0";
    int portnum = 0;
    while (isPortConnected(linkname)) {
        SST::Link* link = configureLink(linkname, new Event::Handler<basicLinks, int>(this, &basicLinks::handleEventWithID, portnum));
        
        if (!link)
            out->fatal(CALL_INFO, -1, "Error in %s: unable to configure link %s\n", getName().c_str(), linkname.c_str());

        linkVector.push_back(link);
        
        // Build the next name to check
        portnum++;
        linkname = linkprefix + std::to_string(portnum);
    }

    // Report how many links we found.
    out->output("Component '%s' found %zu links for the vector of links\n", getName().c_str(), linkVector.size());

    // 3. Polling link
    linkPolled = configureLink("port_polled");
    sst_assert(linkPolled, CALL_INFO, -1, "Error in %s: Link configuration for 'port_polled' failed\n", getName().c_str());


    /*
     *  Statistics for this example
     *  We will count bytes received on each link
     */
    
    // Count bytes received on the port_handler link
    stat_portHandler = registerStatistic<uint64_t>("EventSize_PortHandler");

    // Count bytes received on each port_shr link
    // This version of registering statistics registers one statistic name with multiple unique "SubID" strings (index in this case)
    // Each SubID is counted separately
    for (size_t i = 0; i < linkVector.size(); i++) {
        Statistic<uint64_t>* stat = registerStatistic<uint64_t>("EventSize_PortVector", std::to_string(i));
        stat_portVector.push_back(stat);
    }

    // Count bytes received on the port_polled link
    stat_portPolled = registerStatistic<uint64_t>("EventSize_PortPolled");

}


/*
 * Destructor, clean up our output and RNG
 */
basicLinks::~basicLinks()
{
    delete out;
    delete rng;
}


/* 
 * Event handler with no ID
 * If multiple ports used this handler, we would not be able to differentiate
 * events that arrived on different links
 */
void basicLinks::handleEvent(SST::Event *ev)
{
    basicEvent *event = dynamic_cast<basicEvent*>(ev);
    
    if (event) {

        // Check if this is the last event our neighbor will send us
        if (event->last) {
            lastEventReceived++;
        } else {
            // Record the size of the payload
            stat_portHandler->addData(event->payload.size());
        }

        // Receiver has the responsiblity for deleting events
        delete event;

    } else {
        out->fatal(CALL_INFO, -1, "Error! Bad Event Type received by %s!\n", getName().c_str());
    }
}

/*
 * Event handler with ID
 * The 'id' passed in to this function is the constant that was registered
 * when we create the EventHandler in the constructor
 */
void basicLinks::handleEventWithID(SST::Event *ev, int id) {

    basicEvent *event = dynamic_cast<basicEvent*>(ev);
 
    if (event) {
        
        if (event->last) {
            lastEventReceived++;
        } else {
            // Record the size of the payload
            stat_portVector[id]->addData(event->payload.size());
        }

        delete event;
    } else {
        out->fatal(CALL_INFO, -1, "Error! Bad Event Type received by %s on link ID %d\n", getName().c_str(), id);
    }
}


/* 
 * Clock does three things:
 * 1. Send an event on a random link until we've sent a certain number. Then send a "LAST" event on every link.
 * 2. Poll the polled link to check for received events
 * 3. Check the exit condition and notify the simulator when this component is finished
 */
bool basicLinks::clockTic( Cycle_t cycleCount)
{
    // Send an event if we need to 
    if (eventsToSend > 0) {
        basicEvent *event = new basicEvent();
        
        // Use the RNG to pick a payload size between 1 and eventSize
        uint32_t size = 1 + (rng->generateNextUInt32() % eventSize);

        // Create a dummy payload with of size bytes
        for (int i = 0; i < size; i++) {
            event->payload.push_back(1);
        }
        
        eventsToSend--;

        // Use the RNG to pick a destination
        uint32_t dest = (rng->generateNextUInt32() % (2 + linkVector.size()));
        if (dest == 0) {
            linkHandler->send(event);
        } else if (dest == 1) {
            linkPolled->send(event);
        } else {
            linkVector[dest - 2]->send(event);
        }
        
        // This is the last event we'll send
        // Notify our neighbor(s) that we're done
        if (eventsToSend == 0) {
            for (int i = 0; i < linkVector.size(); i++) {
                basicEvent *event = new basicEvent();
                event->last = true;

                linkVector[i]->send(event);
            }

            basicEvent* event0 = new basicEvent();
            event0->last = true;
            linkHandler->send(event0);

            basicEvent* event1 = new basicEvent();
            event1->last = true;
            linkPolled->send(event1);
        }
    }


    // Check if the polling link has an event to receive on it
    while (SST::Event* ev = linkPolled->recv()) {
        // Static cast is faster for casts that happen many times but higher potential for errors
        basicEvent* event = static_cast<basicEvent*>(ev);
        
        // Check if this is the last event our neighbor will send us
        if (event->last) {
            lastEventReceived++;
        } else {
            // Record the size of the payload
            stat_portPolled->addData(event->payload.size());
        }
        
        // Receiver has the responsiblity for deleting events
        delete event;
    }


    // Check if the exit conditions are met
    // 1. We've sent all of our events
    // 2. We've received every event we expect to
    if (eventsToSend == 0 && lastEventReceived == (2 + linkVector.size())) {
        
        // Tell SST that it's OK to end the simulation (once all primary components agree, simulation will end)
        primaryComponentOKToEndSim(); 

        // Return true to indicate that this clock handler should be disabled
        return true;
    }

    // Return false to indicate the clock handler should not be disabled
    return false;
}
