// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
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
#include "ghost_pattern.h"

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS	(1)
#endif
#include <inttypes.h>		/* For PRId64 */

#include <sst/core/element.h>

void
Ghost_pattern::handle_events(state_event sm_event)
{

    switch (state)   {
	case STATE_INIT:		state_INIT(sm_event); break;
	case STATE_CELL_EXCHANGE:	state_CELL_EXCHANGE(sm_event); break;
	case STATE_COMPUTE_CELLS:	state_COMPUTE_CELLS(sm_event); break;
	case STATE_REDUCE:		state_REDUCE(sm_event); break;
	case STATE_DONE:		state_DONE(sm_event); break;
    }

    if (done)   {
	done= false;
  primaryComponentOKToEndSim();
    }

}  /* end of handle_events() */



void
Ghost_pattern::state_INIT(state_event sm_event)
{

ghost_events_t e= (ghost_events_t)sm_event.event;


    switch (e)   {
	case E_START:
	    // No break!

	case E_NEXT_LOOP:
	    t++;
	    if (t < time_steps)   {
		goto_state(state_CELL_EXCHANGE, STATE_CELL_EXCHANGE, E_CELL_SEND);
	    } else   {
		// We are done
		if (rcv_cnt == neighbor_cnt * time_steps)   {
		    goto_state(state_DONE, STATE_DONE, E_DONE);
		} else   {
		    // We are done, but some receives are still in flight
fprintf(stderr, "[%3d] I'm going to wait for missing receives: rcv_cnt %d\n", my_rank, rcv_cnt);
		    wait_last_receives= true;
		}
	    }
	    break;

	case E_CELL_RECEIVE:
	    // Some guys are fast (or slow)
	    rcv_cnt++;
	    if (rcv_cnt == neighbor_cnt * time_steps)   {
fprintf(stderr, "[%3d] Got the last receive: rcv_cnt %d\n", my_rank, rcv_cnt);
		goto_state(state_DONE, STATE_DONE, E_DONE);
	    }
	    break;

	default:
	    out.fatal(CALL_INFO, -1, "[%3d] Invalid event %d in state %d\n", my_rank, e, state);
	    break;
    }

}  // end of state_INIT()



void
Ghost_pattern::state_CELL_EXCHANGE(state_event sm_event)
{

ghost_events_t e= (ghost_events_t)sm_event.event;
state_event send_event;


    switch (e)   {
	case E_CELL_SEND:
	    comm_time_start= getCurrentSimTime();
	    send_event.event= E_CELL_RECEIVE;
	    if (TwoD)   {
		// Send lines
		send_msg(neighbor_list.up, memory.x_dim * sizeof(double), send_event, E_SEND_DONE);
		send_msg(neighbor_list.down, memory.x_dim * sizeof(double), send_event, E_SEND_DONE);
		send_msg(neighbor_list.left, memory.y_dim * sizeof(double), send_event, E_SEND_DONE);
		send_msg(neighbor_list.right, memory.y_dim * sizeof(double), send_event, E_SEND_DONE);
		bytes_sent += 2 * memory.x_dim * sizeof(double) +
		    2 * memory.y_dim * sizeof(double);
		num_sends += 4;
	    } else   {
		// Send surfaces
		send_msg(neighbor_list.up, memory.x_dim * memory.y_dim * sizeof(double), send_event, E_SEND_DONE);
		send_msg(neighbor_list.down, memory.x_dim * memory.y_dim * sizeof(double), send_event, E_SEND_DONE);
		send_msg(neighbor_list.left, memory.y_dim * memory.z_dim * sizeof(double), send_event, E_SEND_DONE);
		send_msg(neighbor_list.right, memory.y_dim * memory.z_dim * sizeof(double), send_event, E_SEND_DONE);
		send_msg(neighbor_list.back, memory.x_dim * memory.z_dim * sizeof(double), send_event, E_SEND_DONE);
		send_msg(neighbor_list.front, memory.x_dim * memory.z_dim * sizeof(double), send_event, E_SEND_DONE);
		bytes_sent += 2 * memory.x_dim * memory.y_dim * sizeof(double) +
		              2 * memory.y_dim * memory.z_dim * sizeof(double) +
			      2 * memory.x_dim * memory.z_dim * sizeof(double);
		num_sends += 6;

	    }
	    send_done= 0;
	    break;

	case E_SEND_DONE:
	    send_done++;
	    // Do we have all neighbors?
	    if ((rcv_cnt == neighbor_cnt * (t + 1)) && (send_done == neighbor_cnt))   {
		comm_time_total += getCurrentSimTime() - comm_time_start;
		goto_state(state_COMPUTE_CELLS, STATE_COMPUTE_CELLS, E_COMPUTE);
	    }
	    break;

	case E_CELL_RECEIVE:
	    rcv_cnt++;
	    // Do we have all neighbors?
	    if ((rcv_cnt == neighbor_cnt * (t + 1)) && (send_done == neighbor_cnt))   {
		comm_time_total += getCurrentSimTime() - comm_time_start;
		goto_state(state_COMPUTE_CELLS, STATE_COMPUTE_CELLS, E_COMPUTE);
	    }
	    break;

	default:
	    out.fatal(CALL_INFO, -1, "[%3d] Invalid event %d in state %d\n", my_rank, e, state);
	    break;
    }

}  // end of state_CELL_EXCHANGE()



