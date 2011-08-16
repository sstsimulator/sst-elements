// Copyright 2009-2011 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2011, IBM Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


/*
    One of these pattern generators runs on each core of the
    simulated system. For the first test, the generators on the
    first half of cores (0...n/2-1) send a fixed number (num_msgs)
    of messages to one of the cores in the second half of the set
    (n/2...n-1).

    The number of cores is set by the mesh of meshes generated
    by genPatterns.  The number of messages sent by each sender
    (num_msgs) can be set in the SDL file using the num_msgs
    parameter in the Gp section. The message length can also be
    set there using the exchange_msg_len parameter, or using the
    command line option --msg_len for genPatterns.

    The senders do their sends and then enter into a reduce followed
    by a broadcast among all ranks. The receivers wait for the
    specified number of messages and measure how long it takes to
    receive them (from time 0 simulation time). That time value
    is used in the reduce operation that follows.  The total time
    is broadcast to everybody and rank zero calculates the average
    number of messages per second per sender from that.

    In the second test, rank 0 sends messages round-robin to ranks
    1...n-1.
*/
#include <sst_config.h>
#include "sst/core/serialization/element.h"
#include "msgrate_pattern.h"



void
Msgrate_pattern::handle_events(state_event sm_event)
{

    switch (state)   {
	case MSGR_INIT:
	    state_INIT(sm_event);
	    break;
	case MSGR_T1_SENDING:
	    state_T1_SENDING(sm_event);
	    break;
	case MSGR_T1_RECEIVING:
	    state_T1_RECEIVING(sm_event);
	    break;
    }

    if (done)   {
	unregisterExit();
	done= false;
    }

}  /* end of handle_events() */



void
Msgrate_pattern::state_INIT(state_event sm_event)
{

msgrate_events_t e= (msgrate_events_t)sm_event.event;


    switch (e)   {
	case E_START_T1:
	    done= false;
	    if (my_rank == 0)   {
		printf("#  |||  Test 1: Ranks 0...%d will send %d messages to ranks %d...%d\n",
		    num_ranks / 2 - 1, num_msgs, num_ranks / 2, num_ranks - 1);
	    }

	    if (my_rank < num_ranks / 2)   {
		// I'm one of the senders
		state_transition(E_START_T1, MSGR_T1_SENDING);
	    } else   {
		// I'm one of the receivers
		state_transition(E_START_T1, MSGR_T1_RECEIVING);
	    }
	    break;

	default:
	    _abort(msgrate_pattern, "[%3d] Invalid event %d in state %d\n", my_rank, e, state);
	    break;
    }

}  // end of state_INIT()



void
Msgrate_pattern::state_T1_SENDING(state_event sm_event)
{

msgrate_events_t e= (msgrate_events_t)sm_event.event;


    switch (e)   {
	case E_START_T1:
	    break;

	default:
	    _abort(msgrate_pattern, "[%3d] Invalid event %d in state %d\n", my_rank, e, state);
	    break;
    }

}  // end of state_T1_SENDING()



void
Msgrate_pattern::state_T1_RECEIVING(state_event sm_event)
{

msgrate_events_t e= (msgrate_events_t)sm_event.event;


    switch (e)   {
	case E_START_T1:
	    rcv_cnt= 0;
	    msg_wait_time_start= getCurrentSimTime();
	    break;

	default:
	    _abort(msgrate_pattern, "[%3d] Invalid event %d in state %d\n", my_rank, e, state);
	    break;
    }

}  // end of state_T1_RECEIVING()



extern "C" {
Msgrate_pattern *
msgrate_patternAllocComponent(SST::ComponentId_t id,
                          SST::Component::Params_t& params)
{
    return new Msgrate_pattern(id, params);
}
}

BOOST_CLASS_EXPORT(Msgrate_pattern)
