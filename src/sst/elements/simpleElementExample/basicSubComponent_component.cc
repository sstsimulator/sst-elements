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

#include "basicSubComponent_component.h"
#include "basicSubComponentEvent.h"

using namespace SST;
using namespace SST::simpleElementExample;

/* 
 * During construction this components parses its parameter, configures its links, and loads its compute unit subcomponent
 */
basicSubComponent_Component::basicSubComponent_Component(ComponentId_t id, Params& params) : Component(id) {

    // SST Output Object
    out = new Output("", 1, 0, Output::STDOUT);

    // Get the parameter, check if it was found or not and if not, return an error
    bool found;
    value = params.find<int>("value", 0, found);
    sst_assert(found, CALL_INFO, -1,
            "Error: The parameter 'value' is a required parameter and was not found in the input configuration\n");

    // Configure our links to call our event handler when an event arrives
    leftLink = configureLink("left", new Event::Handler<basicSubComponent_Component>(this, &basicSubComponent_Component::handleEvent));
    rightLink = configureLink("right", new Event::Handler<basicSubComponent_Component>(this, &basicSubComponent_Component::handleEvent));

    // Check that the links were configured correctly
    sst_assert(leftLink, CALL_INFO, -1, 
            "Error: Component %s has an incorrectly configured link on port 'left'\n", getName().c_str());
    sst_assert(rightLink, CALL_INFO, -1, 
            "Error: Component %s has an incorrectly configured link on port 'right'\n", getName().c_str());
    
    /****** Load a SubComponent in two steps ******/

    // 1. Check with the input configuration to see if the user put a subcomponent in our subcomponent slot
    computeUnit = loadUserSubComponent<basicSubComponentAPI>("compute_unit");

    // 2. If the user didn't put a subcomponent there then we could error or load a default one. 
    // In this case, we'll load a default and let the user know we did that
    if (!computeUnit) {
        out->output("NOTE: No SubComponent was loaded in the \"compute_unit\" slot on %s. Loading a default instead.\n", 
                getName().c_str());

        // Load a default subcomponent, we need to provide the parameters and the subcomponent type since the user didn't
        // Step 1: Create some parameters. We'll use the 'increment' unit which needs the amount to increment by.
        Params params;
        params.insert("amount", "1");

        // Step 2: Load the subcomponent anonymously.
        // Parameter 1: SubComponent to load in SST's lib.name format
        // Parameter 2: Slot to load into
        // Parameter 3: Index in slot to load into
        // Parameter 4: Whether the subcomponent can access our statistics and/or ports
        // Parameter 5: Parameters to pass the SubComponent constructor
        loadAnonymousSubComponent<basicSubComponentAPI>("simpleElementExample.basicSubComponentIncrement",
                "computeUnit", 0, ComponentInfo::SHARE_NONE, params);

        // Error check that the subcomponent did get loaded
        sst_assert(computeUnit, CALL_INFO, -1,
                "Error: %s attempted to load a default subcomponent into its 'compute_unit' slot and something went wrong!\n", 
                getName().c_str());
    }
    
    /****** SubComponent loaded, almost done with construction ******/

    // Tell the simulation not to end until we're ready
    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();
}


/*
 * Destructor, clean up our output
 * We should not clean up our subcomponent - SST will do that
 */
basicSubComponent_Component::~basicSubComponent_Component()
{
    delete out;
}


/* 
 * Setup function to send our event in preparation for simulation
 * Because the simulation has no clocks and SST will exit if no clocks and no events exist in the system,
 * we need to begin the simulation with an event. 
 *
 * This function is called by SST on each component just prior to simulation start 
 * It is *not* called on subcomponents automatically so we should manually call it on subcomponents that need it
 * Ours don't so we'll skip that step
 */
void basicSubComponent_Component::setup() 
{
    // Create the event we'll send
    basicSubComponentEvent* event = new basicSubComponentEvent(getName(), value, std::to_string(value));
    
    // Send our event
    leftLink->send(event); 
}

/*
 * Event handling
 * If we get the event we sent, print the result and tell SST that we're OK if the simulation ends
 * If we get an event from someone else, pass it to our compute unit, update the event, and forward it to the left
 */

void basicSubComponent_Component::handleEvent(SST::Event* ev)
{
    // We'll static_cast since we're certain the event is the right type - a little less safe but a little faster too
    basicSubComponentEvent* event = static_cast<basicSubComponentEvent*>(ev);
    
    if (event->getSender() == getName()) 
    {   /* This is the event this component sent, report out and prepare to end simulation */
        out->output("%s computed: %s = %d\n", getName().c_str(), event->getComputation().c_str(), event->getNumber());
        delete event;

        primaryComponentOKToEndSim();
    } 
    else
    {   /* Event sent by a different component, use the SubComponent to update the computation */
        int updatedNum = computeUnit->compute(event->getNumber());
        std::string updatedComp = computeUnit->compute(event->getComputation());

        /* Update the event */
        event->setNumber(updatedNum);
        event->setComputation(updatedComp);


        /* Forward the event */
        leftLink->send(event);
    }
}
