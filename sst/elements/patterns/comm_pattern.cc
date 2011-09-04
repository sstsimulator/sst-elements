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
#include "state_machine.h"
#include "comm_pattern.h"


//
// Public functions
//

// Send an event that represents a message of length len to destination dest.
// No actual data is contained in this message. There is some out of band data
// that is contained in the sm_event. It's packed into the payload and sent along.
//
// This behave kind of like an MPI_Isend() When it returns, the message may not have
// left the node yet. Right now it never blocks == infinite buffer space.
void
Comm_pattern::send_msg(int dest, int len, state_event sm_event)
{

// FIXME: We need a better model than 2440 for node latency!
SimTime_t node_latency= 2440;


    common->event_send(dest, (pattern_event_t)sm_event.event,
	SM->SM_current_tag(),
	node_latency, len,
	(const char *)sm_event.payload,
	sm_event.payload_size);

}  // end of send_msg()



// This is only used for the state_transition macro.
void
Comm_pattern::self_event_send(int event_type)
{

uint32_t tag= 0;


    tag= SM->SM_current_tag();
    // FIXME: We need a more generic event send so the cast can go away
    common->event_send(my_rank, (pattern_event_t)event_type, tag, 0.0);

}  // end of self_event_send()



// This is mesg router X and doesn't take nodes into account!
int
Comm_pattern::myNetX(void)
{
    return (my_rank % (x_dim * NoC_x_dim * cores * nodes)) /
	(NoC_x_dim * NoC_y_dim * cores * nodes);
}  // end of myNetX()



// This is mesg router Y and doesn't take nodes into account!
int
Comm_pattern::myNetY(void)
{
    return my_rank / (x_dim * NoC_x_dim * NoC_y_dim * cores * nodes);
}  // end of myNetY()



int
Comm_pattern::myNoCX(void)
{
    return (my_rank % (NoC_x_dim * NoC_y_dim * cores * nodes)) % NoC_x_dim;
}  // end of myNoCX()



int
Comm_pattern::myNoCY(void)
{
    return (my_rank % (NoC_x_dim * NoC_y_dim * cores * nodes)) / (NoC_x_dim * cores);
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



// Cores per NoC router
int
Comm_pattern::NumCores(void)
{
    return cores;
}  // end of NumCores()



//
// Private functions
//
void
Comm_pattern::handle_sst_events(CPUNicEvent *sst_event, const char *err_str)
{

state_event sm;
int payload_len;


    if (sst_event->dest != my_rank)   {
	_abort(comm_pattern, "%s dest %d != my rank %d. Msg ID 0x%08lx\n",
	    err_str, sst_event->dest, my_rank, sst_event->msg_id);
    }

    sm.event= sst_event->GetRoutine();
    payload_len= sst_event->GetPayloadLen();
    if (payload_len > 0)   {
	sst_event->DetachPayload(sm.payload, &payload_len);
    }

    SM->handle_state_events(sst_event->tag, sm);

}  /* end of handle_sst_events() */



// Messages from the global network
void
Comm_pattern::handle_net_events(Event *sst_event)
{

    handle_sst_events((CPUNicEvent *)(sst_event), "NETWORK");

}  /* end of handle_net_events() */



// Messages from the local chip network
void
Comm_pattern::handle_NoC_events(Event *sst_event)
{

    handle_sst_events((CPUNicEvent *)(sst_event), "NoC");

}  /* end of handle_NoC_events() */



// When we send to ourselves, we come here.
// Just pass it on to the main handler above
void
Comm_pattern::handle_self_events(Event *sst_event)
{

    handle_sst_events((CPUNicEvent *)(sst_event), "SELF");

}  /* end of handle_self_events() */



// Events from the local NVRAM
void
Comm_pattern::handle_nvram_events(Event *sst_event)
{

    handle_sst_events((CPUNicEvent *)(sst_event), "NVRAM");

}  /* end of handle_nvram_events() */



// Events from stable storage
void
Comm_pattern::handle_storage_events(Event *sst_event)
{

    handle_sst_events((CPUNicEvent *)(sst_event), "STORAGE");

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
