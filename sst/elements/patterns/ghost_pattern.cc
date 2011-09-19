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
#include "ghost_pattern.h"



void
Ghost_pattern::handle_events(state_event sm_event)
{

    switch (state)   {
	case STATE_INIT:		state_INIT(sm_event); break;
	case STATE_REDUCE:		state_REDUCE(sm_event); break;
    }

    if (done)   {
	unregisterExit();
	done= false;
    }

}  /* end of handle_events() */



void
Ghost_pattern::state_INIT(state_event sm_event)
{

ghost_events_t e= (ghost_events_t)sm_event.event;


    switch (e)   {
	case E_START:
	    break;

	default:
	    _abort(ghost_pattern, "[%3d] Invalid event %d in state %d\n", my_rank, e, state);
	    break;
    }

}  // end of state_INIT()



void
Ghost_pattern::state_REDUCE(state_event sm_event)
{

ghost_events_t e= (ghost_events_t)sm_event.event;


    switch (e)   {
	case E_START:
	    break;

	default:
	    _abort(ghost_pattern, "[%3d] Invalid event %d in state %d\n", my_rank, e, state);
	    break;
    }

}  // end of state_REDUCE()



extern "C" {
Ghost_pattern *
ghost_patternAllocComponent(SST::ComponentId_t id,
                          SST::Component::Params_t& params)
{
    return new Ghost_pattern(id, params);
}
}

BOOST_CLASS_EXPORT(Ghost_pattern)