void
Ghost_pattern::state_COMPUTE_CELLS(state_event sm_event)
{

ghost_events_t e= (ghost_events_t)sm_event.event;
int cnt;
double r;
SimTime_t duration;


    switch (e)   {
	case E_COMPUTE:
	    comp_time_start= getCurrentSimTime();
	    if (TwoD)   {

		cnt= (memory.x_dim - 2) * (2 * 5);
		cnt += (memory.y_dim - 2) * (2 * 5);
		cnt += loops * (memory.x_dim - 2) * (2 * 4);
		cnt += loops * (memory.y_dim - 2) * (2 * 4);

		fop_cnt += cnt;
		duration= cnt * time_per_flop;

		if (delay > 0.0)   {
		    // Extend compute time a little bit, by adding a wait of
		    // delay percent of compute time.
		    if (!compute_imbalance)   {
			duration += (SimTime_t)((double)duration * delay / 100.0);
		    } else   {
			// Randomly delay ranks by an average of delay %
			r= -1.0 * log(drand48()) * delay;
			if (r < 0.0)   {
			    r= 0.0;
			}
			duration += (SimTime_t)((double)duration * r / 100.0);
		    }
		}

		local_compute(E_COMPUTE_DONE, duration);

		// FIXME: Do I need to account for ghost cell update?
		// Code at end of compute() in src_ghost_bench work.c

	    } else   {

		cnt= (memory.x_dim - 2) * (memory.y_dim - 2) * (2 * 7);
		cnt += (memory.y_dim - 2) * (memory.z_dim - 2) * (2 * 7);
		cnt += (memory.x_dim - 2) * (memory.z_dim - 2) * (2 * 7);

		cnt += loops * (memory.x_dim - 2) * (memory.y_dim - 2) * (2 * 6);
		cnt += loops * (memory.y_dim - 2) * (memory.z_dim - 2) * (2 * 6);
		cnt += loops * (memory.x_dim - 2) * (memory.z_dim - 2) * (2 * 6);

		fop_cnt += cnt;
		duration= cnt * time_per_flop;

		if (delay > 0.0)   {
		    // Extend compute time a little bit, by adding a wait of
		    // delay percent of compute time.
		    if (!compute_imbalance)   {
			duration += (SimTime_t)((double)duration * delay / 100.0);
		    } else   {
			// Randomly delay ranks by an average of delay %
			r= -1.0 * log(drand48()) * delay;
			if (r < 0.0)   {
			    r= 0.0;
			}
			duration += (SimTime_t)((double)duration * r / 100.0);
		    }
		}

		local_compute(E_COMPUTE_DONE, duration);

		// FIXME: Do I need to account for ghost cell update?
		// Code at end of compute() in src_ghost_bench work.c
	    }
	    break;

	case E_COMPUTE_DONE:
	    comp_time_total += getCurrentSimTime() - comp_time_start;

	    if ((my_rank == 0) && (verbose > 1) && (t > 0) && ((t % (time_steps / 10)) == 0))   {
		printf("Time step %6d/%d complete\n", t, time_steps);
	    }

	    /* Once in a while do an allreduce to check on convergence */
	    if ((t + 1) % reduce_steps == 0)   {
		comm_time_start= getCurrentSimTime();
		goto_state(state_REDUCE, STATE_REDUCE, E_ALLREDUCE_ENTRY);
	    } else   {
		goto_state(state_INIT, STATE_INIT, E_NEXT_LOOP);
	    }
	    break;

	case E_SEND_DONE:
	    send_done++;
	    fprintf(stderr, "[%3d] SEND DONE in state_COMPUTE_CELLS() should not happen %d\n", my_rank, send_done);
	    break;

	case E_CELL_RECEIVE:
	    // Only count receives here. We wait until we are in state_CELL_EXCHANGE
	    // to make decisions.
	    rcv_cnt++;
	    break;

	default:
	    out.fatal(CALL_INFO, -1, "[%3d] Invalid event %d in state %d\n", my_rank, e, state);
	    break;
    }

}  // end of state_COMPUTE_CELLS()



