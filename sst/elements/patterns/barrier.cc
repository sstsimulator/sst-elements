//
// Copyright (c) 2011, IBM Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


/*

A simple barrier operation on a tree. The exact tree is defined in
collective_topology.cc This state machine only assumes that there
is a root, interior nodes, and leaves.

There are no configuration parameters for this module.

*/
#include "sst/core/serialization/element.h"
#include "barrier.h"



void
Barrier_pattern::handle_events(State_machine::state_event_t sst_event)
{

barrier_events_t event;


    event= (barrier_events_t)sst_event.event;

    switch (state)   {
	case START:
	    state_INIT(event);
	    break;

	case WAIT_CHILDREN:
	    state_WAIT_CHILDREN(event);
	    break;

	case WAIT_PARENT:
	    state_WAIT_PARENT(event);
	    break;
    }

    // Don't call unregisterExit()
    // Only "main" patterns should do that; i.e., patterns that use other
    // patterns like this one. Just return to our caller.
    if (done)   {
	cp->SM->SM_return();
    }

}  /* end of handle_events() */



void
Barrier_pattern::state_INIT(barrier_events_t event)
{

    switch (event)   {
	case E_START:
	    if (ctopo->is_root())   {
		state= WAIT_CHILDREN;
	    } else if (ctopo->is_leaf())   {
		cp->data_send(ctopo->parent_rank(), no_data, E_FROM_CHILD);
		state= WAIT_PARENT;
	    } else   {
		// I must be an interior node
		state= WAIT_CHILDREN;
	    }
	    break;

	default:
	    _abort(barrier_pattern, "[%3d] Invalid event %d in state %d\n", cp->my_rank, event, state);
    }

}  // end of state_INIT()



void
Barrier_pattern::state_WAIT_CHILDREN(barrier_events_t event)
{

    switch (event)   {
	case E_FROM_CHILD:
	    // Count receives from my children. When I have them all, send to parent.
	    receives++;
	    if (receives == ctopo->num_children())   {
		if (ctopo->is_root())   {
		    // Send to my children and get out of here
		    std::list<int>::iterator it;
		    for (it= ctopo->children.begin(); it != ctopo->children.end(); it++)   {
			cp->data_send(*it, no_data, E_FROM_PARENT);
		    }

		    state= START;  // For next barrier
		    done= true;
		} else   {
		    cp->data_send(ctopo->parent_rank(), no_data, E_FROM_CHILD);
		    state= WAIT_PARENT;
		}
		receives= 0;
	    }
	    break;

	default:
	    _abort(barrier_pattern, "[%3d] Invalid event %d in state %d\n", cp->my_rank, event, state);
    }

}  // end of state_WAIT_CHILDREN()



void
Barrier_pattern::state_WAIT_PARENT(barrier_events_t event)
{

std::list<int>::iterator it;


    switch (event)   {
	case E_FROM_PARENT:
	    // Send to my children and get out of here
	    for (it= ctopo->children.begin(); it != ctopo->children.end(); it++)   {
		cp->data_send(*it, no_data, E_FROM_PARENT);
	    }
	    state= START;  // For next barrier
	    done= true;
	    break;

	default:
	    _abort(barrier_pattern, "[%3d] Invalid event %d in state %d\n", cp->my_rank, event, state);
    }

}  // end of state_WAIT_PARENT()
