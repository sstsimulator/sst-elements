//
// Copyright (c) 2011, IBM Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <sst_config.h>
#include "sst/core/serialization/element.h"
#include <assert.h>
#include "comm_pattern.h"


//
// Public functions
//

// Register a state machine and a handler for events
// Assign a SM number. Make the last created SM the current one.
uint32_t
Comm_pattern::SM_create(void *obj, void (*handler)(void *obj, int event))
{

SM_t sm;
int ID;


    // Store it and use it's position in the list as ID
    ID= SM.size();
    sm.handler= handler;
    sm.obj= obj;
    sm.tag= ID;
    SM.push_back(sm);
    maxSM= ID;
    currentSM= ID;

    return ID;

}  // end of SM_create()



// Transition to another state machine
// The next arriving event will be delivered to state machine machineID
void
Comm_pattern::SM_transition(uint32_t machineID, int start_event)
{
    if (machineID > maxSM)   {
	_abort(Comm_pattern, "[%3d] illegal machine ID %d in %s, %s:%d\n",
	    my_rank, machineID, __FILE__, __FUNCTION__, __LINE__);
    }

    SMstack.push_back(currentSM);
    currentSM= machineID;

    self_event_send(start_event);

    // If we just switched to an SM that has pending events, deliever them now
    while (!SM[currentSM].missed_events.empty())   {
	int missed_event;
	missed_event= (int)(SM[currentSM].missed_events.back()->GetRoutine());
	(*SM[currentSM].handler)(SM[currentSM].obj, missed_event);
	delete(SM[currentSM].missed_events.back());
	SM[currentSM].missed_events.pop_back();
    }

}  // end of SM_transition()



void
Comm_pattern::SM_return(void)
{

    if (SMstack.empty())   {
	_abort(Comm_pattern, "[%3d] SM stack is empty!\n", my_rank);
    }
    currentSM= SMstack.back();
    SMstack.pop_back();

    // If we just popped back to an SM that has pending events, deliever them now
    while (!SM[currentSM].missed_events.empty())   {
	int missed_event;
	missed_event= (int)(SM[currentSM].missed_events.back()->GetRoutine());
	(*SM[currentSM].handler)(SM[currentSM].obj, missed_event);
	delete(SM[currentSM].missed_events.back());
	SM[currentSM].missed_events.pop_back();
    }

}  // end of SM_return()



void
Comm_pattern::data_send(int dest, int len, int event_type)
{

uint32_t tag= 0;


    tag= SM[currentSM].tag;
    // FIXME: We need a better model than 2440 for node latency!
    common->send(2440, dest, envelope_size + len, event_type, tag);

}  // end of data_send()



void
Comm_pattern::self_event_send(int event_type)
{

uint32_t tag= 0;


    tag= SM[currentSM].tag;
    // FIXME: We need a more generic event send so the cast can go away
    common->event_send(my_rank, (pattern_event_t)event_type, tag, 0.0);

}  // end of self_event_send()



int
Comm_pattern::myNetX(void)
{
    return (my_rank % (x_dim * NoC_x_dim * cores)) / (NoC_x_dim * NoC_y_dim * cores);
}  // end of myNetX()



int
Comm_pattern::myNetY(void)
{
    return my_rank / (x_dim * NoC_x_dim * NoC_y_dim * cores);
}  // end of myNetY()



int
Comm_pattern::myNoCX(void)
{
    return (my_rank % (NoC_x_dim * NoC_y_dim * cores)) % NoC_x_dim;
}  // end of myNoCX()



int
Comm_pattern::myNoCY(void)
{
    return (my_rank % (NoC_x_dim * NoC_y_dim * cores)) / (NoC_x_dim * cores);
}  // end of myNoCY()



int
Comm_pattern::NetWidth(void)
{
    return x_dim;
}  // end of NetWidth()



int
Comm_pattern::NetHeight(void)
{
    return y_dim;
}  // end of NetHeight()



int
Comm_pattern::NoCWidth(void)
{
    return NoC_x_dim;
}  // end of NoCWidth()



int
Comm_pattern::NoCHeight(void)
{
    return NoC_y_dim;
}  // end of NoCHeight()



// Cores per router
int
Comm_pattern::NumCores(void)
{
    return cores;
}  // end of NumCores()



//
// Private functions
//
void
Comm_pattern::handle_events(CPUNicEvent *e)
{

uint32_t tag;


    tag= e->tag;

    if (tag == SM[currentSM].tag)   {
	// If there are pending events for this SM we have not handled yet, do it now
	while (!SM[currentSM].missed_events.empty())   {
	    int missed_event;
	    missed_event= (int)(SM[currentSM].missed_events.back()->GetRoutine());
	    (*SM[currentSM].handler)(SM[currentSM].obj, missed_event);
	    delete(SM[currentSM].missed_events.back());
	    SM[currentSM].missed_events.pop_back();
	}

	// Currently running SM. Call it.
	int event= (int)e->GetRoutine();
	(*SM[currentSM].handler)(SM[currentSM].obj, event);
	delete(e);

    } else   {
	// Event for a non-running SM. Queue it.
	SM[tag].missed_events.push_back(e);
    }

    return;

}  /* end of handle_events() */



// Messages from the global network
void
Comm_pattern::handle_net_events(Event *sst_event)
{

CPUNicEvent *e;


    e= static_cast<CPUNicEvent *>(sst_event);

    if (e->dest != my_rank)   {
	_abort(comm_pattern, "NETWORK dest %d != my rank %d\n", e->dest, my_rank);
    }

    handle_events(e);

}  /* end of handle_net_events() */



// Messages from the local chip network
void
Comm_pattern::handle_NoC_events(Event *sst_event)
{

CPUNicEvent *e;


    e= static_cast<CPUNicEvent *>(sst_event);

    if (e->dest != my_rank)   {
	_abort(comm_pattern, "NoC dest %d != my rank %d\n", e->dest, my_rank);
    }

    handle_events(e);

}  /* end of handle_NoC_events() */



// When we send to ourselves, we come here.
// Just pass it on to the main handler above
void
Comm_pattern::handle_self_events(Event *sst_event)
{

CPUNicEvent *e;


    e= static_cast<CPUNicEvent *>(sst_event);
    handle_events(e);

}  /* end of handle_self_events() */



// Events from the local NVRAM
void
Comm_pattern::handle_nvram_events(Event *sst_event)
{

CPUNicEvent *e;


    e= static_cast<CPUNicEvent *>(sst_event);
    handle_events(e);

}  /* end of handle_nvram_events() */



// Events from stable storage
void
Comm_pattern::handle_storage_events(Event *sst_event)
{

CPUNicEvent *e;


    e= static_cast<CPUNicEvent *>(sst_event);
    handle_events(e);

}  /* end of handle_storage_events() */



extern "C" {
Comm_pattern *
comm_patternAllocComponent(SST::ComponentId_t id,
                          SST::Component::Params_t& params)
{
    return new Comm_pattern(id, params);
}
}

BOOST_CLASS_EXPORT(Comm_pattern)