void
Ghost_pattern::state_REDUCE(state_event sm_event)
{

ghost_events_t e= (ghost_events_t)sm_event.event;
state_event enter_allreduce, exit_allreduce;
double dummy= 1.0;


    switch (e)   {
	case E_ALLREDUCE_ENTRY:
	    // Set the parameters to be passed to the allreduce SM
	    enter_allreduce.event= SM_START_EVENT;
	    enter_allreduce.set_Fdata(dummy);
	    enter_allreduce.set_Idata(Allreduce_op::OP_SUM);

	    // We want to be called with this event, when allreduce returns
	    exit_allreduce.event= E_ALLREDUCE_EXIT;

	    SM->SM_call(SMallreduce, enter_allreduce, exit_allreduce);
	    break;

	case E_ALLREDUCE_EXIT:
	    // We just came back from the allreduce SM.
	    comm_time_total += getCurrentSimTime() - comm_time_start;
	    reduce_cnt++;
	    goto_state(state_INIT, STATE_INIT, E_NEXT_LOOP);
	    break;

	default:
	    out.fatal(CALL_INFO, -1, "[%3d] Invalid event %d in state %d\n", my_rank, e, state);
	    break;
    }

}  // end of state_REDUCE()



void
Ghost_pattern::state_DONE(state_event sm_event)
{

ghost_events_t e= (ghost_events_t)sm_event.event;


    switch (e)   {
	case E_DONE:
	    total_time_end= getCurrentSimTime();

	    if (rcv_cnt % neighbor_cnt != 0)   {
		fprintf(stderr, "[%3d] ERROR: Did not receive all msgs: %d\n", my_rank, rcv_cnt);
	    }

	    /* Print some statistics */
	    if (my_rank == 0)   {
		printf("Time to complete on %d ranks was %.6f seconds\n", num_ranks,
		    SimTimeToD(total_time_end - total_time_start));
		printf("Total %d time steps\n", time_steps);
		printf("Estimated timing error: %.2f%%\n",
		    100.0 - (100.0 / (total_time_end - total_time_start) * (comm_time_total + comp_time_total)));
		printf("Compute to communicate ratio was %.3g/%.3g = %.1f:1 (%.2f%% computation, %.2f%% "
		    "communication)\n",
		    SimTimeToD(comp_time_total), SimTimeToD(comm_time_total),
		    SimTimeToD(comp_time_total) / SimTimeToD(comm_time_total),
		    100.0 / (SimTimeToD(total_time_end) - SimTimeToD(total_time_start)) * SimTimeToD(comp_time_total),
		    100.0 / (SimTimeToD(total_time_end) - SimTimeToD(total_time_start)) * SimTimeToD(comm_time_total));
		printf("Bytes sent from each rank: %" PRId64 " bytes (%.3f MB), %.1f MB total\n",
		    bytes_sent, (float)bytes_sent / 1024 / 1024,
		    (float)bytes_sent / 1024 / 1024 * num_ranks);
		printf("Sends per rank: %" PRId64 ", %" PRId64 " total\n", num_sends, num_sends * num_ranks);
		printf("Average size per send: %.0f B (%.3f MB)\n", (float)bytes_sent / num_sends,
		    (float)bytes_sent / num_sends / 1024 / 1024);
		printf("Number of allreduces: %" PRId64 " (one every %d time steps)\n",
		    reduce_cnt, reduce_steps);
		printf("Total number of flotaing point ops %" PRId64 " (%.3f x 10^9)\n",
		    fop_cnt * num_ranks, (float)fop_cnt / 1000000000.0 * num_ranks);
		printf("Per second: %.0f Flops (%.3f GFlops)\n",
		    (float)fop_cnt * num_ranks / SimTimeToD(total_time_end - total_time_start),
		    (float)fop_cnt * num_ranks / SimTimeToD(total_time_end - total_time_start) / 1000000000.0);
		printf("Flops per byte sent: %.2f Flops\n", (float)fop_cnt / bytes_sent);
		if (compute_imbalance)   {
		    printf("Each compute step was imbalanced by an average %.1f%%\n", SimTimeToD(compute_delay));
		} else   {
		    printf("Each compute step was delayed by %.1f%%\n", SimTimeToD(compute_delay));
		}
	    }
	    done= true;
	    break;

	default:
	    out.fatal(CALL_INFO, -1, "[%3d] Invalid event %d in state %d\n", my_rank, e, state);
	    break;
    }

}  // end of state_DONE()



