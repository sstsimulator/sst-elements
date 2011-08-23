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
#include "allreduce_pattern.h"



void
Allreduce_pattern::handle_events(state_event sm_event)
{

    switch (state)   {
	case STATE_INIT:		state_INIT(sm_event); break;
	case STATE_TEST:		state_TEST(sm_event); break;
	case STATE_INNER_LOOP:		state_INNER_LOOP(sm_event); break;
	case STATE_ALLREDUCE_TEST:	state_ALLREDUCE_TEST(sm_event); break;
	case STATE_COLLECT_RESULT:	state_COLLECT_RESULT(sm_event); break;
	case STATE_DONE:		state_DONE(sm_event); break;
    }

    if (done)   {
	unregisterExit();
	done= false;
    }

}  /* end of handle_events() */



void
Allreduce_pattern::state_INIT(state_event sm_event)
{

allreduce_events_t e= (allreduce_events_t)sm_event.event;


    switch (e)   {
	case E_START:
	    rcv_cnt= 0;
	    if (my_rank == 0)   {
		printf("#  |||  Test 3: my_allreduce() nodes, min, mean, median, max, sd\n");
	    }

	    goto_state(state_INNER_LOOP, STATE_INNER_LOOP, E_NEXT_INNER_LOOP);
	    break;

	case E_NEXT_OUTER_LOOP:
	    if (nnodes == num_ranks)   {
		// We're done
	    }
	    break;

	default:
	    _abort(allreduce_pattern, "[%3d] Invalid event %d in state %d\n", my_rank, e, state);
	    break;
    }

}  // end of state_INIT()



void
Allreduce_pattern::state_INNER_LOOP(state_event sm_event)
{

allreduce_events_t e= (allreduce_events_t)sm_event.event;


    switch (e)   {
	case E_NEXT_INNER_LOOP:
	    break;

	default:
	    _abort(allreduce_pattern, "[%3d] Invalid event %d in state %d\n", my_rank, e, state);
	    break;
    }

}  // end of state_INNER_LOOP()



void
Allreduce_pattern::state_TEST(state_event sm_event)
{

allreduce_events_t e= (allreduce_events_t)sm_event.event;


    switch (e)   {
	case E_NEXT_INNER_LOOP:
	    break;

	default:
	    _abort(allreduce_pattern, "[%3d] Invalid event %d in state %d\n", my_rank, e, state);
	    break;
    }

}  // end of state_TEST()



void
Allreduce_pattern::state_ALLREDUCE_TEST(state_event sm_event)
{

allreduce_events_t e= (allreduce_events_t)sm_event.event;


    switch (e)   {
	case E_NEXT_INNER_LOOP:
	    break;

	default:
	    _abort(allreduce_pattern, "[%3d] Invalid event %d in state %d\n", my_rank, e, state);
	    break;
    }

}  // end of state_ALLREDUCE_TEST()



void
Allreduce_pattern::state_COLLECT_RESULT(state_event sm_event)
{

allreduce_events_t e= (allreduce_events_t)sm_event.event;


    switch (e)   {
	case E_NEXT_INNER_LOOP:
	    break;

	default:
	    _abort(allreduce_pattern, "[%3d] Invalid event %d in state %d\n", my_rank, e, state);
	    break;
    }

}  // end of state_COLLECT_RESULT()



void
Allreduce_pattern::state_DONE(state_event sm_event)
{

allreduce_events_t e= (allreduce_events_t)sm_event.event;


    switch (e)   {
	case E_NEXT_INNER_LOOP:
	    break;

	default:
	    _abort(allreduce_pattern, "[%3d] Invalid event %d in state %d\n", my_rank, e, state);
	    break;
    }

}  // end of state_DONE()



extern "C" {
Allreduce_pattern *
allreduce_patternAllocComponent(SST::ComponentId_t id,
                          SST::Component::Params_t& params)
{
    return new Allreduce_pattern(id, params);
}
}

BOOST_CLASS_EXPORT(Allreduce_pattern)
