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
#include "fft_pattern.h"

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS			(1)
#endif
#include <inttypes.h>				/* For PRId64 */
#include <math.h>				/* For llrint() */

#include <sst/core/element.h>

#include "util/stats.h"



void
FFT_pattern::handle_events(state_event sm_event)
{

    switch (state)   {
	case STATE_INIT:		state_INIT(sm_event); break;
	case STATE_PHASE1:		state_PHASE1(sm_event); break;
	case STATE_PHASE2:		state_PHASE2(sm_event); break;
	case STATE_PHASE3:		state_PHASE3(sm_event); break;
	case STATE_PHASE4:		state_PHASE4(sm_event); break;
	case STATE_PHASE5:		state_PHASE5(sm_event); break;
	case STATE_DONE:		state_DONE(sm_event); break;
    }

    if (done)   {
  primaryComponentOKToEndSim();
	done= false;
    }

}  /* end of handle_events() */



void
FFT_pattern::state_INIT(state_event sm_event)
{

fft_events_t e= (fft_events_t)sm_event.event;
SimTime_t duration;


    switch (e)   {
	case E_START:
	    test_start_time= getCurrentSimTime();
	    if (my_rank == 0)   {
		// Phase 1: Do bit reversal on rank 0
		duration= N * time_per_flop;
		local_compute(E_COMPUTE_DONE, duration);
	    } else   {
		// Go to phase 2 to wait for work
		goto_state(state_PHASE2, STATE_PHASE2, E_SCATTER_ENTRY);
	    }
	    break;

	case E_COMPUTE_DONE:
	    // Rank 0 is done with the bit reversal. Do the scatter
	    if (verbose > 0)   {
		printf("#  |||  Rank 0 done with phase 1\n");
	    }
	    goto_state(state_PHASE2, STATE_PHASE2, E_SCATTER_ENTRY);
	    break;

	default:
	    _abort(fft_pattern, "[%3d] Invalid event %d in state %d\n", my_rank, e, state);
	    break;
    }

}  // end of state_INIT()



void
FFT_pattern::state_PHASE1(state_event sm_event)
{
  // DO NOTHING STATE; ADDED TO SOLVE STATIC LINKING ISSUE
}


void
FFT_pattern::state_PHASE2(state_event sm_event)
{

fft_events_t e= (fft_events_t)sm_event.event;
state_event enter_scatter, exit_scatter;


    switch (e)   {
	case E_SCATTER_ENTRY:
	    phase1_time= getCurrentSimTime() - test_start_time;
	    // Start of scatter
	    enter_scatter.event= SM_START_EVENT;
	    exit_scatter.event= E_SCATTER_EXIT;
	    SM->SM_call(SMscatter, enter_scatter, exit_scatter);
	    break;

	case E_SCATTER_EXIT:
	    // We just came back from the scatter SM.
	    if (my_rank == 0)   {
		if (verbose > 0)   {
		    printf("#  |||  Scatter phase 2 done\n");
		}
	    }
	    goto_state(state_PHASE3, STATE_PHASE3, E_PHASE3_ENTRY);
	    break;

	default:
	    _abort(fft_pattern, "[%3d] Invalid event %d in state %d\n", my_rank, e, state);
	    break;
    }

}  // end of state_PHASE2()



void
FFT_pattern::state_PHASE3(state_event sm_event)
{

fft_events_t e= (fft_events_t)sm_event.event;
SimTime_t duration;


    switch (e)   {
	case E_PHASE3_ENTRY:
	    phase2_time= getCurrentSimTime() - phase1_time;
	    duration= llrint(time_per_flop * ((N / num_ranks) - (log2(N) - log2(num_ranks))));
	    local_compute(E_PHASE3_EXIT, duration);
	    break;

	case E_PHASE3_EXIT:
	    if ((my_rank == 0) && (verbose > 0))   {
		printf("#  |||  Phase 3 done\n");
	    }
	    goto_state(state_PHASE4, STATE_PHASE4, E_PHASE4_ENTRY);
	    break;

	default:
	    _abort(fft_pattern, "[%3d] Invalid event %d in state %d\n", my_rank, e, state);
	    break;
    }

}  // end of state_PHASE3()



