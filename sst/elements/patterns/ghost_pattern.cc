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
#include "ghost_pattern.h"
#include "pattern_common.h"



void
Ghost_pattern::handle_events(CPUNicEvent *e)
{

pattern_event_t event;


    // Extract the pattern event type from the SST event
    // (We are "misusing" the routine filed in CPUNicEvent to xmit the event type
    event= (pattern_event_t)e->GetRoutine();

    if (application_done)   {
	_GHOST_PATTERN_DBG(0, "[%3d] There should be no more events! (%d)\n",
	    my_rank, event);
	return;
    }

    _GHOST_PATTERN_DBG(2, "[%3d] In state %d and got event %d at time %lu\n", my_rank,
	state, event, getCurrentSimTime());

    switch (state)   {
	case INIT:
	    state_INIT(event);
	    break;
	case COMPUTE:
	    state_COMPUTE(event);
	    break;
	case WAIT:
	    state_WAIT(event);
	    break;
	case DONE:
	    state_DONE(event);
	    break;
	case CHCKPT:
	    state_CHCKPT(event);
	    break;
	case SAVE_ENVELOPE:
	    state_SAVE_ENVELOPE(event);
	    break;
	case LOG_MSG1:
	    state_LOG_MSG1(event);
	    break;
	case LOG_MSG2:
	    state_LOG_MSG2(event);
	    break;
	case LOG_MSG3:
	    state_LOG_MSG3(event);
	    break;
	case LOG_MSG4:
	    state_LOG_MSG4(event);
	    break;
    }

    delete(e);

    if (application_done)   {
	if (my_rank == 0)   {
	    printf(">>> Application has done %.9f s of work in %d time steps\n",
		(double)application_time_so_far /  1000000000.0, timestep_cnt);
	    printf(">>> Rank 0 execution time %.9f s\n",
		(double)execution_time /  1000000000.0);
	    printf(">>> Rank 0 checkpoint time %.9f s\n",
		(double)chckpt_time /  1000000000.0);
	    printf(">>> Rank 0 message wait time %.9f s, %.2f%% of ececution time\n",
		(double)msg_wait_time /  1000000000.0,
		100.0 / (double)execution_time * (double)msg_wait_time);
	    printf(">>> Rank 0 total receives %d\n", total_rcvs);

	    switch (chckpt_method)   {
		case CHCKPT_NONE:
		    printf(">>> Checkpointing not enabled\n");
		    break;
		case CHCKPT_COORD:
		    printf(">>> Wrote %d coordinated checkpoints\n", num_chckpts);
		    break;
		case CHCKPT_UNCOORD:
		    printf(">>> Number of log writes, message log size needed\n");
		    break;
		case CHCKPT_RAID:
		    printf(">>> This checkpoint method is not supported yet\n");
		    break;
	    }
	}

	unregisterExit();
    }

    return;

}  /* end of handle_events() */



// Messages from the global network
void
Ghost_pattern::handle_net_events(Event *sst_event)
{

CPUNicEvent *e;


    e= static_cast<CPUNicEvent *>(sst_event);

    if (e->dest != my_rank)   {
	_abort(ghost_pattern, "NETWORK dest %d != my rank %d\n", e->dest, my_rank);
    }

    // FIXME: could/should check whether the src (in msg_id) is one of our 4 neighbors

    handle_events(e);

}  /* end of handle_net_events() */


// Messages from the local chip network
void
Ghost_pattern::handle_NoC_events(Event *sst_event)
{

CPUNicEvent *e;


    e= static_cast<CPUNicEvent *>(sst_event);

    if (e->dest != my_rank)   {
	_abort(ghost_pattern, "NoC dest %d != my rank %d\n", e->dest, my_rank);
    }

    // FIXME: could/should check whether the src (in msg_id) is one of our 4 neighbors

    handle_events(e);

}  /* end of handle_NoC_events() */


