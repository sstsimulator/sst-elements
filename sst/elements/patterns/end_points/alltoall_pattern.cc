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
#include "alltoall_pattern.h"
#include <sst/core/element.h>
#include "util/stats.h"



void
Alltoall_pattern::handle_events(state_event sm_event)
{

    switch (state)   {
	case STATE_INIT:		state_INIT(sm_event); break;
	case STATE_TEST:		state_TEST(sm_event); break;
	case STATE_INNER_LOOP:		state_INNER_LOOP(sm_event); break;
	case STATE_ALLTOALL_TEST:	state_ALLTOALL_TEST(sm_event); break;
	case STATE_COLLECT_RESULT:	state_COLLECT_RESULT(sm_event); break;
	case STATE_DONE:		state_DONE(sm_event); break;
    }

    if (done)   {
  primaryComponentOKToEndSim();
	done= false;
    }

}  /* end of handle_events() */



void
Alltoall_pattern::state_INIT(state_event sm_event)
{

alltoall_events_t e= (alltoall_events_t)sm_event.event;


    switch (e)   {
	case E_START:
	case E_NEXT_OUTER_LOOP:
	    if (nnodes == num_ranks)   {
		// We are done
		goto_state(state_DONE, STATE_DONE, E_DONE);
	    } else   {
		if (nnodes < 64)   {
		    nnodes++;
		} else if (nnodes < 128)   {
		    nnodes= nnodes + 8;
		    if (nnodes > num_ranks) nnodes= num_ranks;
		} else if (nnodes < 512)   {
		    nnodes= nnodes + 32;
		    if (nnodes > num_ranks) nnodes= num_ranks;
		} else if (nnodes < 2048)   {
		    nnodes= nnodes + 128;
		    if (nnodes > num_ranks) nnodes= num_ranks;
		} else   {
		    nnodes= nnodes + 512;
		    if (nnodes > num_ranks) nnodes= num_ranks;
		}

		if (nnodes < SMALL_LARGE_CUTOFF)   {
		    num_ops= SMALL_ALLREDUCE_OPS;
		} else   {
		    num_ops= LARGE_ALLREDUCE_OPS;
		}

		// Start next loop with nnodes
		a_test->resize(nnodes);
		times.clear();
		set= 0;
		goto_state(state_INNER_LOOP, STATE_INNER_LOOP, E_NEXT_INNER_LOOP);
	    }
	    break;

	default:
	    _abort(alltoall_pattern, "[%3d] Invalid event %d in state %d\n", my_rank, e, state);
	    break;
    }

}  // end of state_INIT()



void
Alltoall_pattern::state_INNER_LOOP(state_event sm_event)
{

alltoall_events_t e= (alltoall_events_t)sm_event.event;
state_event enter_barrier, exit_barrier;


    switch (e)   {
	case E_NEXT_INNER_LOOP:
	    // Start of barrier
	    enter_barrier.event= SM_START_EVENT;
	    exit_barrier.event= E_BARRIER_EXIT;
	    SM->SM_call(SMbarrier, enter_barrier, exit_barrier);
	    break;

	case E_BARRIER_EXIT:
	    // We just came back from the barrier SM.
	    if (my_rank < nnodes)   {
		// My rank will be participating in the test. Go run it
		ops= 0;
		test_start_time= getCurrentSimTime();
		goto_state(state_TEST, STATE_TEST, E_NEXT_TEST);
	    } else   {
		// My rank is not part of this test, skip ahead
		duration= 0;
		goto_state(state_COLLECT_RESULT, STATE_COLLECT_RESULT, E_COLLECT);
	    }
	    break;

	default:
	    _abort(alltoall_pattern, "[%3d] Invalid event %d in state %d\n", my_rank, e, state);
	    break;
    }

}  // end of state_INNER_LOOP()



