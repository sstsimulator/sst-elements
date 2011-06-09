//
// Copyright (c) 2011, IBM Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <sst_config.h>
#include "sst/core/serialization/element.h"
#include <sst/core/cpunicEvent.h>
#include <assert.h>
#include "comm_pattern.h"


//
// Public functions
//

// Register a state machine and a handler for events
void
Comm_pattern::register_app_pattern(Event::HandlerBase* handler)
{
    pingpong_handler= handler;
}  // end of register_app_pattern



// Transition to another state machine
void
Comm_pattern::SM_transition(int machineID)
{
}  // end of SM_transition


void
Comm_pattern::data_send(int dest, int len)
{
    common->send(2440, dest, envelope_size + len);
}  // end of data_send



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

    // FIXME: For now. Later we need to find the currently running state machine
    // and call the appropriate handler
    (*pingpong_handler)(e);

    delete(e);
    // unregisterExit();

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
