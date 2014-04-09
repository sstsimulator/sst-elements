//
// Copyright (c) 2011, IBM Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


/*



*/
#include <sst_config.h>
#include <stdio.h>
#include "state_machine.h"



// Register a state machine and a handler for events
// Assign a SM number. Make the last created SM the current one.
int
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
// We queue the return_event, which is the event we will send to the
// currently running state machine when we return to it.
// Then we switch to the requested SM and call it with the start event
// If a caller wants to pass parameters to a state machine, it can do so
// in the start_event.
void
State_machine::SM_call(int machineID, state_event start_event, state_event return_event)
{

    if (machineID > lastSM)   {
	// FIXME: We should abort here
	fprintf(stderr, "[%3d] Transition from SM %d to non-exisiting %d illegal!\n",
	    my_rank, currentSM, machineID);
    }

    return_event.restart= true;
    return_event.tag= SM_current_tag();
    SM[currentSM].missed_events.push_back(return_event);

    SMstack.push_back(currentSM);
    currentSM= machineID;

    // Call the handler of the new SM with its start event
    (*SM[currentSM].handler)(SM[currentSM].obj, start_event);

    // There may be events for this SM already pending that should be
    // delivered now that the new SM has been activated.
    deliver_missed_events();

}  // end of SM_call()



void
State_machine::SM_return(state_event return_event)
{

state_event restart_event;


    if (SMstack.empty())   {
	_sm_abort(State_machine, "[%3d] SM stack is empty!\n", my_rank);
    }
    currentSM= SMstack.back();
    SMstack.pop_back();

    if (SM[currentSM].missed_events.empty())   {
	// There has to be at least the return event pending!
	_sm_abort(State_machine, "[%3d] No events to return!\n", my_rank);
    }

    restart_event= (SM[currentSM].missed_events.front());
    assert(restart_event.tag == SM_current_tag());
    SM[currentSM].missed_events.pop_front();
    // This should be marked as a restart_event, otherwise something went wrong
    if (!restart_event.restart)   {
	_sm_abort(State_machine, "[%3d] Not a restart event!\n", my_rank);
    }


    // Copy the return data to the restart event
    restart_event.set_Fdata(return_event.get_Fdata(0), return_event.get_Fdata(1));
    restart_event.set_Idata(return_event.get_Idata(0), return_event.get_Idata(1));

    // If the below is not true, we have to copy more fields above
    assert(SM_MAX_DATA_FIELDS == 2);

    // Call back up
    (*SM[currentSM].handler)(SM[currentSM].obj, restart_event);

    // Now delivery any other events that might be pending.
    deliver_missed_events();

}  // end of SM_return()



int
State_machine::SM_current_tag(void)
{

    assert(SM[currentSM].tag >= 0);
    return SM[currentSM].tag;

}  // end of SM_current_tag()



//
// Private functions
//
void
State_machine::handle_state_events(int tag, state_event e)
{

bool switched;


    assert(tag >= 0);
    assert(e.tag >= 0);
    if (tag == SM_current_tag())   {
	// If there are pending events for this SM we have not handled yet, do it now
	switched= deliver_missed_events();
	assert(!switched);

	// Then call the current SM with the event that just arrived
	assert(SM_current_tag() == tag); // otherwise we got switched
	assert(e.tag >= 0);
	(*SM[currentSM].handler)(SM[currentSM].obj, e);

    } else   {
	// Event for a non-running SM. Queue it.
	SM[tag].missed_events.push_back(e);
    }

    return;

}  /* end of handle_state_events() */



bool
State_machine::deliver_missed_events(void)
{

state_event missed_event;
int lastSM;


    // If we just popped back to an SM that has pending events, deliever them now
    lastSM= currentSM;
    while (!SM[currentSM].missed_events.empty())   {
	missed_event= (SM[currentSM].missed_events.front());
	SM[currentSM].missed_events.pop_front();
	assert(missed_event.tag == SM_current_tag());
	assert(missed_event.tag >= 0);
	(*SM[currentSM].handler)(SM[currentSM].obj, missed_event);
	if (lastSM != currentSM)   {
	    // Handler switched us from lastSM to currentSM!
	    return true;
	}
    }

    return false;

}  // end of deliver_missed_events



#ifdef SERIALIZATION_WORKS_NOW
BOOST_CLASS_EXPORT(State_machine)
#endif // SERIALIZATION_WORKS_NOW