// When we send to ourselves, we come here.
// Just pass it on to the main handler above
void
Ghost_pattern::handle_self_events(Event *sst_event)
{

CPUNicEvent *e;


    e= static_cast<CPUNicEvent *>(sst_event);
    handle_events(e);

}  /* end of handle_self_events() */



// Events from the local NVRAM
void
Ghost_pattern::handle_nvram_events(Event *sst_event)
{

CPUNicEvent *e;


    e= static_cast<CPUNicEvent *>(sst_event);
    handle_events(e);

}  /* end of handle_nvram_events() */



// Events from stable storage
void
Ghost_pattern::handle_storage_events(Event *sst_event)
{

CPUNicEvent *e;


    e= static_cast<CPUNicEvent *>(sst_event);
    handle_events(e);

}  /* end of handle_storage_events() */



extern "C" {
Ghost_pattern *
ghost_patternAllocComponent(SST::ComponentId_t id,
                          SST::Component::Params_t& params)
{
    return new Ghost_pattern(id, params);
}
}

BOOST_CLASS_EXPORT(Ghost_pattern)



//
// -----------------------------------------------------------------------------
// For each state in the state machine we have a method to deal with incoming
// events.
// INIT --> COMPUTE on START event
void
Ghost_pattern::state_INIT(pattern_event_t event)
{

    switch (event)   {
	case START:
	    _GHOST_PATTERN_DBG(4, "[%3d] Starting, entering compute state\n", my_rank);
	    done_waiting= false;
	    timestep_needed= application_end_time / compute_time;
	    if (timestep_needed * compute_time < application_end_time)   {
		// Need a partial one at the end
		timestep_needed++;
	    }

	    execution_time= getCurrentSimTime();
	    transition_to_COMPUTE();
	    break;

	case FAIL:
	    break;

	case COMPUTE_DONE:
	case RECEIVE:
	case CHCKPT_DONE:
	case LOG_MSG_DONE:
	case ENV_LOG_DONE:
	case ENTER_WAIT:
	case BIT_BUCKET_WRITE_START:
	case BIT_BUCKET_WRITE_DONE:
	case BIT_BUCKET_READ_START:
	case BIT_BUCKET_READ_DONE:
	    _abort(ghost_pattern, "[%3d] Invalid event %d in state %d\n", my_rank, event, state);
	    break;
    }

}  // end of state_INIT()



// COMPUTE --> COMPUTE on RECEIVE event
// COMPUTE --> DONE on COMPUTE_DONE event, if all timesteps are done
// COMPUTE --> LOG_MSG1 on COMPUTE_DONE event, if sends are logged
// COMPUTE --> WAIT on COMPUTE_DONE event, if sends are not logged
void
Ghost_pattern::state_COMPUTE(pattern_event_t event)
{

    switch (event)   {
	case COMPUTE_DONE:
	    _GHOST_PATTERN_DBG(4, "[%3d] Done computing\n", my_rank);
	    application_time_so_far += getCurrentSimTime() - compute_segment_start;

	    if (timestep_cnt + 1 >= timestep_needed)   {
		application_done= TRUE;
		execution_time= getCurrentSimTime() - execution_time;
		// There is no ghost cell exchange after the last computation
		state= DONE;
		timestep_cnt++;

	    } else   {

		if (chckpt_method == CHCKPT_UNCOORD)   {
		    // We need to log these messages before we send them
		    state= LOG_MSG1;
		    common->nvram_write(exchange_msg_len, LOG_MSG_DONE);
		} else   {

		    // If we are not logging sends, do them and go to WAIT
		    common->send(right, exchange_msg_len);
		    common->send(left, exchange_msg_len);
		    common->send(up, exchange_msg_len);
		    common->send(down, exchange_msg_len);

		    // Go into WAIT. We'll determine there whether we have received all four
		    // messages and whether it is time to checkpoint
		    msg_wait_time_start= getCurrentSimTime();
		    state= WAIT;
		    state_WAIT(ENTER_WAIT);
		}
	    }
	    break;

	case RECEIVE:
	    // Just count the receives for later
	    count_receives();
	    break;

	case FAIL:
	    break;

	case START:
	case CHCKPT_DONE:
	case LOG_MSG_DONE:
	case ENV_LOG_DONE:
	case ENTER_WAIT:
	case BIT_BUCKET_WRITE_START:
	case BIT_BUCKET_WRITE_DONE:
	case BIT_BUCKET_READ_START:
	case BIT_BUCKET_READ_DONE:
	    _abort(ghost_pattern, "[%3d] Invalid event %d in state %d\n", my_rank, event, state);
	    break;
    }

}  // end of state_COMPUTE()



