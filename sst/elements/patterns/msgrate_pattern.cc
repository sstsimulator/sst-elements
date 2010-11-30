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
#include <queue>
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
	case SENDING:
	    state_SENDING(event);
	    break;
	case WAITING:
	    state_WAITING(event, e);
	    break;
	case BCAST:
	    state_BCAST(event, e);
	    break;
	case REDUCE:
	    state_REDUCE(event, e);
	    break;
	case DONE:
	    state_DONE(event);
	    break;
    }

    delete(e);

    if (application_done && (state == DONE))   {
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
Msgrate_pattern::state_INIT(pattern_event_t event, CPUNicEvent *e)
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
		state_WAITING(event, e);
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
Msgrate_pattern::state_SENDING(pattern_event_t event)
{

SimTime_t delay;
int value;


    switch (event)   {
	case START:
	    delay= 0;
	    for (int k= 0; k < num_msgs; k++)   {
		common->send(delay, my_rank + nranks / 2, exchange_msg_len + envelope_size);
		delay += SEND_DELAY;
	    }
	    application_done= true;

	    // Start the reduce
	    state= REDUCE;
	    value= 44;
	    if (my_rank >= nranks / 2)   {
		// If I'm a leaf, send my data
		if (parent >= 0)   {
		    common->sendOB(parent, REDUCE_DATA, value);
		    fprintf(stderr, "[%2d} Start leaf reduce, value %d to parent %d\n", my_rank, value, parent);
		}
		state= BCAST;
	    } else   {
		// Otherwise save it for later
		reduce_queue.push(value);
		fprintf(stderr, "[%2d} Push reduce, value %d\n", my_rank, value);
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

}  // end of state_SENDING()



void
Msgrate_pattern::state_WAITING(pattern_event_t event, CPUNicEvent *e)
{

int value;
int value_len;
unsigned int leaves;




    switch (event)   {
	case START:
	    // Nothing to do yet
	    break;

	case RECEIVE:
	    count_receives();

	    if (done_waiting)   {
		msg_wait_time= getCurrentSimTime() - msg_wait_time_start;
		application_done= true;

		// Start the reduce
		state= REDUCE;
		value= 33;
		if (my_rank >= nranks / 2)   {
		    // If I'm a leaf, send my data
		    if (parent >= 0)   {
			common->sendOB(parent, REDUCE_DATA, value);
			fprintf(stderr, "[%2d} Start leaf reduce, value %d to parent %d\n", my_rank, value, parent);
		    }
		    state= BCAST;
		} else   {
		    // Otherwise save it for later
		    fprintf(stderr, "[%2d} Push reduce, value %d\n", my_rank, value);
		    reduce_queue.push(value);
		}
	    }
	    break;

	case REDUCE_DATA:
	    // Extract the reduce data and push it onto our queue
	    value_len= sizeof(value);
	    e->DetachPayload(&value, &value_len);
	    reduce_queue.push(value);
	    fprintf(stderr, "[%2d} Got reduce value %d in wait\n", my_rank, value);

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
		    fprintf(stderr, "[%2d} Send reduce value %d to parent %d\n", my_rank, value, parent);
		    common->sendOB(parent, REDUCE_DATA, value);
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

}  // end of state_WAITING()



void
Msgrate_pattern::state_REDUCE(pattern_event_t event, CPUNicEvent *e)
{

int value;
int value_len;
unsigned int leaves;


    switch (event)   {
	case REDUCE_DATA:
	    // Extract the reduce data and push it onto our queue
	    value_len= sizeof(value);
	    e->DetachPayload(&value, &value_len);
	    reduce_queue.push(value);
	    fprintf(stderr, "[%2d} Got reduce value %d in reduce\n", my_rank, value);

	    // Do I have one or two children?
	    if (nranks > ((my_rank + 1) * 2))    {
		leaves= 2;
	    } else   {
		// I wouldn't be here, if I had no children
		leaves= 1;
	    }
	    fprintf(stderr, "[%2d} state_REDUCE() %d leaves, size %d\n", my_rank, leaves, reduce_queue.size());

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
		fprintf(stderr, "[%2d} Send reduce value %d to parent %d\n", my_rank, value, parent);
		if (parent >= 0)   {
		    common->sendOB(parent, REDUCE_DATA, value);
		}

		// Then enter the broadcast
		state= BCAST;

		// If I'm the root, initiate the broadcast
		if (my_rank == 0)   {
		    fprintf(stderr, "[%2d} Doing bcast (root) of value %d\n", my_rank, value);
		    if (left >= 0)   {
			common->sendOB(left, BCAST_DATA, value);
		    }
		    if (right >= 0)   {
			common->sendOB(right, BCAST_DATA, value);
		    }
		    bcast_done= true;
		    state= DONE;
		}
	    }
	    fprintf(stderr, "[%2d} state_REDUCE() done with this REDUCE_DATA\n", my_rank);
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

}  // end of state_REDUCE()



void
Msgrate_pattern::state_BCAST(pattern_event_t event, CPUNicEvent *e)
{

int value;
int value_len;


    switch (event)   {
	case BCAST_DATA:
	    // Extract data for our use
	    value_len= sizeof(value);
	    e->DetachPayload(&value, &value_len);
	    fprintf(stderr, "[%2d} Doing bcast of value %d\n", my_rank, value);

	    if (left >= 0)   {
		common->sendOB(left, BCAST_DATA, value);
	    }
	    if (right >= 0)   {
		common->sendOB(right, BCAST_DATA, value);
	    }
	    state= DONE;
	    bcast_done= true;
	    break;

	case REDUCE_DATA:
	    fprintf(stderr, "[%2d} REDUCE_DATA in BCAST\n", my_rank);
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

}  // end of state_BCAST()



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