void
Alltoall_pattern::state_TEST(state_event sm_event)
{

alltoall_events_t e= (alltoall_events_t)sm_event.event;


    switch (e)   {
	case E_NEXT_TEST:
	    ops++;
	    if (ops <= num_ops)   {
		// Do the alltoall test on nnodes
		goto_state(state_ALLTOALL_TEST, STATE_ALLTOALL_TEST, E_ALLTOALL_ENTRY);
	    } else   {
		// Done with the test
		duration= (getCurrentSimTime() - test_start_time) / num_ops;
		goto_state(state_COLLECT_RESULT, STATE_COLLECT_RESULT, E_COLLECT);
	    }
	    break;

	default:
	    _abort(alltoall_pattern, "[%3d] Invalid event %d in state %d\n", my_rank, e, state);
	    break;
    }

}  // end of state_TEST()



void
Alltoall_pattern::state_ALLTOALL_TEST(state_event sm_event)
{

alltoall_events_t e= (alltoall_events_t)sm_event.event;
state_event enter_alltoall, exit_alltoall;


    switch (e)   {
	case E_ALLTOALL_ENTRY:
	    // Set the parameters to be passed to the alltoall SM
	    enter_alltoall.event= SM_START_EVENT;

	    // We want to be called with this event, when alltoall returns
	    exit_alltoall.event= E_ALLTOALL_EXIT;

	    SM->SM_call(SMalltoall_test, enter_alltoall, exit_alltoall);
	    break;

	case E_ALLTOALL_EXIT:
	    // We just came back from the alltoall SM. We're done
	    goto_state(state_TEST, STATE_TEST, E_NEXT_TEST);
	    break;

	default:
	    _abort(alltoall_pattern, "[%3d] Invalid event %d in state %d\n", my_rank, e, state);
	    break;
    }

}  // end of state_ALLTOALL_TEST()



// Now do an allreduce over all nodes collecting the measured times
void
Alltoall_pattern::state_COLLECT_RESULT(state_event sm_event)
{

alltoall_events_t e= (alltoall_events_t)sm_event.event;
state_event enter_allreduce, exit_allreduce;


    switch (e)   {
	case E_COLLECT:
	    // Set the parameters to be passed to the allreduce SM
	    enter_allreduce.event= SM_START_EVENT;
	    enter_allreduce.set_Fdata(SimTimeToD(duration));
	    enter_allreduce.set_Idata(Allreduce_op::OP_SUM);

	    // We want to be called with this event, when allreduce returns
	    exit_allreduce.event= E_ALLREDUCE_EXIT;

	    SM->SM_call(SMallreduce_collect, enter_allreduce, exit_allreduce);
	    break;

	case E_ALLREDUCE_EXIT:
	    // We just came back from the allreduce SM. We're done
	    times.push_back(sm_event.get_Fdata() / nnodes);
	    set++;
	    if (set < num_sets)   {
		goto_state(state_INNER_LOOP, STATE_INNER_LOOP, E_NEXT_INNER_LOOP);
	    } else   {
		if (my_rank == 0)   {
		    printf("%6d ", nnodes);
		    print_stats(times);
		    printf("\n");
		}
		goto_state(state_INIT, STATE_INIT, E_NEXT_OUTER_LOOP);
	    }

	    break;

	default:
	    _abort(alltoall_pattern, "[%3d] Invalid event %d in state %d\n", my_rank, e, state);
	    break;
    }

}  // end of state_COLLECT_RESULT()



void
Alltoall_pattern::state_DONE(state_event sm_event)
{

alltoall_events_t e= (alltoall_events_t)sm_event.event;


    switch (e)   {
	case E_DONE:
	    done= true;
	    break;

	default:
	    _abort(alltoall_pattern, "[%3d] Invalid event %d in state %d\n", my_rank, e, state);
	    break;
    }

}  // end of state_DONE()



/*
// THIS SECTION MOVED TO patterns.cc FOR CONFIG CHANGE TO ONE LIBRARY FILE - ALEVINE

eli(Alltoall_pattern, alltoall_pattern, "Alltoall pattern")
*/

// ADDED FOR PROPER INITIALIZATION - ALEVINE
// SST Startup and Shutdown
void Alltoall_pattern::setup()
{
	// Call the initial State Transition
	state_transition(E_START, STATE_INIT);
}

#ifdef SERIALIZATION_WORKS_NOW
BOOST_CLASS_EXPORT(Alltoall_pattern)
#endif // SERIALIZATION_WORKS_NOW
