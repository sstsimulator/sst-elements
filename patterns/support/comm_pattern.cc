//
// Copyright (c) 2011, IBM Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <sst_config.h>
#include <inttypes.h>		// For PRIx64

#include "comm_pattern.h"

#include <assert.h>

#include <sst/core/element.h>

#include "state_machine.h"


//
// Public functions
//

// Send an event that represents a message of length len to destination dest.
// No actual data is contained in this message. There is some out of band data
// that is contained in the sm_event. It's packed into the payload and sent along.
//
// This behaves kind of like an MPI_Isend() When it returns, the message may not have
// left the node yet.
void
Comm_pattern::send_msg(int dest, int len, state_event sm_event)
{

    common->event_send(dest, sm_event.event,
	SM->SM_current_tag(), len,
	(const char *)sm_event.payload,
	sm_event.payload_size);

}  // end of send_msg()



// This version sends a completion event of type "blocking" back to
// ourselves at the time the message leaves the NIC.
void
Comm_pattern::send_msg(int dest, int len, state_event sm_event, int blocking)
{

    common->event_send(dest, sm_event.event,
	SM->SM_current_tag(), len,
	(const char *)sm_event.payload,
	sm_event.payload_size, blocking);

}  // end of send_msg()



void
Comm_pattern::self_event_send(int event_type, SimTime_t duration)
{

int tag;


    tag= SM->SM_current_tag();
    common->self_event_send(event_type, tag, duration);

}  /* end of self_event_send() */



// Pretend to do some work and then send an event
void
Comm_pattern::local_compute(int done_event, SimTime_t duration)
{

int tag;


    tag= SM->SM_current_tag();
    common->self_event_send(done_event, tag, duration);

}  /* end of local_compute() */



// Pretend to do a memcpy
void
Comm_pattern::memcpy(int done_event, int bytes)
{

SimTime_t duration;
int tag;


    // FIXME: Read this value out of of the XML file
    // duration= common->memdelay(bytes);
    duration= (SimTime_t)(0.000000000428 * bytes * 1000000000.0);
    tag= SM->SM_current_tag();
    common->self_event_send(done_event, tag, duration);

}  // end of memcpy()



// Pretend to do a vector operation, such as an add or max on a
// number of double words.
void
Comm_pattern::vector_op(int done_event, int doubles)
{

SimTime_t duration;
int tag;


    // FIXME: Read this value out of of the XML file
    duration= (SimTime_t)(4.0 * doubles);
    tag= SM->SM_current_tag();
    common->self_event_send(done_event, tag, duration);

}  // end of vector_op()



double
Comm_pattern::SimTimeToD(SimTime_t t)
{
    return (double)t / TIME_BASE_FACTOR;
}  /* end of SimTimeToD() */



//
// A couple of bit-twidling utility functions we need
// Algorithms from http://graphics.stanford.edu/~seander/bithacks.html
//

// Find the next higher power of 2 for v
uint32_t
Comm_pattern::next_power2(uint32_t v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;

    return v;

}  // end of next_power2()



bool
Comm_pattern::is_pow2(int num)
{
    if (num < 1)   {
	return false;
    }

    if ((num & (num - 1)) == 0)   {
	return true;
    }

    return false;

}  // end of is_pow2()



//
// Private functions
//
void
Comm_pattern::handle_sst_events(CPUNicEvent *sst_event, const char *err_str)
{

state_event sm;
int payload_len;


    if (sst_event->dest != my_rank)   {
	out.fatal(CALL_INFO, -1, "%s dest %d != my rank %d. Msg ID 0x%" PRIx64 "\n",
	    err_str, sst_event->dest, my_rank, sst_event->msg_id);
    }

    sm.event= sst_event->GetRoutine();
    assert(sst_event->tag >= 0);
    sm.tag= sst_event->tag;
    payload_len= sst_event->GetPayloadLen();
    if (payload_len > 0)   {
	sst_event->DetachPayload(sm.payload, &payload_len);
    }

    SM->handle_state_events(sst_event->tag, sm);
    delete(sst_event);

}  /* end of handle_sst_events() */



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



/*
// THIS SECTION MOVED TO patterns.cc FOR RELEASE 3.x OF SST - ALEVINE

eli(Comm_pattern, comm_pattern, "Communication pattern base object")
*/

#ifdef SERIALIZATION_WORKS_NOW
BOOST_CLASS_EXPORT(Comm_pattern)
#endif // SERIALIZATION_WORKS_NOW
