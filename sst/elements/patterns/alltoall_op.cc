//
// Copyright (c) 2011, IBM Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


/*

Alltoall

During object creation, the calling module can set the default
message length.  It is used as the simulated message length for
alltoall messages.

No actual data is transferred, only events of the appropriate length.

This version only works on power of 2 ranks!

*/
#include "sst/core/serialization/element.h"
#include "alltoall_op.h"



void
Alltoall_op::handle_events(state_event sm_event)
{

    switch (state)   {
	case START:
	    state_INIT(sm_event);
	    break;

	case MAIN_LOOP:
	    state_MAIN_LOOP(sm_event);
	    break;

	case WAIT_DATA:
	    state_WAIT_DATA(sm_event);
	    break;

    }

    // Don't call unregisterExit()
    // Only "main" patterns should do that; i.e., patterns that use other
    // patterns like this one. Just return to our caller.
    if (done)   {
	state= START;
	done= false;
	cp->SM->SM_return(sm_event);
    }

}  /* end of handle_events() */



void
Alltoall_op::state_INIT(state_event sm_event)
{

alltoall_events_t e= (alltoall_events_t)sm_event.event;
state_event send_event;


    switch (e)   {
	case E_START:
	    /*
	    ** If we did this for real, this would be the place where
	    ** we copy our contrinution to from the in to the result array.
	    */

	    /* Set start parameters */
	    i= alltoall_nranks >> 1;
	    shift= 1;

	    /* Go to the main loop */
	    goto_state(state_MAIN_LOOP, MAIN_LOOP, E_NEXT_LOOP);
	    break;

	default:
	    _abort(alltoall_pattern, "[%3d] Invalid event %d in state %d\n",
		cp->my_rank, e, state);
    }

}  // end of state_INIT()



void
Alltoall_op::state_MAIN_LOOP(state_event sm_event)
{

alltoall_events_t e= (alltoall_events_t)sm_event.event;
state_event send_event;


    switch (e)   {
	case E_NEXT_LOOP:
	    if (i > 0)   {
		/* We got (more) work to do */
		dest= (cp->my_rank + shift) % alltoall_nranks;
		src= ((cp->my_rank - shift) + alltoall_nranks) % alltoall_nranks;
		offset= (cp->my_rank * alltoall_msglen) - ((shift - 1) * alltoall_msglen);

		if (offset < 0)   {
		    /* Need to break it up into two pieces */
		    offset= offset + (alltoall_nranks * alltoall_msglen);
		    len1= (alltoall_nranks * alltoall_msglen) - offset;
		    send_event.event= E_INITIAL_DATA;
		    cp->send_msg(dest, len1 * sizeof(double), send_event);
		    len2= shift * alltoall_msglen - (alltoall_nranks * alltoall_msglen - offset);
		    send_event.event= E_LAST_DATA;
		    // Tricky: We only wait for the second send to finish. Receive has to be in order!
		    cp->send_msg(dest, len2 * sizeof(double), send_event, E_SEND_DONE);

		} else   {
		    /* I can send it in one piece */
		    len1= shift * alltoall_msglen;
		    send_event.event= E_ALL_DATA;
		    cp->send_msg(dest, len1 * sizeof(double), send_event, E_SEND_DONE);
		}
		state= WAIT_DATA;

	    } else   {
		/* We are done looping. Exit */
		done= true;
	    }
	    break;

	default:
	    _abort(alltoall_pattern, "[%3d] Invalid event %d in state %d\n",
		cp->my_rank, e, state);
    }

}  // end of state_MAIN_LOOP()



// Data may arrive in one chunk, or in two pieces
void
Alltoall_op::state_WAIT_DATA(state_event sm_event)
{

alltoall_events_t e= (alltoall_events_t)sm_event.event;
state_event send_event;


    switch (e)   {
	case E_LAST_DATA:
	case E_ALL_DATA:
	    shift= shift << 1;
	    i= i >> 1;
	    goto_state(state_MAIN_LOOP, MAIN_LOOP, E_NEXT_LOOP);
	    break;

	case E_INITIAL_DATA:
	    // FIXME: Not entirely correct. Should have a seperate state
	    break;

	default:
	    _abort(alltoall_pattern, "[%3d] Invalid event %d in state %d\n",
		cp->my_rank, e, state);
    }

}  // end of state_WAIT_DATA()