// We come here from COMPUTE, if uncoordinated checkpointing is enabled
// LOG_MSG1 --> LOG_MSG2 on LOG_MSG_DONE event
// LOG_MSG1 --> LOG_MSG1 on RECEIVE event
void
Ghost_pattern::state_LOG_MSG1(pattern_event_t event)
{

    switch (event)   {
	case LOG_MSG_DONE:
	    // Send the first message
	    common->send(right, exchange_msg_len);

	    // Write the second message to stable storage
	    state= LOG_MSG2;
	    common->nvram_write(exchange_msg_len, LOG_MSG_DONE);
	    break;

	case RECEIVE:
	    count_receives();
	    break;

	case FAIL:
	    break;

	case START:
	case COMPUTE_DONE:
	case CHCKPT_DONE:
	case ENV_LOG_DONE:
	case ENTER_WAIT:
	case BIT_BUCKET_WRITE_START:
	case BIT_BUCKET_WRITE_DONE:
	case BIT_BUCKET_READ_START:
	case BIT_BUCKET_READ_DONE:
	    _abort(ghost_pattern, "[%3d] Invalid event %d in state %d\n", my_rank, event, state);
	    break;
    }

}  // end of state_LOG_MSG1()



// We come here from LOG_MSG1, if uncoordinated checkpointing is enabled
// LOG_MSG2 --> LOG_MSG3 on LOG_MSG_DONE event
// LOG_MSG2 --> LOG_MSG2 on RECEIVE event
void
Ghost_pattern::state_LOG_MSG2(pattern_event_t event)
{

    switch (event)   {
	case LOG_MSG_DONE:
	    // Send the second message
	    common->send(left, exchange_msg_len);

	    // Write the third message to stable storage
	    state= LOG_MSG3;
	    common->nvram_write(exchange_msg_len, LOG_MSG_DONE);
	    break;

	case RECEIVE:
	    count_receives();
	    break;

	case FAIL:
	    break;

	case START:
	case COMPUTE_DONE:
	case CHCKPT_DONE:
	case ENV_LOG_DONE:
	case ENTER_WAIT:
	case BIT_BUCKET_WRITE_START:
	case BIT_BUCKET_WRITE_DONE:
	case BIT_BUCKET_READ_START:
	case BIT_BUCKET_READ_DONE:
	    _abort(ghost_pattern, "[%3d] Invalid event %d in state %d\n", my_rank, event, state);
	    break;
    }

}  // end of state_LOG_MSG2()



// We come here from LOG_MSG2, if uncoordinated checkpointing is enabled
// LOG_MSG3 --> LOG_MSG4 on LOG_MSG_DONE event
// LOG_MSG3 --> LOG_MSG3 on RECEIVE event
void
Ghost_pattern::state_LOG_MSG3(pattern_event_t event)
{

    switch (event)   {
	case LOG_MSG_DONE:
	    // Send the third message
	    common->send(up, exchange_msg_len);

	    // Write the fourth message to stable storage
	    state= LOG_MSG3;
	    common->nvram_write(exchange_msg_len, LOG_MSG_DONE);
	    break;

	case RECEIVE:
	    count_receives();
	    break;

	case FAIL:
	    break;

	case START:
	case COMPUTE_DONE:
	case CHCKPT_DONE:
	case ENV_LOG_DONE:
	case ENTER_WAIT:
	case BIT_BUCKET_WRITE_START:
	case BIT_BUCKET_WRITE_DONE:
	case BIT_BUCKET_READ_START:
	case BIT_BUCKET_READ_DONE:
	    _abort(ghost_pattern, "[%3d] Invalid event %d in state %d\n", my_rank, event, state);
	    break;
    }

}  // end of state_LOG_MSG3()



