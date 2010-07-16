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
#include "ghost.h"

int ghost_pattern_debug;

typedef enum {COMPUTE, WAIT, DONE} state_t;
typedef enum {COMPUTE_DONE, RECEIVE, FAIL, RESEND_MSG} pattern_event_t;

bool
Ghost_pattern::handle_events(Event *sst_event)
{

static state_t state= COMPUTE;
pattern_event_t event;


    _GHOST_PATTERN_DBG(3, "Rank %d got an event\n", my_rank);

    /* Extract the pattern event type from the SST event */
    event= COMPUTE_DONE;
    // event= sst_event->pattern_event;

    switch (state)   {
	case COMPUTE:
	    switch (event)   {
		case COMPUTE_DONE:
		    /* Our time to compute is over */
		    break;
		case RECEIVE:
		    /* We got a message from another rank */
		    break;
		case FAIL:
		    /* We just failed. Deal with it! */
		    break;
		case RESEND_MSG:
		    /* We have been ask to resend a previous msg to help another rank to recover */
		    break;
	    }
	    break;

	case WAIT:
	    break;

	case DONE:
	    /* This rank has done all of its work */
	    return true;
	    break;
    }

    return false;

}  /* end of handle_port_events() */



extern "C" {
Ghost_pattern *
ghost_patternAllocComponent(SST::ComponentId_t id,
                          SST::Component::Params_t& params)
{
    return new Ghost_pattern(id, params);
}
}

BOOST_CLASS_EXPORT(Ghost_pattern)
