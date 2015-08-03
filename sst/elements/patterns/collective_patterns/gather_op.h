//
// Copyright (c) 2011, IBM Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _GATHER_OP_H
#define _GATHER_OP_H

#include "patterns.h"
#include "support/state_machine.h"
#include "support/comm_pattern.h"
#include "collective_topology.h"



class Gather_op   {
    public:
	Gather_op(Comm_pattern * const& current_pattern, int msglen, tree_type_t tree) :
	    cp(current_pattern),
	    gather_msglen(msglen),
	    tree_type(tree)
	{
	    // Do some initializations
	    done= false;
	    state= START;
	    receives= 0;
	    gather_msglen= msglen;

	    // Get access to a virtual tree topology
	    ctopo= new Collective_topology(cp->my_rank, cp->num_ranks, tree_type);
	}

        ~Gather_op() {}

	void resize(int new_size)
	{
	    // Get rid of previous topology
	    delete ctopo;

	    // Start again
	    ctopo= new Collective_topology(cp->my_rank, new_size, tree_type);
	}

	uint32_t install_handler(void)
	{
	    return cp->SM->SM_create((void *)this, Gather_op::wrapper_handle_events);
	}

	// The Gather pattern generator can be in these states and deals
	// with these events.
	typedef enum {START, WAIT_CHILDREN} gather_state_t;

	// The start event should always be SM_START_EVENT
	typedef enum {E_START= SM_START_EVENT, E_FROM_CHILD} gather_events_t;



    private:
	// Wrapping a pointer to a non-static member function like this is from
	// http://www.newty.de/fpt/callback.html
	void handle_events(state_event sst_event);
	static void wrapper_handle_events(void *obj, state_event sst_event)
	{
	    Gather_op* mySelf = (Gather_op*) obj;
	    mySelf->handle_events(sst_event);
	}

	// We need to remember how to upcall into our parent object
	Comm_pattern *cp;

	// Simulated message size
	int gather_msglen;

	// What should the underlying tree look like?
	tree_type_t tree_type;

	gather_state_t state;
	int done;
	int receives;
	Collective_topology *ctopo;

	void state_INIT(state_event event);
	void state_WAIT_CHILDREN(state_event event);

};

#endif // _GATHER_OP_H