// We come here from LOG_MSG3, if uncoordinated checkpointing is enabled
// LOG_MSG4 --> WAIT on LOG_MSG_DONE event
// LOG_MSG4 --> LOG_MSG4 on RECEIVE event
void
Ghost_pattern::state_LOG_MSG4(pattern_event_t event)
{

    switch (event)   {
	case LOG_MSG_DONE:
	    // Send the fourth message
	    common->send(down, exchange_msg_len);

	    msg_wait_time_start= getCurrentSimTime();
	    state= WAIT;
	    state_WAIT(ENTER_WAIT);
	    break;

	case RECEIVE:
	    count_receives();
	    break;

	case FAIL:
	    break;

	case START:
	case COMPUTE_DONE:
	case CHCKPT_DONE:
	case ENV_LOG_DONE:
	case ENTER_WAIT:
	case BIT_BUCKET_WRITE_START:
	case BIT_BUCKET_WRITE_DONE:
	case BIT_BUCKET_READ_START:
	case BIT_BUCKET_READ_DONE:
	    _abort(ghost_pattern, "[%3d] Invalid event %d in state %d\n", my_rank, event, state);
	    break;
    }

}  // end of state_LOG_MSG4()



// WAIT --> WAIT on RECEIVE event
// WAIT --> SAVE_ENVELOPE if we have all four receives
void
Ghost_pattern::state_WAIT(pattern_event_t event)
{

    switch (event)   {
	case RECEIVE:
	    count_receives();

	    // Fall trhough to check whether we have all four messages and are done waiting

	case ENTER_WAIT:
	    if (done_waiting)   {
		done_waiting= false;
		msg_wait_time= msg_wait_time + getCurrentSimTime() - msg_wait_time_start;

		// Write the envlopes of the four messages we received to stable storage
		common->storage_write(envelope_size, ENV_LOG_DONE);
		common->storage_write(envelope_size, ENV_LOG_DONE);
		common->storage_write(envelope_size, ENV_LOG_DONE);
		common->storage_write(envelope_size, ENV_LOG_DONE);
		state= SAVE_ENVELOPE;
	    }
	    break;

	case FAIL:
	    break;

	case START:
	case COMPUTE_DONE:
	case CHCKPT_DONE:
	case LOG_MSG_DONE:
	case ENV_LOG_DONE:
	case BIT_BUCKET_WRITE_START:
	case BIT_BUCKET_WRITE_DONE:
	case BIT_BUCKET_READ_START:
	case BIT_BUCKET_READ_DONE:
	    _abort(ghost_pattern, "[%3d] Invalid event %d in state %d\n", my_rank, event, state);
	    break;
    }

}  // end of state_WAIT()



// SAVE_ENVELOPE --> SAVE_ENVELOPE on RECEIVE event
// SAVE_ENVELOPE --> COMPUTE on 4 ENV_LOG_DONE events and no checkpoint is needed
// SAVE_ENVELOPE --> CHCKPT on 4 ENV_LOG_DONE events and a checkpoint is needed
void
Ghost_pattern::state_SAVE_ENVELOPE(pattern_event_t event)
{

    switch (event)   {
	case RECEIVE:
	    count_receives();
	    break;

	case ENV_LOG_DONE:
	    save_ENVELOPE_cnt++;
	    if (save_ENVELOPE_cnt == 4)   {
		save_ENVELOPE_cnt= 0;

		// Is it time to write a checkpoint?
		if ((chckpt_method == CHCKPT_COORD) && (timestep_cnt % chckpt_steps == 0))   {
		    // Enter the checkpointing state
		    _GHOST_PATTERN_DBG(4, "[%3d] Wrote MSG envelopes, entering chckpt state\n",
			my_rank);
		    transition_to_CHCKPT();
		} else   {
		    _GHOST_PATTERN_DBG(4, "[%3d] Wrote MSG envelopes, entering compute state\n",
			my_rank);
		    transition_to_COMPUTE();
		}
	    }
	    break;

	case FAIL:
	    break;

	case START:
	case COMPUTE_DONE:
	case CHCKPT_DONE:
	case LOG_MSG_DONE:
	case ENTER_WAIT:
	case BIT_BUCKET_WRITE_START:
	case BIT_BUCKET_WRITE_DONE:
	case BIT_BUCKET_READ_START:
	case BIT_BUCKET_READ_DONE:
	    _abort(ghost_pattern, "[%3d] Invalid event %d in state %d\n", my_rank, event, state);
	    break;
    }

}  // end of state_SAVE_ENVELOPE



