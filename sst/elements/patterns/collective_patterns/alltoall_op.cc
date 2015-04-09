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

*/
#include <sst_config.h>
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

	case SEND:
	    state_SEND(sm_event);
	    break;

	case REMAINDER:
	    state_REMAINDER(sm_event);
	    break;

	case WAIT:
	    state_WAIT(sm_event);
	    break;

    }

    // Don't call primaryComponentOKToEndSim()
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
	    // Set start parameters
	    i= alltoall_nranks >> 1;
	    shift= 1;
	    bytes_sent= 0;
	    remainder_done= false;
	    epoch= 0;

	    // We have to copy our contribution from the "in" to the "out" array
	    cp->memcpy(E_MEMCPY_DONE, alltoall_msglen);
	    break;

	case E_MEMCPY_DONE:
	    // Done with the memcpy, go to the main loop
	    goto_state(state_MAIN_LOOP, MAIN_LOOP, E_NEXT_LOOP);
	    break;

	case E_LAST_DATA:
	case E_ALL_DATA:
	    // Early data, while we are waiting for the memcpy to finish
	    receives[sm_event.get_epoch()]++;
	    if (receives[epoch] == expected_receives)   {
		receives[epoch]= 0;
		got_all_receives= true;
	    }

	    if (got_all_sends && got_all_receives)   {
		// The sends cannot be complete yet; they haven't been sent
		assert(false);
	    }
	    break;

	case E_INITIAL_DATA:
	    // We don't count these to keep things simple. There will be an
	    // E_LAST_DATA soon.
	    break;

	default:
	    _abort(alltoall_op, "[%3d] Invalid event %d in state %d\n",
		cp->my_rank, e, state);
    }

}  // end of state_INIT()



void
Alltoall_op::state_MAIN_LOOP(state_event sm_event)
{

alltoall_events_t e= (alltoall_events_t)sm_event.event;


    switch (e)   {
	case E_NEXT_LOOP:
	    epoch= (epoch + 1) % MAX_EPOCH;
	    if (i > 0)   {
		// We got (more) work to do
		goto_state(state_SEND, SEND, E_SEND_START);
	    } else   {
		// If we are not on a power of two ranks, we need one more step
		if (!remainder_done)   {
		    if (!cp->is_pow2(alltoall_nranks))   {
			goto_state(state_REMAINDER, REMAINDER, E_REMAINDER_START);
		    } else   {
			// We are done looping. Exit
			// fprintf(stderr, "[%3d] bytes sent is %lld\n", cp->my_rank, bytes_sent);
			done= true;
		    }
		} else   {
		    done= true;
		}
	    }
	    break;

	default:
	    _abort(alltoall_op, "[%3d] Invalid event %d in state %d\n",
		cp->my_rank, e, state);
    }

}  // end of state_MAIN_LOOP()



void
Alltoall_op::state_SEND(state_event sm_event)
{

alltoall_events_t e= (alltoall_events_t)sm_event.event;
state_event send_event;
//int dest, src;
int dest;
int offset;
int len1, len2;


    switch (e)   {
	case E_SEND_START:
	    dest= (cp->my_rank + shift) % alltoall_nranks;
	    //src= ((cp->my_rank - shift) + alltoall_nranks) % alltoall_nranks;
	    offset= (cp->my_rank * alltoall_msglen) - ((shift - 1) * alltoall_msglen);

	    if (offset < 0)   {
		// Need to break it up into two pieces
		offset= offset + (alltoall_nranks * alltoall_msglen);
		len1= (alltoall_nranks * alltoall_msglen) - offset;
		send_event.event= E_INITIAL_DATA;
		send_event.set_epoch(epoch);
		cp->send_msg(dest, len1 * sizeof(double), send_event);
		bytes_sent= bytes_sent + len1 * sizeof(double);
		len2= shift * alltoall_msglen - (alltoall_nranks * alltoall_msglen - offset);
		send_event.event= E_LAST_DATA;
		send_event.set_epoch(epoch);
		// Tricky: We only wait for the second send to finish. Receive has to be in order!
		cp->send_msg(dest, len2 * sizeof(double), send_event, E_SEND_DONE);
		expected_sends++;
		bytes_sent= bytes_sent + len2 * sizeof(double);

	    } else   {
		// I can send it in one piece
		len1= shift * alltoall_msglen;
		send_event.event= E_ALL_DATA;
		send_event.set_epoch(epoch);
		cp->send_msg(dest, len1 * sizeof(double), send_event, E_SEND_DONE);
		expected_sends++;
		bytes_sent= bytes_sent + len1 * sizeof(double);
	    }

	    shift= shift << 1;
	    i= i >> 1;
	    state= WAIT;

	    // See if the receive we are waiting for is already here
	    if (receives[epoch] == expected_receives)   {
		receives[epoch]= 0;
		got_all_receives= true;
	    }
	    break;

	default:
	    _abort(alltoall_op, "[%3d] Invalid event %d in state %d\n",
		cp->my_rank, e, state);
    }

}  // end of state_SEND()



