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
#include "ghost_pattern.h"
#include "pattern_common.h"



void
Ghost_pattern::handle_events(Event *sst_event)
{

CPUNicEvent *e;
pattern_event_t event;


    // Extract the pattern event type from the SST event
    // (We are "misusing" the routine filed in CPUNicEvent to xmit the event type
    e= static_cast<CPUNicEvent *>(sst_event);
    if (e->hops > 2)   {
	_abort(ghost_pattern, "[%3d] No message should travel through more than two routers! %d\n",
	    my_rank, e->hops);
    }

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
	case COORDINATED_CHCKPT:
	    state_COORDINATED_CHCKPT(event);
	    break;
	case SAVING_ENVELOPE_1:
	    state_SAVING_ENVELOPE_1(event);
	    break;
	case SAVING_ENVELOPE_2:
	    state_SAVING_ENVELOPE_2(event);
	    break;
	case SAVING_ENVELOPE_3:
	    state_SAVING_ENVELOPE_3(event);
	    break;
    }

    delete(sst_event);

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



// When we send to ourselves, we come here.
// Just pass it on to the main handler above
void
Ghost_pattern::handle_self_events(Event *e)
{
    handle_events(e);
}  /* end of handle_self_events() */



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
//
void
Ghost_pattern::state_INIT(pattern_event_t event)
{

    switch (event)   {
	case START:
	    _GHOST_PATTERN_DBG(4, "[%3d] Starting, entering compute state\n", my_rank);
	    execution_time= getCurrentSimTime();
	    transition_to_COMPUTE();
	    break;

	case FAIL:
	    break;

	case WRITE_ENVELOPE_DONE:
	case COMPUTE_DONE:
	case RECEIVE:
	case CHCKPT_DONE:
	    _abort(ghost_pattern, "[%3d] Invalid event %d in state %d\n", my_rank, event, state);
	    break;
    }

}  // end of state_INIT()



void
Ghost_pattern::state_COMPUTE(pattern_event_t event)
{

    switch (event)   {
	case COMPUTE_DONE:
	    _GHOST_PATTERN_DBG(4, "[%3d] Done computing\n", my_rank);
	    application_time_so_far += getCurrentSimTime() - compute_segment_start;

	    if (application_time_so_far >= application_end_time)   {
		application_done= TRUE;
		execution_time= getCurrentSimTime() - execution_time;
		// There is no ghost cell exchange after the last computation
		state= DONE;
		timestep_cnt++;

	    } else   {

		// Tell our neighbors what we have computed
		common->send(right, exchange_msg_len);
		common->send(left, exchange_msg_len);
		common->send(up, exchange_msg_len);
		common->send(down, exchange_msg_len);

		if (rcv_cnt == 4)   {
		    rcv_cnt= 0;
		    timestep_cnt++;
		    msg_wait_time= msg_wait_time + getCurrentSimTime() - msg_wait_time_start;

		    // Is it time to write a checkpoint?
		    if ((chckpt_method == CHCKPT_COORD) && (timestep_cnt % chckpt_steps == 0))   {
			// Enter the checkpointing state
			transition_to_COORDINATED_CHCKPT();
		    } else   {
			transition_to_COMPUTE();
			_GHOST_PATTERN_DBG(4, "[%3d] No need to wait, back to compute state\n",
			    my_rank);
		    }

		} else   {
		    // We don't have all four messages yet. Go into wait state
		    msg_wait_time_start= getCurrentSimTime();
		    state= WAIT;
		}
	    }
	    break;

	case RECEIVE:
	    count_receives();

	    if (chckpt_method == CHCKPT_UNCOORD)   {
		// We'll have to log this message and return to compute
		application_time_so_far += getCurrentSimTime() - compute_segment_start;
		state= SAVING_ENVELOPE_1;
		common->event_send(my_rank, WRITE_ENVELOPE_DONE, envelope_write_time);
	    }

	    break;

	case FAIL:
	    break;

	case START:
	case CHCKPT_DONE:
	case WRITE_ENVELOPE_DONE:
	    _abort(ghost_pattern, "[%3d] Invalid event %d in state %d\n", my_rank, event, state);
	    break;
    }
 
}  // end of state_COMPUTE()



void
Ghost_pattern::state_WAIT(pattern_event_t event)
{

    switch (event)   {
	case RECEIVE:
	    // Count the receives. When we have all four, transition back to compute
	    count_receives();

	    if (chckpt_method == CHCKPT_UNCOORD)   {
		// We'll have to log this message and return to here
		state= SAVING_ENVELOPE_2;
		common->event_send(my_rank, WRITE_ENVELOPE_DONE, envelope_write_time);
	    }

	    if (rcv_cnt == 4)   {
		rcv_cnt= 0;
		timestep_cnt++;
		msg_wait_time= msg_wait_time + getCurrentSimTime() - msg_wait_time_start;

		// Is it time to write a checkpoint?
		if ((chckpt_method == CHCKPT_COORD) && (timestep_cnt % chckpt_steps == 0))   {
		    // Enter the checkpointing state
		    transition_to_COORDINATED_CHCKPT();
		} else   {
		    transition_to_COMPUTE();
		    _GHOST_PATTERN_DBG(4, "[%3d] Done waiting, entering compute state\n",
			my_rank);
		}
	    }
	    break;

	case FAIL:
	    break;

	case START:
	case COMPUTE_DONE:
	case CHCKPT_DONE:
	case WRITE_ENVELOPE_DONE:
	    _abort(ghost_pattern, "[%3d] Invalid event %d in state %d\n", my_rank, event, state);
	    break;
    }
 
}  // end of state_WAIT()



