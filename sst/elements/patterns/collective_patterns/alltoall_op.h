//
// Copyright (c) 2011, IBM Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _ALLTOALL_OP_H
#define _ALLTOALL_OP_H

#include "patterns.h"
#include "support/state_machine.h"
#include "support/comm_pattern.h"

#define MAX_EPOCH	(6)	// FIXME: Not sure why 2 or 3 is not enough...



class Alltoall_op   {
    public:
	// alltoall_msglen is in bytes
	Alltoall_op(Comm_pattern * const& current_pattern, int msglen) :
	    cp(current_pattern),
	    alltoall_msglen(msglen)
	{
	    // Do some initializations
	    done= false;
	    state= START;
	    alltoall_nranks= cp->num_ranks;
	    for (int i= 0; i < MAX_EPOCH; i++)   {
		receives[i]= 0;
	    }
	    sends= 0;
	    got_all_sends= false;
	    got_all_receives= false;
	    expected_sends= 0;
	    expected_receives= 1;
	    epoch= 0;
	}

        ~Alltoall_op() {}

	void resize(int new_size)
	{
	    alltoall_nranks= new_size;
	    epoch= 0;
	}

	uint32_t install_handler(void)
	{
	    int ID;

	    ID= cp->SM->SM_create((void *)this, Alltoall_op::wrapper_handle_events);
	    return ID;
	}

	// The Alltoall pattern generator can be in these states and deals
	// with these events.
	typedef enum {START, MAIN_LOOP, SEND, REMAINDER, WAIT} alltoall_state_t;

	// The start event should always be SM_START_EVENT
	typedef enum {E_START= SM_START_EVENT, E_NEXT_LOOP= 200, E_INITIAL_DATA= 201,
	    E_MEMCPY_DONE= 202, E_SEND_START= 203, E_SEND_DONE= 204, E_REMAINDER_START= 205,
	    E_LAST_DATA= 206, E_ALL_DATA= 207} alltoall_events_t;



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
	int shift;
	int receives[MAX_EPOCH];
	int sends;
	bool got_all_sends;
	bool got_all_receives;
	int expected_sends;
	int expected_receives;
	long long bytes_sent;
	bool remainder_done;
	int epoch;

	void state_INIT(state_event event);
	void state_MAIN_LOOP(state_event event);
	void state_SEND(state_event event);
	void state_REMAINDER(state_event event);
	void state_WAIT(state_event event);

};

#endif // _ALLTOALL_OP_H
