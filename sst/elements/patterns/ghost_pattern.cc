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
	case WRITE_CHCKPT:
	    handle_write_chckpt();
	    break;
	case FAIL:
	    handle_fail();
	    break;
	case RESEND_MSG:
	    handle_resend_msg();
	    break;
    }

    delete(sst_event);

    if (application_done)   {
	if (my_rank == 0)   {
	    printf("||| Application has done %.9fs of work in %d time steps\n",
		(double)application_time_so_far /  1000000000.0, timestep_cnt);
	    printf("||| Execution time %.9fs\n",
		(double)execution_time /  1000000000.0);
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

	    // Schedule the first coordinated checkpoint
	    if (chckpt_method == CHCKPT_COORD)   {
		// For ghost we want to schedule checkpoints at timestep
		// intervals.
		// FIXME: Continue working here
		// common->event_send(my_rank, WRITE_CHCKPT, delay);
	    }
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
Ghost_pattern::handle_compute_done(void)
{

SimTime_t delay;


    switch (state)   {
	case COMPUTE:
	    _GHOST_PATTERN_DBG(4, "[%3d] Done computing, entering wait state\n", my_rank);
	    application_time_so_far += getCurrentSimTime() - compute_segment_start;
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
		    // We already have our for neighbor messages; no need to wait
		    rcv_cnt= 0;
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
		    _GHOST_PATTERN_DBG(4, "[%3d] No need to wait, back to compute state\n",
			my_rank);
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
		_GHOST_PATTERN_DBG(4, "[%3d] Done waiting, entering compute state\n",
		    my_rank);
	    }
	    break;

	case INIT:
	case DONE:
	case COORDINATED_CHCKPT:
	    // Should not happen
	    _abort(ghost_pattern, "[%3d] Invalid receive event\n", my_rank);
	    break;
    }

}  // end of handle_receive



void
Ghost_pattern::handle_write_chckpt(void)
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

}  // end of handle_write_chckpt



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



void
Ghost_pattern::handle_resend_msg(void)
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

}  // end of handle_resend_msg



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
