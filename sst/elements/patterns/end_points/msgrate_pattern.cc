// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
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
    set there using the msg_len parameter (default 0).

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
#include "msgrate_pattern.h"
#include <sst/core/element.h>



void
Msgrate_pattern::handle_events(state_event sm_event)
{

    switch (state)   {
	case STATE_INIT:		state_INIT(sm_event); break;
	case STATE_T1_SENDING:		state_T1_SENDING(sm_event); break;
	case STATE_T1_RECEIVING:	state_T1_RECEIVING(sm_event); break;
	case STATE_ALLREDUCE_T1:	state_ALLREDUCE_T1(sm_event); break;
	case STATE_T2:			state_T2(sm_event); break;
	case STATE_T2_SENDING:		state_T2_SENDING(sm_event); break;
	case STATE_T2_RECEIVING:	state_T2_RECEIVING(sm_event); break;
	case STATE_ALLREDUCE_T2:	state_ALLREDUCE_T2(sm_event); break;
	case STATE_T3:			state_T3(sm_event); break;
	case STATE_T3_SENDING:		state_T3_SENDING(sm_event); break;
	case STATE_T3_RECEIVING:	state_T3_RECEIVING(sm_event); break;
	case STATE_ALLREDUCE_T3:	state_ALLREDUCE_T3(sm_event); break;
    }

    if (done)   {
  primaryComponentOKToEndSim();
	done= false;
    }

}  /* end of handle_events() */



