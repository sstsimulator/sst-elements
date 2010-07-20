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
#include "common.h"

int ghost_pattern_debug;

typedef enum {COMPUTE, WAIT, DONE} state_t;
typedef enum {COMPUTE_DONE, RECEIVE, FAIL, RESEND_MSG} pattern_event_t;

void
Ghost_pattern::handle_events(Event *sst_event)
{

static state_t state= COMPUTE;
pattern_event_t event;
static int left, right, up, down;
int myX, myY;
int len;
static bool first_time= true;
static int rcv_cnt= 0;


    _GHOST_PATTERN_DBG(3, "Rank %d got an event\n", my_rank);

    /* For now, our message size is always 128 bytes */
    len= 128;

    /* Who are my four neighbors? */
    if (first_time)   {
	myX= my_rank % x_dim;
	myY= my_rank / y_dim;
	right= ((myX + 1) % x_dim) + (myY * y_dim);
	left= ((myX - 1 + x_dim) % x_dim) + (myY * y_dim);
	down= myX + ((myY + 1) % y_dim) * y_dim;
	up= myX + ((myY - 1 + y_dim) % y_dim) * y_dim;
	first_time= false;
    }

    /* Extract the pattern event type from the SST event */
    event= COMPUTE_DONE;
    // event= sst_event->pattern_event;

    switch (state)   {
	case COMPUTE:
	    switch (event)   {
		case COMPUTE_DONE:
		    /* Our time to compute is over */
		    pattern_send(right, len);
		    pattern_send(left, len);
		    pattern_send(up, len);
		    pattern_send(down, len);
		    state= WAIT;
		    break;
		case RECEIVE:
		    /* We got a message from another rank */
		    rcv_cnt++;
		    if (rcv_cnt == 4)   {
			rcv_cnt= 0;
			state= COMPUTE;
		    }
		    break;
		case FAIL:
		    /* We just failed. Deal with it! */
		    break;
		case RESEND_MSG:
		    /* We have been asked to resend a previous msg to help another rank to recover */
		    break;
	    }
	    break;

	case WAIT:
	    /*
	    ** We are waiting for messages from our four neighbors */
	    switch (event)   {
		case COMPUTE_DONE:
		    /* Doesn't make sense; we should not be getting this event */
		    _abort(ghost_pattern, "Compute done event in wait\n");
		    break;
		case RECEIVE:
		    /* YES! We got a message from another rank */
		    rcv_cnt++;
		    if (rcv_cnt == 4)   {
			rcv_cnt= 0;
			state= COMPUTE;
		    }
		    break;
		case FAIL:
		    /* We just failed. Deal with it! */
		    break;
		case RESEND_MSG:
		    /* We have been asked to resend a previous msg to help another rank to recover */
		    break;
	    }
	    break;

	case DONE:
	    /* This rank has done all of its work */
	    return;
	    break;
    }

    return;

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
