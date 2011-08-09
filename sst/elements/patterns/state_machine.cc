//
// Copyright (c) 2011, IBM Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


/*



*/
#include <stdio.h>
#include "state_machine.h"



// Register a state machine and a handler for events
// Assign a SM number. Make the last created SM the current one.
uint32_t
State_machine::SM_create(void *obj, void (*handler)(void *obj, state_event event))
{

SM_t sm;
int ID;


    // Store it and use it's position in the list as ID
    ID= SM.size();
    sm.handler= handler;
    sm.obj= obj;
    sm.tag= ID;
    SM.push_back(sm);
    lastSM= ID;
    currentSM= ID;

    return ID;

}  // end of SM_create()



// Transition to another state machine
// We queue the exit_event, which is the event we will send to the
// current running state machine when we return to it.
// The we switch to the request SM and call it with a start event
// All Sm should make their start events 0
void
State_machine::SM_call(int machineID, int exit_event)
{

state_event e;


    if (machineID > lastSM)   {
	// FIXME: We should abort here
	fprintf(stderr, "[%3d] Transition from SM %d to non-exisiting %d illegal!\n",
	    my_rank, currentSM, machineID);
    }

    e.event= exit_event;
    SM[currentSM].missed_events.push_back(e);

    SMstack.push_back(currentSM);
    currentSM= machineID;

    // Call the handler of the new SM with its start event
    e.event= SM_START_EVENT;
    (*SM[currentSM].handler)(SM[currentSM].obj, e);

    // There may be events for this SM already pending
    deliver_missed_events();

}  // end of SM_call()



void
State_machine::SM_return(void)
{

    if (SMstack.empty())   {
	// FIXME:
	// _abort(State_machine, "[%3d] SM stack is empty!\n", my_rank);
    }
    currentSM= SMstack.back();
    SMstack.pop_back();
    deliver_missed_events();

}  // end of SM_return()



uint32_t
State_machine::SM_current_tag(void)
{

    return SM[currentSM].tag;

}  // end of SM_current_tag()



//
// Private functions
//
void
State_machine::handle_state_events(uint32_t tag, state_event e)
{

    if (tag == SM_current_tag())   {
	// If there are pending events for this SM we have not handled yet, do it now
	deliver_missed_events();

	// Then call the current SM with the event that just arrived
	(*SM[currentSM].handler)(SM[currentSM].obj, e);

    } else   {
	// Event for a non-running SM. Queue it.
	SM[tag].missed_events.push_back(e);
    }

    return;

}  /* end of handle_state_events() */



void
State_machine::deliver_missed_events(void)
{

state_event missed_event;
int lastSM;


    // If we just popped back to an SM that has pending events, deliever them now
    lastSM= currentSM;
    while (!SM[currentSM].missed_events.empty())   {
	missed_event= (SM[currentSM].missed_events.front());
	(*SM[currentSM].handler)(SM[currentSM].obj, missed_event);
	SM[lastSM].missed_events.pop_front();
	if (lastSM != currentSM)   {
	    // Handler switched us from lastSM to currentSM!
	    break;
	}
    }

}  // end of deliver_missed_events
