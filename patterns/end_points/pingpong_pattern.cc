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
topology). Each message length is done num_msgs times. Rank 0
calculates the one-way latency and bandwidth. The message size is
then incremented by len_inc bytes and another round of num_msgs trials
starts.  The test ends when the message size end_len is reached.

The following are configuration file input parameters: destination,
num_msgs, end_len, and len_inc.

The barrier at the end is there only for testing of the barrier
iteself and the gate keeper mechanism in comm_pattern.cc

*/
#include <sst_config.h>
#include "pingpong_pattern.h"
#include <sst/core/element.h>
#include <sst/core/component.h>



void
Pingpong_pattern::handle_events(state_event sm_event)
{

    switch (state)   {
	case PP_INIT:
	    state_INIT(sm_event);
	    break;
	case PP_RECEIVING:
	    state_RECEIVING(sm_event);
	    break;
	case PP_BARRIER:
	    state_BARRIER(sm_event);
	    break;
	case PP_ALLREDUCE:
	    state_ALLREDUCE(sm_event);
	    break;
	case PP_DONE:
	    // Not really a state we need
	    break;
    }

    if (done)   {
  primaryComponentOKToEndSim();
	done= false;
    }

}  /* end of handle_events() */


//
// Code for each possible state of the pingpong pattern
//
void
Pingpong_pattern::state_INIT(state_event sm_event)
{

pingpong_events_t e= (pingpong_events_t)sm_event.event;
state_event pp_event;


    switch (e)   {
	case E_START:
	    cnt= num_msgs;
	    done= false;
	    first_receive= true;
	    len= 0; // Starting length


	    if (my_rank == 0)   {
		printf("#  |||  Ping pong between ranks 0 and %d\n", dest);
		printf("#  |||  Number of messages per each size: %d\n", num_msgs);
		printf("# [%3d] PING I (%d of %d on node %d) am at X,Y,Z %3d/%3d/%3d in the network, "
		    "and x,y %3d/%3d/%3d in the NoC\n",
			my_rank, my_rank % machine->get_cores_per_node(), machine->get_cores_per_node(),
			machine->myNode(), machine->myNetX(), machine->myNetY(), machine->myNetZ(),
			machine->myNoCX(), machine->myNoCY(), machine->myNoCZ());
		start_time= getCurrentSimTime();

		// If I'm rank 0 send, otherwise wait
		pp_event.event= E_RECEIVE;
		send_msg(dest, len, pp_event);
		state= PP_RECEIVING;

	    } else if (my_rank != dest)   {
		// I'm not participating in pingpong. Go straight to barrier
		goto_state(state_BARRIER, PP_BARRIER, E_BARRIER_ENTRY);

	    } else   {
		printf("# [%3d] PONG I (%d of %d on node %d) am at X,Y,Z %3d/%3d/%3d in the network, "
		    "and x,y %3d/%3d/%3d in the NoC\n",
			my_rank, my_rank % machine->get_cores_per_node(), machine->get_cores_per_node(),
			machine->myNode(), machine->myNetX(), machine->myNetY(), machine->myNetZ(),
			machine->myNoCX(), machine->myNoCY(), machine->myNoCZ());
		state= PP_RECEIVING;
	    }
	    break;

	default:
	    out.fatal(CALL_INFO, -1, "[%3d] Invalid event %d in state %d\n", my_rank, e, state);
	    break;
    }

}  // end of state_INIT()



