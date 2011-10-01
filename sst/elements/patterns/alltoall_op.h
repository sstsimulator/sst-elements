//
// Copyright (c) 2011, IBM Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _ALLTOALL_OP_H
#define _ALLTOALL_OP_H

#include "state_machine.h"
#include "comm_pattern.h"



class Alltoall_op   {
    public:
	Alltoall_op(Comm_pattern * const& current_pattern, int msglen) :
	    cp(current_pattern),
	    alltoall_msglen(msglen)
	{
	    // Do some initializations
	    done= false;
	    state= START;
	    alltoall_nranks= cp->num_ranks;
	    if (!cp->is_pow2(alltoall_nranks))   {
		_abort(alltoall_pattern, "[%3d] num_ranks (%d) must be power of 2!\n",
		    cp->my_rank, alltoall_nranks);
	    }
	}

        ~Alltoall_op() {}

	void resize(int new_size)
	{
	    alltoall_nranks= new_size;
	    if (!cp->is_pow2(alltoall_nranks))   {
		_abort(alltoall_pattern, "[%3d] num_ranks (%d) must be power of 2!\n",
		    cp->my_rank, alltoall_nranks);
	    }
	}

	uint32_t install_handler(void)
	{
	    return cp->SM->SM_create((void *)this, Alltoall_op::wrapper_handle_events);
	}

	// The Alltoall pattern generator can be in these states and deals
	// with these events.
	typedef enum {START, MAIN_LOOP, WAIT_DATA} alltoall_state_t;

	// The start event should always be SM_START_EVENT
	typedef enum {E_START= SM_START_EVENT, E_NEXT_LOOP, E_INITIAL_DATA,
	    E_SEND_DONE, E_LAST_DATA, E_ALL_DATA} alltoall_events_t;



    private:
	// Wrapping a pointer to a non-static member function like this is from
	// http://www.newty.de/fpt/callback.html
	void handle_events(state_event sst_event);
	static void wrapper_handle_events(void *obj, state_event sst_event)
	{
	    Alltoall_op* mySelf = (Alltoall_op*) obj;
	    mySelf->handle_events(sst_event);
	}

	// We need to remember how to upcall into our parent object
	Comm_pattern *cp;

	// Simulated message size and communicator size
	int alltoall_msglen;
	int alltoall_nranks;

	alltoall_state_t state;
	int done;
	unsigned int i;
	int shift, dest;
	int src;
	int offset;
	int len1, len2;

	void state_INIT(state_event event);
	void state_MAIN_LOOP(state_event event);
	void state_WAIT_DATA(state_event event);

};

#endif // _ALLTOALL_OP_H