// -----------------------------------------------------------------------------
// Helper functions
// -----------------------------------------------------------------------------
void
Ghost_pattern::do_decomposition(void)
{
    /*
    ** Assign ranks to portions of the data.
    ** Rank_width, rank_height, and rank_depth are in number of ranks.
    ** x_elements, y_elements, z_elements are the number of elements (double)
    ** per rank in each dimension.
    */
    TwoD= !z_elements;
    map_ranks(num_ranks, TwoD, &rank_width, &rank_height, &rank_depth);
    check_element_assignment(verbose, decomposition_only, num_ranks, rank_width,
	rank_height, rank_depth, my_rank, &TwoD, &x_elements, &y_elements, &z_elements);

    /* Figure out who my neighbors are */
    neighbors(verbose, my_rank, rank_width, rank_height, rank_depth, &neighbor_list);

    /* Estimate how much memory we will need */
    mem_estimate= mem_needed(verbose, decomposition_only, num_ranks, my_rank, TwoD,
	x_elements, y_elements, z_elements);

}  // end of do_decomposition()


// -----------------------------------------------------------------------------



/*
// THIS SECTION MOVED TO patterns.cc FOR CONFIG CHANGE TO ONE LIBRARY FILE - ALEVINE

eli(Ghost_pattern, ghost_pattern, "Ghost pattern")
*/

// ADDED FOR PROPER INITIALIZATION - ALEVINE
// SST Startup and Shutdown
void Ghost_pattern::setup()
{
	// Call the initial State Transition
	state_transition(E_START, STATE_INIT);
}

#ifdef SERIALIZATION_WORKS_NOW
BOOST_CLASS_EXPORT(Ghost_pattern)
#endif // SERIALIZATION_WORKS_NOW
