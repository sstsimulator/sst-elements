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

    _GHOST_PATTERN_DBG(2, "[%3d] got event %d at time %lu\n", my_rank, event, getCurrentSimTime());

    switch (event)   {
	case START:
	    handle_start();
	    break;
	case COMPUTE_DONE:
	    handle_compute_done();
	    break;
	case RECEIVE:
	    handle_receive(e->hops);
	    break;
	case CHCKPT_DONE:
	    handle_chckpt_done();
	    break;
	case FAIL:
	    handle_fail();
	    break;
    }

    delete(sst_event);

    if (application_done)   {
	// Subtract the checkpoint time from the overall time to get
	// execution time
	execution_time= execution_time - chckpt_time;

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
		case CHCKPT_RAID:
		    printf(">>> These checkpoints are not supported yet\n");
		    break;
	    }
	}
	unregisterExit();
    }

    return;

}  /* end of handle_events() */



void
Ghost_pattern::handle_start(void)
{

SimTime_t delay;


    switch (state)   {
	case INIT:
	    // Transition from INIT state to compute state
	    // Send ourselves a COMPUTE_DONE event
	    _GHOST_PATTERN_DBG(4, "[%3d] Starting, entering compute state\n",
		my_rank);

	    execution_time= getCurrentSimTime();
	    if (application_end_time - application_time_so_far > compute_time)   {
		// Do a full time step
		delay= compute_time;
	    } else   {
		// Do the remaining work
		delay= application_end_time - application_time_so_far;
	    }
	    compute_segment_start= getCurrentSimTime();
	    common->event_send(my_rank, COMPUTE_DONE, delay);
	    state= COMPUTE;
	    timestep_cnt++;

	    break;

	case COMPUTE:
	case WAIT:
	case DONE:
	case COORDINATED_CHCKPT:
	    // Should not happen
	    _abort(ghost_pattern, "[%3d] Invalid start event\n", my_rank);
	    break;
    }

}  // end of handle_start



void
Ghost_pattern::handle_chckpt_done(void)
{

SimTime_t delay;


    if (chckpt_method == CHCKPT_NONE)   {
	_abort(ghost_pattern, "[%3d] That's a bug!\n", my_rank);
    }

    switch (state)   {
	case COORDINATED_CHCKPT:
	    num_chckpts++;

	    // Back to computing
	    if (application_end_time - application_time_so_far > compute_time)   {
		// Do a full time step
		delay= compute_time;
	    } else   {
		// Do the remaining work
		delay= application_end_time - application_time_so_far;
	    }
	    common->event_send(my_rank, COMPUTE_DONE, delay);
	    chckpt_time= chckpt_time + getCurrentSimTime() - chckpt_segment_start;
	    state= COMPUTE;
	    _GHOST_PATTERN_DBG(4, "[%3d] Checkpoint done, back to compute state\n", my_rank);
	    break;

	case INIT:
	case COMPUTE:
	case WAIT:
	case DONE:
	    // Should not happen
	    _abort(ghost_pattern, "[%3d] Invalid checkpoint done event\n", my_rank);
	    break;
    }

}  // end of handle_chckpt_done



void
Ghost_pattern::handle_compute_done(void)
{

SimTime_t delay;


    switch (state)   {
	case COMPUTE:
	    _GHOST_PATTERN_DBG(4, "[%3d] Done computing, entering wait state\n", my_rank);
	    application_time_so_far += getCurrentSimTime() - compute_segment_start;
	    msg_wait_time_start= getCurrentSimTime();

	    if (application_time_so_far >= application_end_time)   {
		application_done= TRUE;
		execution_time= getCurrentSimTime() - execution_time;
		// There is no ghost cell exchange after the last computation

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
			state= COORDINATED_CHCKPT;
			chckpt_segment_start= getCurrentSimTime();
			common->event_send(my_rank, CHCKPT_DONE, chckpt_delay);
			_GHOST_PATTERN_DBG(4, "[%3d] Writing a checkpoint, back in %lu\n", my_rank,
			    chckpt_delay);

		    } else   {

			// Proceed to the next compute cycle
			if (application_end_time - application_time_so_far > compute_time)   {
			    // Do a full time step
			    delay= compute_time;
			} else   {
			    // Do the remaining work
			    delay= application_end_time - application_time_so_far;
			}
			compute_segment_start= getCurrentSimTime();
			common->event_send(my_rank, COMPUTE_DONE, delay);
			state= COMPUTE;
			_GHOST_PATTERN_DBG(4, "[%3d] No need to wait, back to compute state\n",
			    my_rank);
		    }

		} else   {
		    state= WAIT;
		}
	    }
	    break;

	case INIT:
	case WAIT:
	case DONE:
	case COORDINATED_CHCKPT:
	    // Should not happen
	    _abort(ghost_pattern, "[%3d] Invalid compute done event\n", my_rank);
	    break;
    }

}  // end of handle_compute_done



