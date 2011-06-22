//
// Copyright (c) 2011, IBM Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


/*



*/
#include <sst_config.h>
#include "sst/core/serialization/element.h"
#include "ping_pong.h"



void
Pingpong_pattern::handle_events(int sst_event)
{

pingpong_events_t event;


    event= (pingpong_events_t)sst_event;

    switch (state)   {
	case PP_INIT:
	    state_INIT(event);
	    break;
	case PP_RECEIVING:
	    state_RECEIVING(event);
	    break;
	case PP_BARRIER:
	    state_BARRIER(event);
	    break;
    }

    if (done)   {
	unregisterExit();
    }

}  /* end of handle_events() */


//
// Code for each possible state of the pingpong pattern
//
void
Pingpong_pattern::state_INIT(pingpong_events_t event)
{

    switch (event)   {
	case E_START:
	    cnt= num_msg;
	    done= false;
	    first_receive= true;
	    len= 0; // Starting length


	    if (my_rank == 0)   {
		printf("# [%3d] I'm at X,Y %3d/%-3d in the network, and x,y %3d/%-3d in the NoC\n",
			my_rank, myNetX(), myNetY(), myNoCX(), myNoCY());
		printf("# [%3d] Num msg per msg size: %d\n", my_rank, num_msg);
		start_time= getCurrentSimTime();

		// If I'm rank 0 send, otherwise wait
		data_send(dest, len, E_RECEIVE);
		state= PP_RECEIVING;

	    } else if (my_rank != dest)   {
		// I'm not participating in pingpong. Go straight to barrier
		state= PP_BARRIER;
		handle_events(E_BARRIER_ENTRY);

	    } else   {
		printf("# [%3d] I'm at X,Y %3d/%-3d in the network, and x,y %3d/%-3d in the NoC\n",
			my_rank, myNetX(), myNetY(), myNoCX(), myNoCY());
		state= PP_RECEIVING;
	    }
	    break;

	default:
	    _abort(pingpong_pattern, "[%3d] Invalid event %d in state %d\n", my_rank, event, state);
	    break;
    }

}  // end of state_INIT()



void
Pingpong_pattern::state_RECEIVING(pingpong_events_t event)
{

double execution_time;
double latency;


    switch (event)   {
	case E_RECEIVE:
	    // We're either rank 0 or dest. Others don't receive.
	    // Send it back, unless we're done
	    cnt--;
	    if (first_receive)   {
		// FIXME: I'd like to print how many hops the first message took
		// printf("# [%3d] Number of hops (routers) along path: %d\n", my_rank, e->hops);
		if (my_rank == 0)   {
		    printf("#\n");
		    printf("# Msg size (bytes)   Latency (seconds)\n");
		}
		first_receive= false;
	    }

	    if (my_rank != 0)   {
		// We always send back (to 0)
		data_send(0, len, E_RECEIVE);

		if (cnt < 1)   {
		    if (len > 0)   {
			len= len + len_inc;
		    } else   {
			len= len_inc;
		    }
		    cnt= num_msg;
		    if (len > end_len)   {
			state= PP_BARRIER;
			handle_events(E_BARRIER_ENTRY);
		    }
		}

	    } else   {
		// I'm rank 0
		if (cnt > 0)   {
		    data_send(dest, len, E_RECEIVE);
		} else   {
		    execution_time= (double)(getCurrentSimTime() - start_time) / 1000000000.0;
		    latency= execution_time / num_msg / 2.0;
		    printf("%9d %.9f\n", len, latency);
		    // Start next message size, if we haven't reached 0
		    if (len > 0)   {
			len= len + len_inc;
		    } else   {
			len= len_inc;
		    }
		    if (len > end_len)   {
			// We've done all sizes num_msg times
			state= PP_BARRIER;
			handle_events(E_BARRIER_ENTRY);
		    } else   {
			cnt= num_msg;
			start_time= getCurrentSimTime();
			data_send(dest, len, E_RECEIVE);
		    }
		}
	    }
	    break;

	default:
	    _abort(pingpong_pattern, "[%3d] Invalid event %d in state %d\n", my_rank, event, state);
	    break;
    }

}  // end of state_RECEIVING()



void
Pingpong_pattern::state_BARRIER(pingpong_events_t event)
{

    switch (event)   {
	case E_BARRIER_ENTRY:
	    // This will be delivered to us after barrier switches back to us
	    self_event_send(E_BARRIER_EXIT);

	    // Switch the state machine to the start of the barrier SM
	    SM_transition(SMbarrier, Barrier_pattern::E_START);
	    break;

	case E_BARRIER_EXIT:
	    // We just came back from the barrier SM. We're done
	    done= true;
	    break;

	default:
	    _abort(pingpong_pattern, "[%3d] Invalid event %d in state %d\n", my_rank, event, state);
	    break;
    }

}  // end of state_BARRIER()



extern "C" {
Pingpong_pattern *
pingpong_patternAllocComponent(SST::ComponentId_t id,
                          SST::Component::Params_t& params)
{
    return new Pingpong_pattern(id, params);
}
}

BOOST_CLASS_EXPORT(Pingpong_pattern)
