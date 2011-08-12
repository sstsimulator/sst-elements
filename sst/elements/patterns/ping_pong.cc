//
// Copyright (c) 2011, IBM Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


/*

A simple ping-pong test between rank 0 and rank n/2 (the assumption
is that n/2 is fardest from rank 0 in the underlying network
topology). Each message length is done num_msg times. Rank 0
calculates the one-way latency and bandwidth. The message size is
then incremented by len_inc bytes and another round of num_msg trials
starts.  The test ends when the message size end_len is reached.

The following are configuration file input parameters: destination,
num_msg, end_len, and len_inc.

The barrier at the end is there only for testing of the barrier
iteself and the gate keeper mechanism in comm_pattern.cc

*/
#include <sst_config.h>
#include "sst/core/serialization/element.h"
#include "ping_pong.h"



void
Pingpong_pattern::handle_events(state_event sm_event)
{

pingpong_events_t event;


    event= (pingpong_events_t)sm_event.event;

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
	case PP_ALLREDUCE:
	    state_ALLREDUCE(sm_event);
	    break;
	case PP_DONE:
	    // Not really a state we need
	    break;
    }

    if (done)   {
	unregisterExit();
	done= false;
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
		printf("#  |||  Ping pong between ranks 0 and %d\n", dest);
		printf("#  |||  Number of messages per each size: %d\n", num_msg);
		printf("# [%3d] PING I'm at X,Y %3d/%-3d in the network, and x,y %3d/%-3d in the NoC\n",
			my_rank, myNetX(), myNetY(), myNoCX(), myNoCY());
		start_time= getCurrentSimTime();

		// If I'm rank 0 send, otherwise wait
		data_send(dest, len, E_RECEIVE);
		state= PP_RECEIVING;

	    } else if (my_rank != dest)   {
		// I'm not participating in pingpong. Go straight to barrier
		state_transition(E_BARRIER_ENTRY, PP_BARRIER);

	    } else   {
		printf("# [%3d] PONG I'm at X,Y %3d/%-3d in the network, and x,y %3d/%-3d in the NoC\n",
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
			state_transition(E_BARRIER_ENTRY, PP_BARRIER);
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
		    // Start next message size, if we haven't reached the end yet
		    if (len > 0)   {
			len= len + len_inc;
		    } else   {
			len= len_inc;
		    }
		    if (len > end_len)   {
			// We've done all sizes num_msg times
			state_transition(E_BARRIER_ENTRY, PP_BARRIER);
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

state_event enter_barrier, exit_barrier;


    switch (event)   {
	case E_BARRIER_ENTRY:
	    // Set the parameters to be passed to the barrier SM
	    enter_barrier.event= SM_START_EVENT;

	    // We want to be called with this event, when allreduce returns
	    exit_barrier.event= E_BARRIER_EXIT;

	    SM->SM_call(SMbarrier, enter_barrier, exit_barrier);
	    break;

	case E_BARRIER_EXIT:
	    // We just came back from the barrier SM. Go do an allreduce
	    state_transition(E_ALLREDUCE_ENTRY, PP_ALLREDUCE);
	    break;

	default:
	    _abort(pingpong_pattern, "[%3d] Invalid event %d in state %d\n", my_rank, event, state);
	    break;
    }

}  // end of state_BARRIER()



void
Pingpong_pattern::state_ALLREDUCE(state_event event)
{

state_event enter_allreduce, exit_allreduce;
double check= 0.0;


    switch (event.event)   {
	case E_ALLREDUCE_ENTRY:
	    // Set the parameters to be passed to the allreduce SM
	    enter_allreduce.event= SM_START_EVENT;
	    enter_allreduce.set_Fdata(my_rank + 1.0);

	    // We want to be called with this event, when allreduce returns
	    exit_allreduce.event= E_ALLREDUCE_EXIT;

	    SM->SM_call(SMallreduce, enter_allreduce, exit_allreduce);
	    break;

	case E_ALLREDUCE_EXIT:
	    // We just came back from the allreduce SM. We're done

	    for (int i= 0; i < num_ranks; i++)   {
		check= check + i + 1.0;
	    }

	    if (my_rank == 0)   {
		if (check == event.get_Fdata())   {
		    printf("# [%3d] Allreduce test passed\n", my_rank);
		} else   {
		    printf("# [%3d] Allreduce test failed: %6.3f != %6.3f\n", my_rank,
			event.get_Fdata(), check);
		}
	    } else   {
		// Only report errors
		if (check != event.get_Fdata())   {
		    printf("# [%3d] Allreduce test failed: %6.3f != %6.3f\n", my_rank,
			event.get_Fdata(), check);
		}
	    }
	    state= PP_DONE;
	    done= true;
	    break;

	default:
	    _abort(pingpong_pattern, "[%3d] Invalid event %d in state %d\n", my_rank, event.event, state);
	    break;
    }

}  // end of state_ALLREDUCE()



extern "C" {
Pingpong_pattern *
pingpong_patternAllocComponent(SST::ComponentId_t id,
                          SST::Component::Params_t& params)
{
    return new Pingpong_pattern(id, params);
}
}

BOOST_CLASS_EXPORT(Pingpong_pattern)