void
Alltoall_op::state_REMAINDER(state_event sm_event)
{

alltoall_events_t e= (alltoall_events_t)sm_event.event;
state_event send_event;
int n;
int s1_len, s2_len;
//int src, dest;
int dest;


    switch (e)   {
	case E_REMAINDER_START:

	    // One more step: Each rank sends n blocks of data n ranks back, if
	    // we are n over the last power of two
	    n= alltoall_nranks - cp->next_power2(alltoall_nranks) / 2;
	    dest= (cp->my_rank + alltoall_nranks - n) % alltoall_nranks;
	    //src= (cp->my_rank + n + alltoall_nranks) % alltoall_nranks;


	    // Send one piece or two?
	    if (cp->my_rank < n - 1)   {
		// Split buffer; send two pieces
		s1_len= (n - cp->my_rank - 1) * alltoall_msglen;
		s2_len= (cp->my_rank + 1) * alltoall_msglen;

		send_event.event= E_INITIAL_DATA;
		send_event.set_epoch(epoch);
		cp->send_msg(dest, s1_len, send_event);
		bytes_sent= bytes_sent + s1_len;

		send_event.event= E_LAST_DATA;
		send_event.set_epoch(epoch);
		// Tricky: We only wait for the second send to finish. Receive has to be in order!
		cp->send_msg(dest, s2_len, send_event, E_SEND_DONE);
		expected_sends++;
		bytes_sent= bytes_sent + s2_len;
	    } else   {
		// All in one piece
		s1_len= n * alltoall_msglen;

		send_event.event= E_ALL_DATA;
		send_event.set_epoch(epoch);
		cp->send_msg(dest, s1_len, send_event, E_SEND_DONE);
		expected_sends++;
		bytes_sent= bytes_sent + s1_len;
	    }

	    shift= shift << 1;
	    i= i >> 1;
	    remainder_done= true;
	    state= WAIT;

	    // See if the receive we are waiting for is already here
	    if (receives[epoch] == expected_receives)   {
		receives[epoch]= 0;
		got_all_receives= true;
	    }
	    break;

	default:
	    _abort(alltoall_op, "[%3d] Invalid event %d in state %d\n",
		cp->my_rank, e, state);
    }

}  // end of state_REMAINDER()



void
Alltoall_op::state_WAIT(state_event sm_event)
{

alltoall_events_t e= (alltoall_events_t)sm_event.event;


    switch (e)   {
	case E_SEND_DONE:
	    // This event doesn't have epoch set, since it came through Patterns::self_event_send
	    sends++;
	    if (sends == expected_sends)   {
		expected_sends= 0;
		sends= 0;
		got_all_sends= true;
	    }
	    if (got_all_sends && got_all_receives)   {
		got_all_sends= false;
		got_all_receives= false;
		goto_state(state_MAIN_LOOP, MAIN_LOOP, E_NEXT_LOOP);
	    }
	    break;

	case E_LAST_DATA:
	case E_ALL_DATA:
	    receives[sm_event.get_epoch()]++;
	    if (receives[epoch] == expected_receives)   {
		receives[epoch]= 0;
		got_all_receives= true;
	    }

	    if (got_all_sends && got_all_receives)   {
		got_all_sends= false;
		got_all_receives= false;
		goto_state(state_MAIN_LOOP, MAIN_LOOP, E_NEXT_LOOP);
	    }
	    break;

	case E_INITIAL_DATA:
	    // We don't count these to keep things simple. There will be a
	    // E_LAST_DATA event soon.
	    break;

	default:
	    _abort(alltoall_op, "[%3d] Invalid event %d in state %d\n",
		cp->my_rank, e, state);
    }

}  // end of state_WAIT_SEND()

#ifdef SERIALIZATION_WORKS_NOW
BOOST_CLASS_EXPORT(Alltoall_op)
#endif // SERIALIZATION_WORKS_NOW