// CHCKPYT --> COMPUTE on CHCKPT_DONE event
void
Ghost_pattern::state_CHCKPT(pattern_event_t event)
{

    assert(chckpt_method != CHCKPT_NONE);

    switch (event)   {
	case RECEIVE:
	    // Another rank is way ahead of us and already sent us the next msg
	    // That's OK, we're not checkpointing the ghost cells.
	    count_receives();
	    break;

	case CHCKPT_DONE:
	    num_chckpts++;
	    chckpt_time= chckpt_time + getCurrentSimTime() - chckpt_segment_start;
	    fprintf(stderr, "=== %d got an answer back from my storage node\n", my_rank);

	    transition_to_COMPUTE();
	    _GHOST_PATTERN_DBG(4, "[%3d] Checkpoint write done, back to compute\n", my_rank);
	    break;

	case FAIL:
	    break;

	case START:
	case COMPUTE_DONE:
	case LOG_MSG_DONE:
	case ENV_LOG_DONE:
	case ENTER_WAIT:
	case BIT_BUCKET_WRITE_START:
	case BIT_BUCKET_WRITE_DONE:
	case BIT_BUCKET_READ_START:
	case BIT_BUCKET_READ_DONE:
	    _abort(ghost_pattern, "[%3d] Invalid event %d in state %d\n", my_rank, event, state);
	    break;
    }

}  // end of state_CHCKPT()



// DONE -->
void
Ghost_pattern::state_DONE(pattern_event_t event)
{

    _abort(ghost_pattern, "[%3d] Should not get anymore events after we are in DONE state\n",
	my_rank);

}  // end of state_DONE()



//
// -----------------------------------------------------------------------------
// Functions that help us transitions from one state to the next
//
void
Ghost_pattern::transition_to_COMPUTE(void)
{

SimTime_t delay;


    compute_segment_start= getCurrentSimTime();

    if (application_end_time - application_time_so_far > compute_time)   {
	// Do a full time step
	delay= compute_time;
    } else   {
	// Do the remaining work
	delay= application_end_time - application_time_so_far;
    }

    // Send ourselves a COMPUTE_DONE event
    state= COMPUTE;
    common->event_send(my_rank, COMPUTE_DONE, delay);

}  // end of transition_to_COMPUTE()



void
Ghost_pattern::transition_to_CHCKPT(void)
{

    // Start checkpointing
    chckpt_segment_start= getCurrentSimTime();
    state= CHCKPT;
    common->storage_write(chckpt_size, CHCKPT_DONE);
    _GHOST_PATTERN_DBG(4, "[%3d] Writing a checkpoint of size %d bytes\n",
	my_rank, chckpt_size);

}  // end of transition_to_CHCKPT()



//
// -----------------------------------------------------------------------------
// Other utility functions
//
void
Ghost_pattern::count_receives(void)
{

    total_rcvs++;
    rcv_cnt++;
    if (rcv_cnt == 4)   {
	timestep_cnt++;
	rcv_cnt= 0;
	done_waiting= true;
    }
    _GHOST_PATTERN_DBG(3, "[%3d] Got msg #%d from a neighbor\n", my_rank, rcv_cnt);

}  // end of count_receives()
