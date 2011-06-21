//
// Copyright (c) 2011, IBM Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _BARRIER_PATTERN_H
#define _BARRIER_PATTERN_H

#include "comm_pattern.h"
#include "collective_topology.h"



class Barrier_pattern   {
    public:
	Barrier_pattern(Comm_pattern * const& current_pattern) :
	    cp(current_pattern)
	{
	    state= START;
	    epoch= 0;
	    receives[0]= 0;
	    receives[1]= 0;
	    ctopo= new Collective_topology(cp->my_rank, cp->num_ranks);
	}

        ~Barrier_pattern() {}

	uint32_t install_handler(void)
	{
	    return cp->SM_create((void *)this, Barrier_pattern::wrapper_handle_events);
	}

	// The Barrier pattern generator can be in these states and deals
	// with these events.
	typedef enum {START, WAIT_CHILDREN, WAIT_PARENT} barrier_state_t;
	typedef enum {E_START, E_FROM_CHILD, E_FROM_PARENT} barrier_events_t;


    private:
	// Wrapping a pointer to a non-static member function like this is from
	// http://www.newty.de/fpt/callback.html
	void handle_events(int sst_event);
	static void wrapper_handle_events(void *obj, int sst_event)
	{
	    Barrier_pattern* mySelf = (Barrier_pattern*) obj;
	    mySelf->handle_events(sst_event);
	}

	// We need to remember how to upcall into our parent object
	Comm_pattern *cp;

	barrier_state_t state;
	int done;
	int epoch; // Each barrier happens in a different time cycle
	int receives[2];	// Receives in each epoch
	Collective_topology *ctopo;

	void state_INIT(barrier_events_t event);
	void state_WAIT_CHILDREN(barrier_events_t event);
	void state_WAIT_PARENT(barrier_events_t event);

};

#endif // _BARRIER_PATTERN_H
