// Copyright 2009-2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2010, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <sst_config.h>
#include "sst/core/serialization/element.h"
#include <sst/core/cpunicEvent.h>
#include <assert.h>
#include "msgrate_pattern.h"
#include "pattern_common.h"

// It's unrealistic to be able to send without any delay
#define SEND_DELAY	(1000)	// 1 usec


void
Msgrate_pattern::handle_events(CPUNicEvent *e)
{

pattern_event_t event;


    // Extract the pattern event type from the SST event
    // (We are "misusing" the routine filed in CPUNicEvent to xmit the event type
    event= (pattern_event_t)e->GetRoutine();

    if (application_done)   {
	_MSGRATE_PATTERN_DBG(0, "[%3d] There should be no more events! (%d)\n",
	    my_rank, event);
	return;
    }

    _MSGRATE_PATTERN_DBG(2, "[%3d] In state %d and got event %d at time %lu\n", my_rank,
	state, event, getCurrentSimTime());

    switch (state)   {
	case INIT:
	    state_INIT(event);
	    break;
	case SENDING:
	    state_SENDING(event);
	    break;
	case WAITING:
	    state_WAITING(event);
	    break;
	case DONE:
	    state_DONE(event);
	    break;
    }

    delete(e);

    if (application_done)   {
	if (my_rank >= nranks / 2)   {
	    if (rcv_cnt != num_msgs)   {
		printf("ERROR: Rank %d received %d messages instead of %d\n",
		    my_rank, rcv_cnt, num_msgs);
	    }
	    printf(">>> Rank %3d to %3d, %d messages, rate %.3f msgs/s\n",
		my_rank - nranks / 2, my_rank, rcv_cnt,
		1000000000.0 / ((double)msg_wait_time / rcv_cnt));
	}

	unregisterExit();
    }

    return;

}  /* end of handle_events() */



// Messages from the global network
void
Msgrate_pattern::handle_net_events(Event *sst_event)
{

CPUNicEvent *e;


    e= static_cast<CPUNicEvent *>(sst_event);

    if (e->dest != my_rank)   {
	_abort(msgrate_pattern, "NETWORK dest %d != my rank %d\n", e->dest, my_rank);
    }

    handle_events(e);

}  /* end of handle_net_events() */



// Messages from the local chip network
void
Msgrate_pattern::handle_NoC_events(Event *sst_event)
{

CPUNicEvent *e;


    e= static_cast<CPUNicEvent *>(sst_event);

    if (e->dest != my_rank)   {
	_abort(msgrate_pattern, "NoC dest %d != my rank %d\n", e->dest, my_rank);
    }

    handle_events(e);

}  /* end of handle_NoC_events() */



// When we send to ourselves, we come here.
// Just pass it on to the main handler above
void
Msgrate_pattern::handle_self_events(Event *sst_event)
{

CPUNicEvent *e;


    e= static_cast<CPUNicEvent *>(sst_event);
    handle_events(e);

}  /* end of handle_self_events() */



// Events from the local NVRAM
void
Msgrate_pattern::handle_nvram_events(Event *sst_event)
{

CPUNicEvent *e;


    _abort(msgrate_pattern, "We should not get NVRAM events in this pattern!\n");
    e= static_cast<CPUNicEvent *>(sst_event);
    handle_events(e);

}  /* end of handle_nvram_events() */



// Events from stable storage
void
Msgrate_pattern::handle_storage_events(Event *sst_event)
{

CPUNicEvent *e;


    _abort(msgrate_pattern, "We should not get stable storage events in this pattern!\n");
    e= static_cast<CPUNicEvent *>(sst_event);
    handle_events(e);

}  /* end of handle_storage_events() */



extern "C" {
Msgrate_pattern *
msgrate_patternAllocComponent(SST::ComponentId_t id,
                          SST::Component::Params_t& params)
{
    return new Msgrate_pattern(id, params);
}
}

