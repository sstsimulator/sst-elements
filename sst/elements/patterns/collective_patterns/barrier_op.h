//
// Copyright (c) 2011, IBM Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _BARRIER_PATTERN_H
#define _BARRIER_PATTERN_H

#include "patterns.h"
#include "support/state_machine.h"
#include "support/comm_pattern.h"
#include "collective_topology.h"



class Barrier_op   {
    public:
	Barrier_op(Comm_pattern * const& current_pattern) :
	    cp(current_pattern),
	    no_data(0),
        out(Simulation::getSimulation()->getSimulationOutput())
	{
	    done= false;
	    state= START;
	    receives= 0;
	    ctopo= new Collective_topology(cp->my_rank, cp->num_ranks, TREE_DEEP);
	}

        ~Barrier_op() {}

	uint32_t install_handler(void)
	{
	    return cp->SM->SM_create((void *)this, Barrier_op::wrapper_handle_events);
	}

	// The Barrier pattern generator can be in these states and deals
	// with these events.
	typedef enum {START, WAIT_CHILDREN, WAIT_PARENT} barrier_state_t;

	// The start event should always be SM_START_EVENT
	typedef enum {E_START= SM_START_EVENT, E_FROM_CHILD, E_FROM_PARENT} barrier_events_t;


    private:
	// Wrapping a pointer to a non-static member function like this is from
	// http://www.newty.de/fpt/callback.html
	void handle_events(state_event sst_event);
	static void wrapper_handle_events(void *obj, state_event sst_event)
	{
	    Barrier_op* mySelf = (Barrier_op*) obj;
	    mySelf->handle_events(sst_event);
	}

	// We need to remember how to upcall into our parent object
	Comm_pattern *cp;

	// Barrier uses zero-length messages
	const int no_data;

	// Some more variables we need to keep track of state
	barrier_state_t state;
	int done;
	int receives;
	Collective_topology *ctopo;

    Output &out;

	void state_INIT(state_event event);
	void state_WAIT_CHILDREN(state_event event);
	void state_WAIT_PARENT(state_event event);

};

#endif // _BARRIER_PATTERN_H