void
Ghost_pattern::state_DONE(pattern_event_t event)
{

    _abort(ghost_pattern, "[%3d] Should not get anymore events after we are in DONE state\n",
	my_rank);
 
}  // end of state_DONE()



void
Ghost_pattern::state_COORDINATED_CHCKPT(pattern_event_t event)
{

    if (chckpt_method == CHCKPT_NONE)   {
	_abort(ghost_pattern, "[%3d] That's a bug!\n", my_rank);
    }

    switch (event)   {
	case RECEIVE:
	    // Another rank is way ahead of us and already sent us the next msg
	    // That's OK, we're not checkpointing the ghost cells.
	    count_receives();

	    if (chckpt_method == CHCKPT_UNCOORD)   {
		// We'll have to log this message and return to here and finish checkpoitning
		// FIXME: Are we going to use this method to write local checkpoints as well?
		state= SAVING_ENVELOPE_3;
		chckpt_time= chckpt_time + getCurrentSimTime() - chckpt_segment_start;
		chckpt_done_interrupted++;
		common->event_send(my_rank, WRITE_ENVELOPE_DONE, envelope_write_time);
	    }
	    break;

	case CHCKPT_DONE:
	    // I may not be completly done:
	    if (chckpt_done_interrupted > 0)   {
		// Ignore this event. There is another in the pipeline that was
		// sent by state_SAVING_ENVELOPE_3() when they interrupted us
		chckpt_done_interrupted--;
	    } else   {
		num_chckpts++;
		chckpt_time= chckpt_time + getCurrentSimTime() - chckpt_segment_start;

		transition_to_COMPUTE();
		_GHOST_PATTERN_DBG(4, "[%3d] Checkpoint done, back to compute state\n", my_rank);
	    }
	    break;

	case FAIL:
	    break;

	case COMPUTE_DONE:
	case START:
	case WRITE_ENVELOPE_DONE:
	    _abort(ghost_pattern, "[%3d] Invalid event %d in state %d\n", my_rank, event, state);
	    break;
    }
 
}  // end of state_COORDINATED_CHCKPT()



void
Ghost_pattern::state_SAVING_ENVELOPE_1(pattern_event_t event)
{

    switch (event)   {
	case START:
	case COMPUTE_DONE:
	case RECEIVE:
	case CHCKPT_DONE:
	    _abort(ghost_pattern, "[%3d] Invalid event %d in state %d\n", my_rank, event, state);
	    break;

	case FAIL:
	    break;

	case WRITE_ENVELOPE_DONE:
	    // Go back to COMPUTE
	    break;
    }
 
}  // end of state_SAVING_ENVELOPE_1()



void
Ghost_pattern::state_SAVING_ENVELOPE_2(pattern_event_t event)
{

    switch (event)   {
	case START:
	case COMPUTE_DONE:
	case RECEIVE:
	case CHCKPT_DONE:
	    _abort(ghost_pattern, "[%3d] Invalid event %d in state %d\n", my_rank, event, state);
	    break;

	case FAIL:
	    break;

	case WRITE_ENVELOPE_DONE:
	    // Go back to WAIT
	    break;
    }
 
}  // end of state_SAVING_ENVELOPE_2()



void
Ghost_pattern::state_SAVING_ENVELOPE_3(pattern_event_t event)
{

SimTime_t chckpt_delay_remainder;


    switch (event)   {
	case START:
	case COMPUTE_DONE:
	    _abort(ghost_pattern, "[%3d] Invalid event %d in state %d\n", my_rank, event, state);
	    break;

	case RECEIVE:
	    _abort(ghost_pattern, "[%3d] I need to handle this case too!\n", my_rank);
	    break;

	case CHCKPT_DONE:
	    // We're absorbing an earlier CHCKPT_DONE event. It was sent before
	    // we knew we would spend time in this state
	    chckpt_done_interrupted--;
	    break;

	case FAIL:
	    break;

	case WRITE_ENVELOPE_DONE:
	    // Go back to writing checkpoint
	    chckpt_segment_start= getCurrentSimTime();
	    state= COORDINATED_CHCKPT;
	    chckpt_delay_remainder= chckpt_delay -
		(getCurrentSimTime() - envelope_write_time - chckpt_segment_start);
	    common->event_send(my_rank, CHCKPT_DONE, chckpt_delay_remainder);
	    break;
    }
 
}  // end of state_SAVING_ENVELOPE_3()



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
Ghost_pattern::transition_to_COORDINATED_CHCKPT(void)
{

    // Start checkpointing
    chckpt_segment_start= getCurrentSimTime();
    state= COORDINATED_CHCKPT;
    common->event_send(my_rank, CHCKPT_DONE, chckpt_delay);
    _GHOST_PATTERN_DBG(4, "[%3d] Writing a checkpoint, we'll be back in %lu\n",
	my_rank, chckpt_delay);

}  // end of Ghost_pattern::transition_to_COORDINATED_CHCKPT()



//
// -----------------------------------------------------------------------------
// Other utility functions
//
void
Ghost_pattern::count_receives(void)
{

    rcv_cnt++;
    if (rcv_cnt > 4)   {
	_abort(ghost_pattern, "[%3d] COMPUTE: We should never get more than 4 messages!\n",
	    my_rank);
    }
    _GHOST_PATTERN_DBG(3, "[%3d] Got msg #%d from a neighbor\n", my_rank, rcv_cnt);

}  // end of count_receives()
