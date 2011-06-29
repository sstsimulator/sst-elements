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

void
Comm_pattern::data_send(int dest, int len, int event_type)
{

uint32_t tag= 0;


    tag= SM->SM_current_tag();
    // FIXME: We need a better model than 2440 for node latency!
    common->send(2440, dest, envelope_size + len, event_type, tag);

}  // end of data_send()



void
Comm_pattern::self_event_send(int event_type)
{

uint32_t tag= 0;


    tag= SM->SM_current_tag();
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
// Messages from the global network
void
Comm_pattern::handle_net_events(Event *sst_event)
{

CPUNicEvent *e;


    e= static_cast<CPUNicEvent *>(sst_event);

    if (e->dest != my_rank)   {
	_abort(comm_pattern, "NETWORK dest %d != my rank %d\n", e->dest, my_rank);
    }

    SM->handle_state_events(e->tag, e->GetRoutine());
    delete(e);

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

    SM->handle_state_events(e->tag, e->GetRoutine());
    delete(e);

}  /* end of handle_NoC_events() */



// When we send to ourselves, we come here.
// Just pass it on to the main handler above
void
Comm_pattern::handle_self_events(Event *sst_event)
{

CPUNicEvent *e;


    e= static_cast<CPUNicEvent *>(sst_event);
    SM->handle_state_events(e->tag, e->GetRoutine());
    delete(e);

}  /* end of handle_self_events() */



// Events from the local NVRAM
void
Comm_pattern::handle_nvram_events(Event *sst_event)
{

CPUNicEvent *e;


    e= static_cast<CPUNicEvent *>(sst_event);
    SM->handle_state_events(e->tag, e->GetRoutine());
    delete(e);

}  /* end of handle_nvram_events() */



// Events from stable storage
void
Comm_pattern::handle_storage_events(Event *sst_event)
{

CPUNicEvent *e;


    e= static_cast<CPUNicEvent *>(sst_event);
    SM->handle_state_events(e->tag, e->GetRoutine());
    delete(e);

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