// We got a message from another rank
void
Ghost_pattern::handle_receive(int hops)
{

SimTime_t delay;


    switch (state)   {
	case COMPUTE:
	    // Simply count the receives and continue computing
	    rcv_cnt++;
	    if (rcv_cnt > 4)   {
		_abort(ghost_pattern, "[%3d] COMPUTE: We should never get more than 4 messages!\n",
		    my_rank);
	    }
	    _GHOST_PATTERN_DBG(3, "[%3d] Got msg #%d from neighbor with %d hops\n", my_rank,
		rcv_cnt, hops);

	    state= COMPUTE;
	    break;

	case WAIT:
	    // Count the receives. When we have all four, transition back to compute
	    rcv_cnt++;
	    if (rcv_cnt > 4)   {
		_abort(ghost_pattern, "[%3d] WAIT: We should never get more than 4 messages!\n",
		    my_rank);
	    }
	    _GHOST_PATTERN_DBG(3, "[%3d] Got msg #%d from neighbor with %d hops\n",
		my_rank, rcv_cnt, hops);
	    if (rcv_cnt == 4)   {
		rcv_cnt= 0;
		timestep_cnt++;
		msg_wait_time= msg_wait_time + getCurrentSimTime() - msg_wait_time_start;

		// Is it time to write a checkpoint?
		if ((chckpt_method == CHCKPT_COORD) && (timestep_cnt % chckpt_steps == 0))   {
		    // Enter the checkpointing state
		    state= COORDINATED_CHCKPT;
		    chckpt_segment_start= getCurrentSimTime();
		    common->event_send(my_rank, CHCKPT_DONE, chckpt_delay);
		    _GHOST_PATTERN_DBG(4, "[%3d] Writing a checkpoint, we'll be back in %lu\n",
			my_rank, chckpt_delay);

		} else   {

		    if (application_end_time - application_time_so_far > compute_time)   {
			// Do a full time step
			delay= compute_time;
		    } else   {
			// Do the remaining work
			delay= application_end_time - application_time_so_far;
		    }
		    compute_segment_start= getCurrentSimTime();
		    common->event_send(my_rank, COMPUTE_DONE, delay);
		    state= COMPUTE;
		    _GHOST_PATTERN_DBG(4, "[%3d] Done waiting, entering compute state\n",
			my_rank);
		}
	    }
	    break;

	case COORDINATED_CHCKPT:
	    // Another rank is way ahead of us and already sent us the next msg
	    // That's OK, we're not checkpointing the ghost cells, but can this
	    // really happen?
	    rcv_cnt++;
	    if (rcv_cnt > 4)   {
		_abort(ghost_pattern, "[%3d] COORDINATED_CHCKPT: We should never get more than "
		    "4 messages!\n", my_rank);
	    }
	    _GHOST_PATTERN_DBG(0, "[%3d] Got msg #%d from neighbor with %d hops while checkpointing\n", my_rank,
		rcv_cnt, hops);

	    state= COORDINATED_CHCKPT; //  Doesn't change
	    break;

	case INIT:
	case DONE:
	    // Should not happen
	    _abort(ghost_pattern, "[%3d] Invalid receive event\n", my_rank);
	    break;
    }

}  // end of handle_receive



void
Ghost_pattern::handle_fail(void)
{

    switch (state)   {
	case INIT:
	    break;
	case COMPUTE:
	    break;
	case WAIT:
	    break;
	case DONE:
	    break;
	case COORDINATED_CHCKPT:
	    break;
    }

}  // end of handle_fail



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