BOOST_CLASS_EXPORT(Msgrate_pattern)



//
// -----------------------------------------------------------------------------
// For each state in the state machine we have a method to deal with incoming
// events.
// INIT --> COMPUTE on START event
void
Msgrate_pattern::state_INIT(pattern_event_t event)
{

    switch (event)   {
	case START:
	    _MSGRATE_PATTERN_DBG(4, "[%3d] Starting\n", my_rank);
	    done_waiting= false;
	    rcv_cnt= 0;
	    msg_wait_time_start= getCurrentSimTime();
	    if (my_rank < nranks / 2)   {
		state= SENDING;
		state_SENDING(event);
	    } else   {
		state= WAITING;
		state_WAITING(event);
	    }
	    break;

	case COMPUTE_DONE:
	case RECEIVE:
	case CHCKPT_DONE:
	case FAIL:
	case LOG_MSG_DONE:
	case ENV_LOG_DONE:
	case ENTER_WAIT:
	case BIT_BUCKET_WRITE_START:
	case BIT_BUCKET_WRITE_DONE:
	case BIT_BUCKET_READ_START:
	case BIT_BUCKET_READ_DONE:
	    _abort(msgrate_pattern, "[%3d] Invalid event %d in state %d\n", my_rank, event, state);
	    break;
    }

}  // end of state_INIT()



void
Msgrate_pattern::state_SENDING(pattern_event_t event)
{

SimTime_t delay;


    switch (event)   {
	case START:
	    delay= 0;
	    for (int k= 0; k < num_msgs; k++)   {
		common->send(delay, my_rank + nranks / 2, exchange_msg_len + envelope_size);
		delay += SEND_DELAY;
	    }
	    application_done= true;
	    break;

	case COMPUTE_DONE:
	case RECEIVE:
	case CHCKPT_DONE:
	case FAIL:
	case LOG_MSG_DONE:
	case ENV_LOG_DONE:
	case ENTER_WAIT:
	case BIT_BUCKET_WRITE_START:
	case BIT_BUCKET_WRITE_DONE:
	case BIT_BUCKET_READ_START:
	case BIT_BUCKET_READ_DONE:
	    _abort(msgrate_pattern, "[%3d] Invalid event %d in state %d\n", my_rank, event, state);
	    break;
    }

}  // end of state_SENDING()



void
Msgrate_pattern::state_WAITING(pattern_event_t event)
{

    switch (event)   {
	case START:
	    // Nothing to do yet
	    break;

	case RECEIVE:
	    count_receives();

	    if (done_waiting)   {
		msg_wait_time= getCurrentSimTime() - msg_wait_time_start;
		application_done= true;
		state= DONE;
	    }
	    break;

	case COMPUTE_DONE:
	case CHCKPT_DONE:
	case FAIL:
	case LOG_MSG_DONE:
	case ENV_LOG_DONE:
	case ENTER_WAIT:
	case BIT_BUCKET_WRITE_START:
	case BIT_BUCKET_WRITE_DONE:
	case BIT_BUCKET_READ_START:
	case BIT_BUCKET_READ_DONE:
	    _abort(msgrate_pattern, "[%3d] Invalid event %d in state %d\n", my_rank, event, state);
	    break;
    }

}  // end of state_WAITING()



void
Msgrate_pattern::state_DONE(pattern_event_t event)
{

    _abort(msgrate_pattern, "[%3d] Ended up in unused DONE state...\n", my_rank);

}  // end of state_DONE()



//
// -----------------------------------------------------------------------------
// Utility functions
//
void
Msgrate_pattern::count_receives(void)
{

    rcv_cnt++;
    if (rcv_cnt == num_msgs)   {
	done_waiting= true;
    }
    _MSGRATE_PATTERN_DBG(3, "[%3d] Got msg #%d/%d\n", my_rank, rcv_cnt, num_msgs);

}  // end of count_receives()