void
FFT_pattern::state_PHASE4(state_event sm_event)
{

fft_events_t e= (fft_events_t)sm_event.event;
SimTime_t duration;


    switch (e)   {
	case E_PHASE4_ENTRY:
	    phase3_time= getCurrentSimTime() - phase2_time;
	    M= lrint(log2(num_ranks));
	    // Do M message exchanges of size N/num_ranks?
	    // With whom?
	    duration= time_per_flop * N / num_ranks;
	    local_compute(E_PHASE4_EXIT, duration);
	    break;

	case E_PHASE4_EXIT:
	    if ((my_rank == 0) && (verbose > 0))   {
		printf("#  |||  Phase 4 done\n");
	    }
	    goto_state(state_PHASE5, STATE_PHASE5, E_GATHER_ENTRY);
	    break;

	default:
	    _abort(fft_pattern, "[%3d] Invalid event %d in state %d\n", my_rank, e, state);
	    break;
    }

}  // end of state_PHASE4()



void
FFT_pattern::state_PHASE5(state_event sm_event)
{

fft_events_t e= (fft_events_t)sm_event.event;
state_event enter_gather, exit_gather;


    switch (e)   {
	case E_GATHER_ENTRY:
	    phase4_time= getCurrentSimTime() - phase3_time;
	    // Start of gather
	    enter_gather.event= SM_START_EVENT;
	    exit_gather.event= E_GATHER_EXIT;
	    SM->SM_call(SMgather, enter_gather, exit_gather);
	    break;

	case E_GATHER_EXIT:
	    // We just came back from the gather SM.
	    if ((my_rank == 0) && (verbose > 0))   {
		printf("#  |||  Phase 5 gather done\n");
	    }
	    goto_state(state_DONE, STATE_DONE, E_DONE);
	    break;

	default:
	    _abort(fft_pattern, "[%3d] Invalid event %d in state %d\n", my_rank, e, state);
	    break;
    }

}  // end of state_PHASE5()



void
FFT_pattern::state_DONE(state_event sm_event)
{

fft_events_t e= (fft_events_t)sm_event.event;


    switch (e)   {
	case E_DONE:
	    phase5_time= getCurrentSimTime() - phase4_time;
	    total_time= getCurrentSimTime() - test_start_time;
	    if (my_rank == 0)   {
		printf("#  |||  FFT Times\n");
		printf("#  |||      Phase 1 %12" PRId64 " %s\n", (uint64_t)phase1_time, TIME_BASE);
		printf("#  |||      Phase 2 %12" PRId64 " %s\n", (uint64_t)phase2_time, TIME_BASE);
		printf("#  |||      Phase 3 %12" PRId64 " %s\n", (uint64_t)phase3_time, TIME_BASE);
		printf("#  |||      Phase 4 %12" PRId64 " %s\n", (uint64_t)phase4_time, TIME_BASE);
		printf("#  |||      Phase 5 %12" PRId64 " %s\n", (uint64_t)phase5_time, TIME_BASE);
		printf("#  |||    Total     %12" PRId64 " %s\n", (uint64_t)total_time, TIME_BASE);
	    }
	    done= true;
	    break;

	default:
	    _abort(fft_pattern, "[%3d] Invalid event %d in state %d\n", my_rank, e, state);
	    break;
    }

}  // end of state_DONE()



/*
// THIS SECTION MOVED TO patterns.cc FOR CONFIG CHANGE TO ONE LIBRARY FILE - ALEVINE

eli(FFT_pattern, fft_pattern, "FFT pattern")
*/

// ADDED FOR PROPER INITIALIZATION - ALEVINE
// SST Startup and Shutdown
void FFT_pattern::setup()
{
	// Call the initial State Transition
	state_transition(E_START, STATE_INIT);
}

#ifdef SERIALIZATION_WORKS_NOW
BOOST_CLASS_EXPORT(FFT_pattern)
#endif // SERIALIZATION_WORKS_NOW
