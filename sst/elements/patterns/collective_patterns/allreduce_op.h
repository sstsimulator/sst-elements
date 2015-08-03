//
// Copyright (c) 2011, IBM Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _ALLREDUCE_OP_H
#define _ALLREDUCE_OP_H

#include "patterns.h"
#include "support/state_machine.h"
#include "support/comm_pattern.h"
#include "collective_topology.h"



class Allreduce_op   {
    public:
	// msglen is in bytes
	Allreduce_op(Comm_pattern * const& current_pattern, int msglen, tree_type_t tree) :
	    cp(current_pattern),
	    allreduce_msglen(msglen),
	    tree_type(tree)
	{
	    // Do some initializations
	    done= false;
	    state= START;
	    receives= 0;
	    sends_complete= 0;
	    vector_op_pending= 0;
	    epoch= 0;
	    memcpy_done= false;
	    active= false;

	    // Get access to a virtual tree topology
	    ctopo= new Collective_topology(cp->my_rank, cp->num_ranks, tree_type);
	}

        ~Allreduce_op() {}

	void resize(int new_size)
	{
	    assert(ctopo);
	    assert((sends_complete == 0) || sends_complete == ctopo->num_children());
	    assert((receives == 0) || receives == ctopo->num_children());

	    // Get rid of previous topology
	    delete ctopo;

	    // Start again
	    ctopo= new Collective_topology(cp->my_rank, new_size, tree_type);
	    epoch= 0;
	}

	uint32_t install_handler(void)
	{
	    return cp->SM->SM_create((void *)this, Allreduce_op::wrapper_handle_events);
	}

	// The Allreduce pattern generator can be in these states and deals
	// with these events.
	typedef enum {START, WAIT_MEMCPY, WAIT_VECTOR_OP, WAIT_CHILDREN,
	    WAIT_PARENT} allreduce_state_t;

	// The start event should always be SM_START_EVENT
	typedef enum {E_START= SM_START_EVENT, E_MEMCPY_DONE, E_VECTOR_OP_DONE,
	    E_SEND_DONE, E_FROM_CHILD, E_FROM_PARENT} allreduce_events_t;

	// The operations we support
	typedef enum {OP_SUM, OP_PROD, OP_MIN, OP_MAX} allreduce_op_t;


    private:
	// Wrapping a pointer to a non-static member function like this is from
	// http://www.newty.de/fpt/callback.html
	void handle_events(state_event sst_event);
	static void wrapper_handle_events(void *obj, state_event sst_event)
	{
	    Allreduce_op* mySelf = (Allreduce_op*) obj;
	    mySelf->handle_events(sst_event);
	}

	// We need to remember how to upcall into our parent object
	Comm_pattern *cp;

	// Simulated message size in bytes
	int allreduce_msglen;

	// What should the underlying tree look like?
	tree_type_t tree_type;

	allreduce_state_t state;
	int done;
	int receives;
	int sends_complete;
	Collective_topology *ctopo;

	std::list <state_event>pending_msg[2];
	int vector_op_pending;
	int epoch;
	bool memcpy_done;
	bool active;

	void state_INIT(state_event event);
	void state_WAIT_MEMCPY(state_event event);
	void state_WAIT_VECTOR_OP(state_event event);
	void state_WAIT_CHILDREN(state_event event);
	void state_WAIT_PARENT(state_event event);

};

#endif // _ALLREDUCE_OP_H