void
Pingpong_pattern::state_RECEIVING(state_event sm_event)
{

pingpong_events_t e= (pingpong_events_t)sm_event.event;
state_event pp_event;
double execution_time;
double latency;


    switch (e)   {
	case E_RECEIVE:
	    // We're either rank 0 or dest. Others don't receive.
	    // Send it back, unless we're done
	    cnt--;
	    if (first_receive)   {
		if (my_rank == 0)   {
		    printf("#\n");
		    printf("# Msg size (bytes)   Latency (micro seconds)\n");
		}
		first_receive= false;
	    }

	    if (my_rank != 0)   {
		// We always send back (to 0)
		pp_event.event= E_RECEIVE;
		send_msg(0, len, pp_event);

		if (cnt < 1)   {
		    if (len > 0)   {
			if (user_len_inc <= 0)   {
			    if (len >= (len_inc << 4))   {
				len_inc= len_inc << 1;
			    }
			    len= len + len_inc;
			} else   {
			    len= len + user_len_inc;
			}

		    } else   {
			if (user_len_inc <= 0)   {
			    len= len_inc;
			} else   {
			    len= user_len_inc;
			}
		    }
		    cnt= num_msgs;
		    if (len > end_len)   {
			goto_state(state_BARRIER, PP_BARRIER, E_BARRIER_ENTRY);
		    }
		}

	    } else   {
		// I'm rank 0
		if (cnt > 0)   {
		    pp_event.event= E_RECEIVE;
		    send_msg(dest, len, pp_event);
		} else   {
		    execution_time= SimTimeToD(getCurrentSimTime() - start_time);
		    latency= execution_time / num_msgs / 2.0;
		    printf("%9d %.9f\n", len, latency * 1000000.0);
		    // Start next message size, if we haven't reached the end yet
		    if (len > 0)   {
			if (user_len_inc <= 0)   {
			    if (len >= (len_inc << 4))   {
				len_inc= len_inc << 1;
			    }
			    len= len + len_inc;
			} else   {
			    len= len + user_len_inc;
			}

		    } else   {
			if (user_len_inc <= 0)   {
			    len= len_inc;
			} else   {
			    len= user_len_inc;
			}
		    }
		    if (len > end_len)   {
			// We've done all sizes num_msgs times
			goto_state(state_BARRIER, PP_BARRIER, E_BARRIER_ENTRY);
		    } else   {
			cnt= num_msgs;
			start_time= getCurrentSimTime();
			pp_event.event= E_RECEIVE;
			send_msg(dest, len, pp_event);
		    }
		}
	    }
	    break;

	default:
	    out.fatal(CALL_INFO, -1, "[%3d] Invalid event %d in state %d\n", my_rank, e, state);
	    break;
    }

}  // end of state_RECEIVING()



void
Pingpong_pattern::state_BARRIER(state_event sm_event)
{

pingpong_events_t e= (pingpong_events_t)sm_event.event;
state_event enter_barrier, exit_barrier;


    switch (e)   {
	case E_BARRIER_ENTRY:
	    // Set the parameters to be passed to the barrier SM
	    enter_barrier.event= SM_START_EVENT;

	    // We want to be called with this event, when barrier returns
	    exit_barrier.event= E_BARRIER_EXIT;

	    SM->SM_call(SMbarrier, enter_barrier, exit_barrier);
	    break;

	case E_BARRIER_EXIT:
	    // We just came back from the barrier SM. Go do an allreduce
	    goto_state(state_ALLREDUCE, PP_ALLREDUCE, E_ALLREDUCE_ENTRY);
	    break;

	default:
	    out.fatal(CALL_INFO, -1, "[%3d] Invalid event %d in state %d\n", my_rank, e, state);
	    break;
    }

}  // end of state_BARRIER()



void
Pingpong_pattern::state_ALLREDUCE(state_event sm_event)
{

pingpong_events_t e= (pingpong_events_t)sm_event.event;
state_event enter_allreduce, exit_allreduce;
double check= 0.0;


    switch (e)   {
	case E_ALLREDUCE_ENTRY:
	    // Set the parameters to be passed to the allreduce SM
	    enter_allreduce.event= SM_START_EVENT;
	    enter_allreduce.set_Fdata(my_rank + 1.0);
	    enter_allreduce.set_Idata(Allreduce_op::OP_SUM);

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
		if (check == sm_event.get_Fdata())   {
		    printf("# [%3d] Allreduce test of length %d passed\n", my_rank,
			allreduce_msglen);
		} else   {
		    printf("# [%3d] Allreduce test failed: %6.3f != %6.3f\n", my_rank,
			sm_event.get_Fdata(), check);
		}
	    } else   {
		// Only report errors
		if (check != sm_event.get_Fdata())   {
		    printf("# [%3d] Allreduce test failed: %6.3f != %6.3f\n", my_rank,
			sm_event.get_Fdata(), check);
		}
	    }
	    state= PP_DONE;
	    done= true;
	    break;

	default:
	    out.fatal(CALL_INFO, -1, "[%3d] Invalid event %d in state %d\n", my_rank, e, state);
	    break;
    }

}  // end of state_ALLREDUCE()



/*
// THIS SECTION MOVED TO patterns.cc FOR CONFIG CHANGE TO ONE LIBRARY FILE - ALEVINE

eli(Pingpong_pattern, pingpong_pattern, "Ping-pong pattern")
*/

// ADDED FOR PROPER INITIALIZATION - ALEVINE
// SST Startup and Shutdown
void Pingpong_pattern::setup() 
{
	// Call the initial State Transition
	state_transition(E_START, PP_INIT);
}