void
Msgrate_pattern::state_INIT(state_event sm_event)
{

msgrate_events_t e= (msgrate_events_t)sm_event.event;


    switch (e)   {
	case E_START_T1:
	    rcv_cnt= 0;
	    if (my_rank == 0)   {
		printf("#  |||  Test 1: Ranks 0...%d will send %d messages of length %d to ranks %d...%d along %d paths\n",
		    num_ranks / 2 - 1, num_msgs, msg_len, num_ranks / 2, num_ranks - 1, num_ranks / 2);
	    }

	    if (my_rank < num_ranks / 2)   {
		// I'm one of the senders
		msg_wait_time= 0.0;
		goto_state(state_T1_SENDING, STATE_T1_SENDING, E_START_T1);
	    } else   {
		// I'm one of the receivers; start the wait for messages.
		state= STATE_T1_RECEIVING;
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
int dest;
state_event msgr_event;


    switch (e)   {
	case E_START_T1:
	    // Send num_msg to my partner in the other half of the machine
	    // NOTE: These all go out simultaneously in simulation time.
	    // However, as soon as they go into the network, they get queued and
	    // experience the proper delays
	    dest= my_rank + num_ranks / 2;
	    msgr_event.event= E_T1_RECEIVE;
	    for (int i= 0; i < num_msgs; i++)   {
		send_msg(dest, msg_len, msgr_event);
	    }

	    goto_state(state_ALLREDUCE_T1, STATE_ALLREDUCE_T1, E_ALLREDUCE_ENTRY);
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
	case E_T1_RECEIVE:
	    // Got another message
	    rcv_cnt++;
	    if (rcv_cnt == 1)   {
		// Got the first message; start the timer
		msg_wait_time_start= getCurrentSimTime();
	    }

	    if (rcv_cnt >= num_msgs)   {
		msg_wait_time= SimTimeToD(getCurrentSimTime() - msg_wait_time_start);
		goto_state(state_ALLREDUCE_T1, STATE_ALLREDUCE_T1, E_ALLREDUCE_ENTRY);
	    }
	    break;

	default:
	    _abort(msgrate_pattern, "[%3d] Invalid event %d in state %d\n", my_rank, e, state);
	    break;
    }

}  // end of state_T1_RECEIVING()



void
Msgrate_pattern::state_ALLREDUCE_T1(state_event sm_event)
{

msgrate_events_t e= (msgrate_events_t)sm_event.event;
state_event enter_allreduce, exit_allreduce;


    switch (e)   {
	case E_ALLREDUCE_ENTRY:
	    // Set the parameters to be passed to the allreduce SM
	    enter_allreduce.event= SM_START_EVENT;
	    enter_allreduce.set_Fdata(msg_wait_time);
	    enter_allreduce.set_Idata(Allreduce_op::OP_SUM);

	    // We want to be called with this event, when allreduce returns
	    exit_allreduce.event= E_ALLREDUCE_EXIT;

	    SM->SM_call(SMallreduce, enter_allreduce, exit_allreduce);
	    break;

	case E_ALLREDUCE_EXIT:
	    // Calculate the average, and let rank 0 display it
	    if (my_rank == 0)   {
		printf("#  |||  Test 1: Average bi-section rate: %8.0f msgs/s\n",
		    1.0 / ((sm_event.get_Fdata() / (num_ranks / 2) / num_msgs)));
	    }

	    // Go to test 2
	    goto_state(state_T2, STATE_T2, E_START_T2);
	    break;

	default:
	    _abort(msgrate_pattern, "[%3d] Invalid event %d in state %d\n", my_rank, e, state);
	    break;
    }

}  // end of state_ALLREDUCE_T1()



void
Msgrate_pattern::state_T2(state_event sm_event)
{

msgrate_events_t e= (msgrate_events_t)sm_event.event;


    switch (e)   {
	case E_START_T2:
	    msg_wait_time= 0.0;
	    rcv_cnt= 0;

	    if (my_rank == 0)   {
		if (rank_stride == 0)   {
		    printf("#  |||  Test 2: Rank 0 will send %d messages of length %d to ranks %d...%d\n",
			num_msgs, msg_len, my_rank + 1, num_ranks - 1);
		} else   {
		    printf("#  |||  Test 2: Rank 0 will send %d messages of length %d to ranks %d...%d, stride %d\n",
			num_msgs, msg_len, my_rank + 1, num_ranks - 1, rank_stride);
		}
		goto_state(state_T2_SENDING, STATE_T2_SENDING, E_START_T2);
	    } else if ((rank_stride == 0) && (my_rank == start_rank))   {
		/* I'm the only receiver */
		state= STATE_T2_RECEIVING;
	    } else if ((rank_stride == 0) && (my_rank != start_rank))   {
		/* I'm one of the others */
		goto_state(state_ALLREDUCE_T2, STATE_ALLREDUCE_T2, E_ALLREDUCE_ENTRY);
	    } else if ((my_rank >= start_rank) && ((my_rank - start_rank) % rank_stride) == 0)   {
		// I'm one of the receivers; start the wait for messages.
		state= STATE_T2_RECEIVING;
	    } else   {
		// I'm not participating in this test; skip it.
		goto_state(state_ALLREDUCE_T2, STATE_ALLREDUCE_T2, E_ALLREDUCE_ENTRY);
	    }
	    break;

	default:
	    _abort(msgrate_pattern, "[%3d] Invalid event %d in state %d\n", my_rank, e, state);
	    break;
    }

}  // end of state_T2()



void
Msgrate_pattern::state_T2_SENDING(state_event sm_event)
{

msgrate_events_t e= (msgrate_events_t)sm_event.event;
int dest;
state_event msgr_event;


    switch (e)   {
	case E_START_T2:
	    // Give the receivers a chance to get ready
	    // Doing a barrier would be cleaner
	    local_compute(E_START_T2_SEND, 2000000);
	    break;

	case E_START_T2_SEND:
	    // Send num_msg to my partners
	    dest= start_rank;
	    msgr_event.event= E_T2_RECEIVE;
	    for (int i= 0; i < num_msgs * (num_ranks - 1); i++)   {
		send_msg(dest, msg_len, msgr_event);
		dest= dest + rank_stride;
		if (dest >= num_ranks)   {
		    dest= start_rank;
		}
	    }

	    // Go to the allreduce
	    goto_state(state_ALLREDUCE_T2, STATE_ALLREDUCE_T2, E_ALLREDUCE_ENTRY);
	    break;

	default:
	    _abort(msgrate_pattern, "[%3d] Invalid event %d in state %d\n", my_rank, e, state);
	    break;
    }

}  // end of state_T2_SENDING()



void
Msgrate_pattern::state_T2_RECEIVING(state_event sm_event)
{

msgrate_events_t e= (msgrate_events_t)sm_event.event;


    switch (e)   {
	case E_T2_RECEIVE:
	    // Got another message
	    rcv_cnt++;
	    if (rcv_cnt == 1)   {
		msg_wait_time_start= getCurrentSimTime();
	    }

	    if (rcv_cnt >= num_msgs)   {
		msg_wait_time= SimTimeToD(getCurrentSimTime() - msg_wait_time_start);
		goto_state(state_ALLREDUCE_T2, STATE_ALLREDUCE_T2, E_ALLREDUCE_ENTRY);
	    }
	    break;

	default:
	    _abort(msgrate_pattern, "[%3d] Invalid event %d in state %d\n", my_rank, e, state);
	    break;
    }

}  // end of state_T2_RECEIVING()



void
Msgrate_pattern::state_ALLREDUCE_T2(state_event sm_event)
{

msgrate_events_t e= (msgrate_events_t)sm_event.event;
state_event enter_allreduce, exit_allreduce;
state_event t3_event;


    switch (e)   {
	case E_ALLREDUCE_ENTRY:
	    // Set the parameters to be passed to the allreduce SM
	    enter_allreduce.event= SM_START_EVENT;
	    enter_allreduce.set_Fdata(msg_wait_time);
	    enter_allreduce.set_Idata(Allreduce_op::OP_SUM);

	    // We want to be called with this event, when allreduce returns
	    exit_allreduce.event= E_ALLREDUCE_EXIT;

	    SM->SM_call(SMallreduce, enter_allreduce, exit_allreduce);
	    break;

	case E_ALLREDUCE_EXIT:
	    // Calculate the average, and let rank 0 display it
	    if (my_rank == 0)   {
		int num_receivers;

		if (rank_stride == 0)   {
		    num_receivers= 1;
		} else   {
		    num_receivers= ((num_ranks - start_rank) / rank_stride);
		    if ((num_ranks - start_rank) % rank_stride != 0)   {
			num_receivers++;
		    }
		}
		printf("#  |||  Test 2: %d receivers\n", num_receivers);
		printf("#  |||  Test 2: Average send rate:       %8.0f msgs/s\n",
		    1.0 / ((sm_event.get_Fdata() / (num_ranks - 1) / num_msgs)));
	    }

	    // Go to test 3
	    t3_event.event= E_START_T3;
	    state_T3(t3_event);
	    break;

	default:
	    _abort(msgrate_pattern, "[%3d] Invalid event %d in state %d\n", my_rank, e, state);
	    break;
    }

}  // end of state_ALLREDUCE_T2()



void
Msgrate_pattern::state_T3(state_event sm_event)
{

msgrate_events_t e= (msgrate_events_t)sm_event.event;


    switch (e)   {
	case E_START_T3:
	    msg_wait_time= 0.0;
	    rcv_cnt= 0;

	    if (my_rank == 0)   {
		printf("#  |||  Test 3: Ranks 1...%d will send %d messages of length %d to rank 0\n",
		    num_ranks - 1, num_msgs, msg_len);
		state= STATE_T3_RECEIVING;
	    } else   {
		// All other nodes send
		goto_state(state_T3_SENDING, STATE_T3_SENDING, E_START_T3);
	    }
	    break;

	default:
	    _abort(msgrate_pattern, "[%3d] Invalid event %d in state %d\n", my_rank, e, state);
	    break;
    }

}  // end of state_T3()



void
Msgrate_pattern::state_T3_SENDING(state_event sm_event)
{

msgrate_events_t e= (msgrate_events_t)sm_event.event;
state_event msgr_event;


    switch (e)   {
	case E_START_T3:
	    // Send num_msg to 0
	    msgr_event.event= E_T3_RECEIVE;
	    for (int i= 0; i < num_msgs; i++)   {
		send_msg(0, msg_len, msgr_event);
	    }

	    // Go to the allreduce
	    goto_state(state_ALLREDUCE_T3, STATE_ALLREDUCE_T3, E_ALLREDUCE_ENTRY);
	    break;

	default:
	    _abort(msgrate_pattern, "[%3d] Invalid event %d in state %d\n", my_rank, e, state);
	    break;
    }

}  // end of state_T3_SENDING()



void
Msgrate_pattern::state_T3_RECEIVING(state_event sm_event)
{

msgrate_events_t e= (msgrate_events_t)sm_event.event;


    switch (e)   {
	case E_T3_RECEIVE:
	    // Got another message
	    rcv_cnt++;
	    if (rcv_cnt == 1)   {
		msg_wait_time_start= getCurrentSimTime();
	    }

	    if (rcv_cnt >= num_msgs * (num_ranks - 1))   {
		msg_wait_time= SimTimeToD(getCurrentSimTime() - msg_wait_time_start);
		goto_state(state_ALLREDUCE_T3, STATE_ALLREDUCE_T3, E_ALLREDUCE_ENTRY);
	    }
	    break;

	default:
	    _abort(msgrate_pattern, "[%3d] Invalid event %d in state %d\n", my_rank, e, state);
	    break;
    }

}  // end of state_T3_RECEIVING()



void
Msgrate_pattern::state_ALLREDUCE_T3(state_event sm_event)
{

msgrate_events_t e= (msgrate_events_t)sm_event.event;
state_event enter_allreduce, exit_allreduce;


    switch (e)   {
	case E_ALLREDUCE_ENTRY:
	    // Set the parameters to be passed to the allreduce SM
	    enter_allreduce.event= SM_START_EVENT;
	    enter_allreduce.set_Fdata(msg_wait_time);
	    enter_allreduce.set_Idata(Allreduce_op::OP_SUM);

	    // We want to be called with this event, when allreduce returns
	    exit_allreduce.event= E_ALLREDUCE_EXIT;

	    SM->SM_call(SMallreduce, enter_allreduce, exit_allreduce);
	    break;

	case E_ALLREDUCE_EXIT:
	    // Calculate the average, and let rank 0 display it
	    if (my_rank == 0)   {
		printf("#  |||  Test 3: Average receive rate:    %8.0f msgs/s\n",
		    1.0 / (msg_wait_time / (num_msgs * (num_ranks - 1))));
	    }

	    done= true;
	    break;

	default:
	    _abort(msgrate_pattern, "[%3d] Invalid event %d in state %d\n", my_rank, e, state);
	    break;
    }

}  // end of state_ALLREDUCE_T3()



/*
// THIS SECTION MOVED TO patterns.cc FOR CONFIG CHANGE TO ONE LIBRARY FILE - ALEVINE

eli(Msgrate_pattern, msgrate_pattern, "Message rate pattern")
*/

// ADDED FOR PROPER INITIALIZATION - ALEVINE
// SST Startup and Shutdown
void Msgrate_pattern::setup() 
{
	// Call the initial State Transition
	state_transition(E_START_T1, STATE_INIT);
}

#ifdef SERIALIZATION_WORKS_NOW
BOOST_CLASS_EXPORT(Msgrate_pattern)
#endif // SERIALIZATION_WORKS_NOW
