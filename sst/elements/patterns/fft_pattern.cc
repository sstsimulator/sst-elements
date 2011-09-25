// Copyright 2009-2011 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
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
#include "fft_pattern.h"
#include "stats.h"



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
	unregisterExit();
	done= false;
    }

}  /* end of handle_events() */



void
FFT_pattern::state_INIT(state_event sm_event)
{

fft_events_t e= (fft_events_t)sm_event.event;


    switch (e)   {
	case E_START:
	    if (my_rank == 0)   {
		// Phase 1: Do bit reversal on rank 0
		duration= N * 1;
		local_compute(E_COMPUTE_DONE, duration);
	    } else   {
		// Go to phase 2 to wait for work
		goto_state(state_PHASE2, STATE_PHASE2, E_SCATTER_ENTRY);
	    }
	    break;

	case E_COMPUTE_DONE:
	    // Rank 0 is done with the bit reversal. Do the scatter
	    goto_state(state_PHASE2, STATE_PHASE2, E_SCATTER_ENTRY);

	default:
	    _abort(fft_pattern, "[%3d] Invalid event %d in state %d\n", my_rank, e, state);
	    break;
    }

}  // end of state_INIT()



void
FFT_pattern::state_PHASE2(state_event sm_event)
{

fft_events_t e= (fft_events_t)sm_event.event;
state_event enter_scatter, exit_scatter;


    switch (e)   {
	case E_SCATTER_ENTRY:
	    // Start of barrier
	    enter_scatter.event= SM_START_EVENT;
	    exit_scatter.event= E_SCATTER_EXIT;
	    SM->SM_call(SMscatter, enter_scatter, exit_scatter);
	    break;

	case E_SCATTER_EXIT:
	    // We just came back from the scatter SM.
	    goto_state(state_PHASE3, STATE_PHASE3, E_COMPUTE);
	    break;

	default:
	    _abort(fft_pattern, "[%3d] Invalid event %d in state %d\n", my_rank, e, state);
	    break;
    }

}  // end of state_PHASE2()



void
FFT_pattern::state_DONE(state_event sm_event)
{

fft_events_t e= (fft_events_t)sm_event.event;


    switch (e)   {
	case E_DONE:
	    done= true;
	    break;

	default:
	    _abort(fft_pattern, "[%3d] Invalid event %d in state %d\n", my_rank, e, state);
	    break;
    }

}  // end of state_DONE()



extern "C" {
FFT_pattern *
fft_patternAllocComponent(SST::ComponentId_t id,
                          SST::Component::Params_t& params)
{
    return new FFT_pattern(id, params);
}
}

BOOST_CLASS_EXPORT(FFT_pattern)
