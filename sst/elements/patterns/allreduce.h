//
// Copyright (c) 2011, IBM Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _ALLREDUCE_PATTERN_H
#define _ALLREDUCE_PATTERN_H

#include "comm_pattern.h"
#include "collective_topology.h"



class Allreduce_pattern   {
    public:
	Allreduce_pattern(Comm_pattern * const& current_pattern) :
	    cp(current_pattern)
	{
	    state= START;
	    receives= 0;
	    ctopo= new Collective_topology(cp->my_rank, cp->num_ranks);
	}

        ~Allreduce_pattern() {}

	uint32_t install_handler(void)
	{
	    return cp->SM_create((void *)this, Allreduce_pattern::wrapper_handle_events);
	}

	// The Allreduce pattern generator can be in these states and deals
	// with these events.
	typedef enum {START, WAIT_CHILDREN, WAIT_PARENT} allreduce_state_t;
	typedef enum {E_START, E_FROM_CHILD, E_FROM_PARENT} allreduce_events_t;


    private:
	// Wrapping a pointer to a non-static member function like this is from
	// http://www.newty.de/fpt/callback.html
	void handle_events(int sst_event);
	static void wrapper_handle_events(void *obj, int sst_event)
	{
	    Allreduce_pattern* mySelf = (Allreduce_pattern*) obj;
	    mySelf->handle_events(sst_event);
	}

	// We need to remember how to upcall into our parent object
	Comm_pattern *cp;

	allreduce_state_t state;
	int done;
	int receives;
	Collective_topology *ctopo;

	void state_INIT(allreduce_events_t event);
	void state_WAIT_CHILDREN(allreduce_events_t event);
	void state_WAIT_PARENT(allreduce_events_t event);

};

#endif // _ALLREDUCE_PATTERN_H
