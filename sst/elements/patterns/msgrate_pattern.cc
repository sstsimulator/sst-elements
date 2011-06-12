// Copyright 2009-2011 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2011, Sandia Corporation
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
#include <sst/core/cpunicEvent.h>
#include <queue>
#include <assert.h>
#include "msgrate_pattern.h"
#include "pattern_common.h"

// It's unrealistic to be able to send without any delay
#define SEND_DELAY	(1000)	// 1 usec


#define STATE_DBG(to) \
    _MSGRATE_PATTERN_DBG(1, "[%3d] state transition to %s\n", my_rank, to);



void
Msgrate_pattern::handle_events(CPUNicEvent *e)
{

pattern_event_t event;


    // Extract the pattern event type from the SST event
    // (We are "misusing" the routine filed in CPUNicEvent to xmit the event type
    event= (pattern_event_t)e->GetRoutine();

    if (application_done && (state == DONE))   {
	_MSGRATE_PATTERN_DBG(0, "[%3d] There should be no more events! (%d)\n",
	    my_rank, event);
	return;
    }

    _MSGRATE_PATTERN_DBG(2, "[%3d] In state %d and got event %d at time %lu\n", my_rank,
	state, event, getCurrentSimTime());

    switch (state)   {
	case INIT:
	    state_INIT(event, e);
	    break;
	case SENDING1:
	    state_SENDING1(event);
	    break;
	case WAITING1:
	    state_WAITING1(event, e);
	    break;
	case SENDING2:
	    state_SENDING2(event);
	    break;
	case WAITING2:
	    state_WAITING2(event, e);
	    break;
	case SENDING3:
	    state_SENDING3(event);
	    break;
	case WAITING3:
	    state_WAITING3(event, e);
	    break;
	case BCAST1:
	    state_BCAST1(event, e);
	    break;
	case BCAST2:
	    state_BCAST2(event, e);
	    break;
	case REDUCE1:
	    state_REDUCE1(event, e);
	    break;
	case REDUCE2:
	    state_REDUCE2(event, e);
	    break;
	case REDUCE3:
	    state_REDUCE3(event, e);
	    break;
	case DONE:
	    state_DONE(event);
	    break;
    }

    delete(e);

    if (application_done && (state == DONE))   {
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
Msgrate_pattern::state_INIT(pattern_event_t event, CPUNicEvent *e)
{

    switch (event)   {
	case START:
	    _MSGRATE_PATTERN_DBG(4, "[%3d] Starting\n", my_rank);
	    done_waiting= false;
	    rcv_cnt= 0;
	    msg_wait_time_start= getCurrentSimTime();
	    if (my_rank < nranks / 2)   {
		STATE_DBG("SENDING1");
		state= SENDING1;
		state_SENDING1(event);
	    } else   {
		STATE_DBG("WAITING1");
		state= WAITING1;
		state_WAITING1(event, e);
	    }
	    break;

	case COMPUTE_DONE:
	case RECEIVE:
	case CHCKPT_DONE:
	case FAIL:
	case LOG_MSG_DONE:
	case ENV_LOG_DONE:
	case ENTER_WAIT:
	case BCAST_DATA:
	case REDUCE_DATA:
	case BIT_BUCKET_WRITE_START:
	case BIT_BUCKET_WRITE_DONE:
	case BIT_BUCKET_READ_START:
	case BIT_BUCKET_READ_DONE:
	    _abort(msgrate_pattern, "[%3d] Invalid event %d in state %d\n", my_rank, event, state);
	    break;
    }

}  // end of state_INIT()



void
Msgrate_pattern::state_SENDING1(pattern_event_t event)
{

SimTime_t delay;
SimTime_t value;


    switch (event)   {
	case START:
	    delay= 0;
	    for (int k= 0; k < num_msgs; k++)   {
		common->send(delay, my_rank + nranks / 2, exchange_msg_len + envelope_size);
		delay += SEND_DELAY;
	    }

	    // Start the reduce
	    STATE_DBG("REDUCE1");
	    state= REDUCE1;
	    value= 0;
	    if (my_rank >= nranks / 2)   {
		// If I'm a leaf, send my data
		if (parent >= 0)   {
		    common->event_send(parent, REDUCE_DATA, 0, 0, 0, (char *)&value, sizeof(value));
		}
		STATE_DBG("BCAST1"); // This is really to REDUCE1 to BCAST1
		state= BCAST1;
	    } else   {
		// Otherwise save it for later
		reduce_queue.push(value);
	    }
	    break;

	case COMPUTE_DONE:
	case RECEIVE:
	case CHCKPT_DONE:
	case FAIL:
	case LOG_MSG_DONE:
	case ENV_LOG_DONE:
	case ENTER_WAIT:
	case BCAST_DATA:
	case REDUCE_DATA:
	case BIT_BUCKET_WRITE_START:
	case BIT_BUCKET_WRITE_DONE:
	case BIT_BUCKET_READ_START:
	case BIT_BUCKET_READ_DONE:
	    _abort(msgrate_pattern, "[%3d] Invalid event %d in state %d\n", my_rank, event, state);
	    break;
    }

}  // end of state_SENDING1()



void
Msgrate_pattern::state_WAITING1(pattern_event_t event, CPUNicEvent *e)
{

SimTime_t value;
SimTime_t msg_wait_time;
int value_len;
unsigned int leaves;




    switch (event)   {
	case START:
	    // Nothing to do yet
	    break;

	case RECEIVE:
	    count_receives1();

	    if (done_waiting)   {
		msg_wait_time= getCurrentSimTime() - msg_wait_time_start;

		// Start the reduce
		STATE_DBG("REDUCE1");
		state= REDUCE1;
		if (my_rank >= nranks / 2)   {
		    // If I'm a leaf, send my data
		    if (parent >= 0)   {
			common->event_send(parent, REDUCE_DATA, 0, 0, 0, (char *)&msg_wait_time, sizeof(msg_wait_time));
		    }
		    STATE_DBG("BCAST1");
		    state= BCAST1;
		} else   {
		    // Otherwise save it for later
		    reduce_queue.push(msg_wait_time);
		}
		done_waiting= false;
	    }
	    break;

	case REDUCE_DATA:
	    // Extract the reduce data and push it onto our queue
	    value_len= sizeof(value);
	    e->DetachPayload(&value, &value_len);
	    reduce_queue.push(value);

	    // Do I have one or two children?
	    if (nranks >= ((my_rank + 1) * 2))    {
		leaves= 2;
	    } else   {
		// I wouldn't be here, if I had no children
		leaves= 1;
	    }

	    if (reduce_queue.size() >= (leaves + 1))   {
		// We got it all. Add it up and send it to the parent
		value= reduce_queue.front();
		reduce_queue.pop();
		value= value + reduce_queue.front();
		reduce_queue.pop();
		if (!reduce_queue.empty())   {
		    value= value + reduce_queue.front();
		    reduce_queue.pop();
		}

		// Queue should be empty now
		assert(reduce_queue.empty());

		// Send result up
		if (parent >= 0)   {
		    common->event_send(parent, REDUCE_DATA, 0, 0, 0, (char *)&value, sizeof(value));
		}
	    }
	    break;

	case COMPUTE_DONE:
	case CHCKPT_DONE:
	case FAIL:
	case LOG_MSG_DONE:
	case ENV_LOG_DONE:
	case ENTER_WAIT:
	case BCAST_DATA:
	case BIT_BUCKET_WRITE_START:
	case BIT_BUCKET_WRITE_DONE:
	case BIT_BUCKET_READ_START:
	case BIT_BUCKET_READ_DONE:
	    _abort(msgrate_pattern, "[%3d] Invalid event %d in state %d\n", my_rank, event, state);
	    break;
    }

}  // end of state_WAITING1()



void
Msgrate_pattern::state_SENDING2(pattern_event_t event)
{

SimTime_t delay;
SimTime_t value;
int dest;


    switch (event)   {
	case START:
	    delay= 0;
	    dest= 1;
	    // Rank 0 sends num_msgs round robin to ranks 1..n-1
	    msg_wait_time_start= getCurrentSimTime();
	    for (int k= 0; k < num_msgs; k++)   {
		common->send(delay, dest, exchange_msg_len + envelope_size);
		delay += SEND_DELAY;
		dest++;
		if (dest % nranks == 0)   {
		    dest= 1;
		}
	    }

	    // Start the second reduce
	    STATE_DBG("REDUCE3");
	    state= REDUCE3;
	    value= 0;
	    if (my_rank >= nranks / 2)   {
		// If I'm a leaf, send my data
		if (parent >= 0)   {
		    common->event_send(parent, REDUCE_DATA, 0, 0, 0, (char *)&value, sizeof(value));
		}
		STATE_DBG("BCAST2");
		state= BCAST2;
	    } else   {
		// Otherwise save it for later
		reduce_queue.push(value);
	    }
	    break;

	case COMPUTE_DONE:
	case RECEIVE:
	case CHCKPT_DONE:
	case FAIL:
	case LOG_MSG_DONE:
	case ENV_LOG_DONE:
	case ENTER_WAIT:
	case BCAST_DATA:
	case REDUCE_DATA:
	case BIT_BUCKET_WRITE_START:
	case BIT_BUCKET_WRITE_DONE:
	case BIT_BUCKET_READ_START:
	case BIT_BUCKET_READ_DONE:
	    _abort(msgrate_pattern, "[%3d] Invalid event %d in state %d\n", my_rank, event, state);
	    break;
    }

}  // end of state_SENDING2()



void
Msgrate_pattern::state_WAITING2(pattern_event_t event, CPUNicEvent *e)
{

SimTime_t value;
SimTime_t msg_wait_time;
int value_len;
unsigned int leaves;




    switch (event)   {
	case START:
	    // Nothing to do yet
	    break;

	case RECEIVE:
	    count_receives2();

	    if (done_waiting)   {
		msg_wait_time= getCurrentSimTime() - msg_wait_time_start;
		application_done= true;

		// Start the reduce
		state= REDUCE3;
		STATE_DBG("REDUCE3");
		if (my_rank >= nranks / 2)   {
		    // If I'm a leaf, send my data
		    if (parent >= 0)   {
			common->event_send(parent, REDUCE_DATA, 0, 0, 0, (char *)&msg_wait_time, sizeof(msg_wait_time));
		    }
		    STATE_DBG("BCAST2");
		    state= BCAST2;
		} else   {
		    // Otherwise save it for later
		    reduce_queue.push(msg_wait_time);
		}
	    }
	    break;

	case REDUCE_DATA:
	    // Extract the reduce data and push it onto our queue
	    value_len= sizeof(value);
	    e->DetachPayload(&value, &value_len);
	    reduce_queue.push(value);

	    // Do I have one or two children?
	    if (nranks >= ((my_rank + 1) * 2))    {
		leaves= 2;
	    } else   {
		// I wouldn't be here, if I had no children
		leaves= 1;
	    }

	    if (reduce_queue.size() >= (leaves + 1))   {
		// We got it all. Add it up and send it to the parent
		value= reduce_queue.front();
		reduce_queue.pop();
		value= value + reduce_queue.front();
		reduce_queue.pop();
		if (!reduce_queue.empty())   {
		    value= value + reduce_queue.front();
		    reduce_queue.pop();
		}

		// Queue should be empty now
		assert(reduce_queue.empty());

		// Send result up
		if (parent >= 0)   {
		    common->event_send(parent, REDUCE_DATA, 0, 0, 0, (char *)&value, sizeof(value));
		}
	    }
	    break;

	case COMPUTE_DONE:
	case CHCKPT_DONE:
	case FAIL:
	case LOG_MSG_DONE:
	case ENV_LOG_DONE:
	case ENTER_WAIT:
	case BCAST_DATA:
	case BIT_BUCKET_WRITE_START:
	case BIT_BUCKET_WRITE_DONE:
	case BIT_BUCKET_READ_START:
	case BIT_BUCKET_READ_DONE:
	    _abort(msgrate_pattern, "[%3d] Invalid event %d in state %d\n", my_rank, event, state);
	    break;
    }

}  // end of state_WAITING2()



void
Msgrate_pattern::state_SENDING3(pattern_event_t event)
{

SimTime_t delay;
int dest;


    switch (event)   {
	case START:
	    delay= 0;
	    dest= 0;
	    // Everybody sends to rank 0
	    for (int k= 0; k < num_msgs / (nranks - 1); k++)   {
		common->send(delay, dest, exchange_msg_len + envelope_size);
		delay += SEND_DELAY;
	    }
	    application_done= true;

	    // Start the second reduce
	    STATE_DBG("DONE");
	    state= DONE;
	    break;

	case COMPUTE_DONE:
	case RECEIVE:
	case CHCKPT_DONE:
	case FAIL:
	case LOG_MSG_DONE:
	case ENV_LOG_DONE:
	case ENTER_WAIT:
	case BCAST_DATA:
	case REDUCE_DATA:
	case BIT_BUCKET_WRITE_START:
	case BIT_BUCKET_WRITE_DONE:
	case BIT_BUCKET_READ_START:
	case BIT_BUCKET_READ_DONE:
	    _abort(msgrate_pattern, "[%3d] Invalid event %d in state %d\n", my_rank, event, state);
	    break;
    }

}  // end of state_SENDING3()



void
Msgrate_pattern::state_WAITING3(pattern_event_t event, CPUNicEvent *e)
{

SimTime_t msg_wait_time;
int num_rcvs;




    switch (event)   {
	case START:
	    // Nothing to do yet
	    break;

	case RECEIVE:
	    count_receives3();

	    if (done_waiting)   {
		msg_wait_time= getCurrentSimTime() - msg_wait_time_start;
		num_rcvs= num_msgs / (nranks - 1);
		num_rcvs= num_rcvs * (nranks - 1);
		application_done= true;

		// Start the reduce
		state= DONE;
		STATE_DBG("DONE");
		printf(">>> Average rate for %d messages from ranks 1...%d to 0 of length %d (+ %d B header): %.3f msgs/s\n",
		    num_msgs, nranks - 1, exchange_msg_len, envelope_size,
		    1000000000.0 / ((double)msg_wait_time / num_rcvs));
	    }
	    break;

	case REDUCE_DATA:
	case COMPUTE_DONE:
	case CHCKPT_DONE:
	case FAIL:
	case LOG_MSG_DONE:
	case ENV_LOG_DONE:
	case ENTER_WAIT:
	case BCAST_DATA:
	case BIT_BUCKET_WRITE_START:
	case BIT_BUCKET_WRITE_DONE:
	case BIT_BUCKET_READ_START:
	case BIT_BUCKET_READ_DONE:
	    _abort(msgrate_pattern, "[%3d] Invalid event %d in state %d\n", my_rank, event, state);
	    break;
    }

}  // end of state_WAITING3()



void
Msgrate_pattern::state_REDUCE1(pattern_event_t event, CPUNicEvent *e)
{

SimTime_t value;
int value_len;
unsigned int leaves;


    switch (event)   {
	case REDUCE_DATA:
	    // Extract the reduce data and push it onto our queue
	    value_len= sizeof(value);
	    e->DetachPayload(&value, &value_len);
	    reduce_queue.push(value);

	    // Do I have one or two children?
	    if (nranks > ((my_rank + 1) * 2))    {
		leaves= 2;
	    } else   {
		// I wouldn't be here, if I had no children
		leaves= 1;
	    }

	    if (reduce_queue.size() >= (leaves + 1))   {
		// We got it all. Add it up and send it to the parent
		value= reduce_queue.front();
		reduce_queue.pop();
		value= value + reduce_queue.front();
		reduce_queue.pop();
		if (!reduce_queue.empty())   {
		    value= value + reduce_queue.front();
		    reduce_queue.pop();
		}

		// Queue should be empty now
		assert(reduce_queue.empty());

		// Send result up
		if (parent >= 0)   {
		    common->event_send(parent, REDUCE_DATA, 0, 0, 0, (char *)&value, sizeof(value));
		}

		// Then enter the broadcast
		STATE_DBG("BCAST1");
		state= BCAST1;

		// If I'm the root, initiate the broadcast
		if (my_rank == 0)   {
		    printf(">>> Average rate for %d messages of length %d (+ %d B header): %.3f msgs/s\n",
			num_msgs, exchange_msg_len, envelope_size,
			1000000000.0 / ((double)value / (num_msgs * nranks / 2)));

		    if (left >= 0)   {
			common->event_send(left, BCAST_DATA, 0, 0, 0, (char *)&value, sizeof(value));
		    }
		    if (right >= 0)   {
			common->event_send(right, BCAST_DATA, 0, 0, 0, (char *)&value, sizeof(value));
		    }
		    bcast_done= true;
		    STATE_DBG("REDUCE2");
		    state= REDUCE2;
		    value= 0;
		    common->event_send(my_rank, REDUCE_DATA, 0, 0, 0, (char *)&value, sizeof(value));
		}
	    }
	    break;

	case BCAST_DATA:
	case RECEIVE:
	case START:
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

}  // end of state_REDUCE1()



void
Msgrate_pattern::state_REDUCE2(pattern_event_t event, CPUNicEvent *e)
{

SimTime_t value;
int value_len;
unsigned int leaves;


    switch (event)   {
	case REDUCE_DATA:
	    // Extract the reduce data and push it onto our queue
	    value_len= sizeof(value);
	    e->DetachPayload(&value, &value_len);
	    reduce_queue.push(value);

	    // Do I have one or two children?
	    if (nranks > ((my_rank + 1) * 2))    {
		leaves= 2;
	    } else   {
		// I wouldn't be here, if I had no children
		leaves= 1;
	    }

	    if (reduce_queue.size() >= (leaves + 1))   {
		// We got it all. Add it up and send it to the parent
		value= reduce_queue.front();
		reduce_queue.pop();
		value= value + reduce_queue.front();
		reduce_queue.pop();
		if (!reduce_queue.empty())   {
		    value= value + reduce_queue.front();
		    reduce_queue.pop();
		}

		// Queue should be empty now
		assert(reduce_queue.empty());

		// Send result up
		if (parent >= 0)   {
		    common->event_send(parent, REDUCE_DATA, 0, 0, 0, (char *)&value, sizeof(value));
		}

		if (my_rank == 0)   {
		    STATE_DBG("SENDING2");
		    state= SENDING2;
		    common->event_send(my_rank, START, 0, 0, 0, NULL, 0);
		} else   {
		    STATE_DBG("WAITING2");
		    state= WAITING2;
		}
	    }
	    break;

	case BCAST_DATA:
	case RECEIVE:
	case START:
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

}  // end of state_REDUCE2()



void
Msgrate_pattern::state_REDUCE3(pattern_event_t event, CPUNicEvent *e)
{

SimTime_t value;
SimTime_t msg_wait_time;
int value_len;
unsigned int leaves;


    switch (event)   {
	case REDUCE_DATA:
	    // Extract the reduce data and push it onto our queue
	    value_len= sizeof(value);
	    e->DetachPayload(&value, &value_len);
	    reduce_queue.push(value);

	    // Do I have one or two children?
	    if (nranks > ((my_rank + 1) * 2))    {
		leaves= 2;
	    } else   {
		// I wouldn't be here, if I had no children
		leaves= 1;
	    }

	    if (reduce_queue.size() >= (leaves + 1))   {
		// We got it all. Add it up and send it to the parent
		value= reduce_queue.front();
		reduce_queue.pop();
		value= value + reduce_queue.front();
		reduce_queue.pop();
		if (!reduce_queue.empty())   {
		    value= value + reduce_queue.front();
		    reduce_queue.pop();
		}

		// Queue should be empty now
		assert(reduce_queue.empty());

		// Send result up
		if (parent >= 0)   {
		    common->event_send(parent, REDUCE_DATA, 0, 0, 0, (char *)&value, sizeof(value));
		}

		STATE_DBG("BCAST2");
		state= BCAST2;

		// If I'm the root, print the result, and initiate the broadcast
		if (my_rank == 0)   {
		    msg_wait_time= getCurrentSimTime() - msg_wait_time_start;
		    printf(">>> Average rate for %d messages from rank 0 to 1...%d of length %d (+ %d B header): %.3f msgs/s\n",
			num_msgs, nranks - 1, exchange_msg_len, envelope_size,
			1000000000.0 / ((double)msg_wait_time / num_msgs));

		    if (left >= 0)   {
			common->event_send(left, BCAST_DATA, 0, 0, 0, (char *)&value, sizeof(value));
		    }
		    if (right >= 0)   {
			common->event_send(right, BCAST_DATA, 0, 0, 0, (char *)&value, sizeof(value));
		    }
		    bcast_done= true;
		    STATE_DBG("WAITING3");
		    state= WAITING3;
		}
	    }
	    break;

	case BCAST_DATA:
	case RECEIVE:
	case START:
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

}  // end of state_REDUCE3()



void
Msgrate_pattern::state_BCAST1(pattern_event_t event, CPUNicEvent *e)
{

SimTime_t value;
int value_len;


    switch (event)   {
	case BCAST_DATA:
	    // Extract data for our use
	    value_len= sizeof(value);
	    e->DetachPayload(&value, &value_len);

	    if (left >= 0)   {
		common->event_send(left, BCAST_DATA, 0, 0, 0, (char *)&value, value_len);
	    }
	    if (right >= 0)   {
		common->event_send(right, BCAST_DATA, 0, 0, 0, (char *)&value, value_len);
	    }
	    STATE_DBG("REDUCE2");
	    state= REDUCE2;
	    bcast_done= true;
	    value= 3;
	    if (my_rank >= nranks / 2)   {
		// If I'm a leaf, send my data
		if (parent >= 0)   {
		    common->event_send(parent, REDUCE_DATA, 0, 0, 0, (char *)&value, sizeof(value));
		}
		STATE_DBG("BCAST1");
		state= WAITING2;
	    } else   {
		// Otherwise save it for later
		reduce_queue.push(value);
	    }
	    break;

	case REDUCE_DATA:
	    printf("Should I ever get here?\n");
	    break;

	case RECEIVE:
	case START:
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

}  // end of state_BCAST1()



void
Msgrate_pattern::state_BCAST2(pattern_event_t event, CPUNicEvent *e)
{

SimTime_t value;
int value_len;


    switch (event)   {
	case BCAST_DATA:
	    // Extract data for our use
	    value_len= sizeof(value);
	    e->DetachPayload(&value, &value_len);

	    if (left >= 0)   {
		common->event_send(left, BCAST_DATA, 0, 0, 0, (char *)&value, value_len);
	    }
	    if (right >= 0)   {
		common->event_send(right, BCAST_DATA, 0, 0, 0, (char *)&value, value_len);
	    }
	    bcast_done= true;

	    // Test 3: Everybody sends to rank 0
	    if (my_rank == 0)   {
		STATE_DBG("WAITING3");
		state= WAITING3;
		msg_wait_time_start= getCurrentSimTime();
	    } else   {
		STATE_DBG("SENDING3");
		state= SENDING3;
		common->event_send(my_rank, START, 0, 0, 0, NULL, 0);
	    }
	    break;

	case REDUCE_DATA:
	case RECEIVE:
	case START:
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

}  // end of state_BCAST2()



void
Msgrate_pattern::state_DONE(pattern_event_t event)
{

    _abort(msgrate_pattern, "[%3d] Ended up in unused DONE state...\n", my_rank);

}  // end of state_DONE()



//
// -----------------------------------------------------------------------------
// Utility functions
//



// Count the receives for the first test
void
Msgrate_pattern::count_receives1(void)
{

    rcv_cnt++;
    _MSGRATE_PATTERN_DBG(3, "[%3d] Got msg #%d/%d for test 1\n", my_rank, rcv_cnt, num_msgs);

    if (rcv_cnt == num_msgs)   {
	done_waiting= true;
	rcv_cnt= 0;
    }

}  // end of count_receives1()



// Count the receives for the second test
void
Msgrate_pattern::count_receives2(void)
{

int num_rcvs;


    // Every receiver gets this many messages
    num_rcvs= num_msgs / (nranks - 1);

    // Some may get one more
    if (my_rank <= (num_msgs - (num_rcvs * (nranks - 1))))   {
	num_rcvs++;
    }

    rcv_cnt++;
    _MSGRATE_PATTERN_DBG(3, "[%3d] Got msg #%d/%d for test 2\n", my_rank, rcv_cnt, num_rcvs);

    if (rcv_cnt == num_rcvs)   {
	done_waiting= true;
    }

}  // end of count_receives2()



// Count the receives for the third test
void
Msgrate_pattern::count_receives3(void)
{

int num_rcvs;


    // n - 1 ranks send an equal number of messages
    num_rcvs= num_msgs / (nranks - 1);
    num_rcvs= num_rcvs * (nranks - 1);

    rcv_cnt++;
    _MSGRATE_PATTERN_DBG(3, "[%3d] Got msg #%d/%d for test 3\n", my_rank, rcv_cnt, num_rcvs);

    if (rcv_cnt == num_rcvs)   {
	done_waiting= true;
    }

}  // end of count_receives3()
