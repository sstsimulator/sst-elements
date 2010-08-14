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
SimTime_t delay;


    // Extract the pattern event type from the SST event
    // (We are "misusing" the routine filed in CPUNicEvent to xmit the event type
    e= static_cast<CPUNicEvent *>(sst_event);
    if (e->hops > 2)   {
	_abort(ghost_pattern, "[%3d] No message should travel through more than two routers!\n",
	    my_rank);
    }
    event= (pattern_event_t)e->GetRoutine();

    if (application_done)   {
	_GHOST_PATTERN_DBG(0, "[%3d] There should be no more events! (%d)\n",
	    my_rank, event);
	return;
    }

    _GHOST_PATTERN_DBG(2, "[%3d] got event %d at time %lu\n", my_rank, event, getCurrentSimTime());

    switch (state)   {
	case INIT:
	    /* Wait for the start signal */
	    state= init_state(state, event);
	    break;

	case COMPUTE:
	    switch (event)   {
		case COMPUTE_DONE:
		    _GHOST_PATTERN_DBG(4, "[%3d] Done computing, entering wait state\n", my_rank);
		    application_time_so_far += getCurrentSimTime() - compute_segment_start;
		    if (application_time_so_far >= application_end_time)   {
			application_done= TRUE;
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
		case RECEIVE:
		    /* We got a message from another rank */
		    rcv_cnt++;
		    if (rcv_cnt > 4)   {
			_abort(ghost_pattern, "[%3d] COMPUTE: We should never get more than 4 messages!\n",
			    my_rank);
		    }
		    _GHOST_PATTERN_DBG(3, "[%3d] Got msg #%d from neighbor with %d hops\n", my_rank,
			rcv_cnt, e->hops);
		    break;
		case START:
		case FAIL:
		case RESEND_MSG:
		    // Should not happen
		    _abort(ghost_pattern, "[%3d] Invalid event in COMPUTE\n", my_rank);
		    break;
	    }
	    break;

	case WAIT:
	    /* We are waiting for messages from our four neighbors */
	    switch (event)   {
		case START:
		case COMPUTE_DONE:
		    // Doesn't make sense; we should not be getting these events
		    _abort(ghost_pattern, "[%3d] Invalid event in WAIT\n", my_rank);
		    break;
		case RECEIVE:
		    /* YES! We got a message from another rank */
		    rcv_cnt++;
		    if (rcv_cnt > 4)   {
			_abort(ghost_pattern, "[%3d] WAIT: We should never get more than 4 messages!\n",
			    my_rank);
		    }
		    _GHOST_PATTERN_DBG(3, "[%3d] Got msg #%d from neighbor with %d hops\n",
			my_rank, rcv_cnt, e->hops);
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
		case FAIL:
		    /* We just failed. Deal with it! */
		    break;
		case RESEND_MSG:
		    /* We have been asked to resend a previous msg to help another rank to recover */
		    break;
	    }
	    break;

	case DONE:
	    // We're done, and just waiting for the program to exit.
	    // This state is not really needed
	    break;
    }

    delete(sst_event);

    if (application_done)   {
	_GHOST_PATTERN_DBG(0, "[%3d] Application has done %.9fs of work in %d time steps\n",
	    my_rank, (double)application_time_so_far /  1000000000.0, timestep_cnt);
	unregisterExit();
    }

    return;

}  /* end of handle_events() */



//
// Transition from INIT state to new state state
//
state_t
Ghost_pattern::init_state(state_t state, pattern_event_t event)
{

SimTime_t delay;


    switch (event)   {
	case START:
	    // Send ourselves a COMPUTE_DONE event
	    _GHOST_PATTERN_DBG(4, "[%3d] Starting, entering compute state\n",
		my_rank);
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

	case COMPUTE_DONE:
	case RECEIVE:
	case FAIL:
	case RESEND_MSG:
	    // Should not happen
	    _abort(ghost_pattern, "[%3d] Invalid event in INIT\n", my_rank);
	    break;
    }

    return state;

}  // end of init_state



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
