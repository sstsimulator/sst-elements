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
    printf("%s on line %d of file %s got called\n", __FUNCTION__, __LINE__, __FILE__);
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
    common->send(dest, exchange_msg_len + len);
}  // end of data_send



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
