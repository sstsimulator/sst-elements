//
// Copyright (c) 2011, IBM Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


/*

A simple allreduce operation on a tree. The exact tree is defined in
collective_topology.cc This state machine only assumes that there
is a root, interior nodes, and leaves.

There are no configuration parameters for this module.

*/
#include "sst/core/serialization/element.h"
#include "allreduce.h"



void
Allreduce_pattern::handle_events(State_machine::state_event sst_event)
{

    switch (state)   {
	case START:
	    state_INIT(sst_event);
	    break;

	case WAIT_CHILDREN:
	    state_WAIT_CHILDREN(sst_event);
	    break;

	case WAIT_PARENT:
	    state_WAIT_PARENT(sst_event);
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
Allreduce_pattern::state_INIT(State_machine::state_event event)
{
    // Set the value of our contribution
    cp->SM->SM_data.set_Fdata(3.14159 * (cp->my_rank + 1));
    fprintf(stderr, "[%3d] Contributed value %6.3f\n", cp->my_rank, cp->SM->SM_data.get_Fdata());

    switch (event.event)   {
	case E_START:
	    if (ctopo->is_root())   {
		state= WAIT_CHILDREN;
	    } else if (ctopo->is_leaf())   {
		fprintf(stderr, "[%3d] Leaf: sending to parent %d, data %6.3f\n", cp->my_rank, ctopo->parent_rank(), cp->SM->SM_data.get_Fdata());
		// FIXME: This should not be no_data
		cp->send_msg(ctopo->parent_rank(), no_data, E_FROM_CHILD);
		state= WAIT_PARENT;
	    } else   {
		// I must be an interior node
		state= WAIT_CHILDREN;
	    }
	    break;

	case E_FROM_CHILD:
	    // It's possible that other ranks have already entered the allreduce and
	    // sent us events before we entered the state machine for allreduce.
	    if (ctopo->is_root())   {
		state= WAIT_CHILDREN;
		state_WAIT_CHILDREN(event);
	    } else if (ctopo->is_leaf())   {
		// This cannot happen
		_abort(allreduce_pattern, "[%3d] Invalid event %d in state %d\n", cp->my_rank, event.event, state);
	    } else   {
		// I must be an interior node
		state= WAIT_CHILDREN;
		state_WAIT_CHILDREN(event);
	    }
	    break;

	default:
	    _abort(allreduce_pattern, "[%3d] Invalid event %d in state %d\n", cp->my_rank, event.event, state);
    }
}  // end of state_INIT()



void
Allreduce_pattern::state_WAIT_CHILDREN(State_machine::state_event event)
{
    switch (event.event)   {
	case E_FROM_CHILD:
	    // Count receives from my children. When I have them all, send to parent.
	    receives++;
	    fprintf(stderr, "[%3d] Receive %d form child, waiting for %d, data is %6.3f\n", cp->my_rank, receives, ctopo->num_children(), event.get_Fdata());

	    // Extract child's contribution and add it to my current value
	    cp->SM->SM_data.set_Fdata(cp->SM->SM_data.get_Fdata() + event.get_Fdata());

	    if (receives == ctopo->num_children())   {
		if (ctopo->is_root())   {
		    fprintf(stderr, "[%3d] Root sending data %6.3f\n", cp->my_rank, cp->SM->SM_data.get_Fdata());
		    // Send to my children and get out of here
		    std::list<int>::iterator it;
		    for (it= ctopo->children.begin(); it != ctopo->children.end(); it++)   {
			// FIXME: This should not be no_data
			cp->send_msg(*it, no_data, E_FROM_PARENT);
		    }

		    state= START;  // For next allreduce
		    done= true;
		} else   {
		    fprintf(stderr, "[%3d] sending to parent %d, data %6.3f\n", cp->my_rank, ctopo->parent_rank(), cp->SM->SM_data.get_Fdata());
		    // FIXME: This should not be no_data
		    cp->send_msg(ctopo->parent_rank(), no_data, E_FROM_CHILD);
		    state= WAIT_PARENT;
		}
		receives= 0;
	    }
	    break;

	default:
	    _abort(allreduce_pattern, "[%3d] Invalid event %d in state %d\n", cp->my_rank, event.event, state);
    }
}  // end of state_WAIT_CHILDREN()



void
Allreduce_pattern::state_WAIT_PARENT(State_machine::state_event event)
{

std::list<int>::iterator it;


    switch (event.event)   {
	case E_FROM_PARENT:
	    // Send to my children and get out of here
	    fprintf(stderr, "[%3d] Receive form parent, sending to %d children, data %6.3f\n", cp->my_rank, ctopo->num_children(), event.get_Fdata());
	    // Save allreduce value received from root
	    cp->SM->SM_data.set_Fdata(event.get_Fdata());

	    for (it= ctopo->children.begin(); it != ctopo->children.end(); it++)   {
		fprintf(stderr, "[%3d] Interior: sending to child %d\n", cp->my_rank, *it);
		// FIXME: This should not be no_data
		cp->send_msg(*it, no_data, E_FROM_PARENT);
	    }

	    done= true;
	    break;

	default:
	    _abort(allreduce_pattern, "[%3d] Invalid event %d in state %d\n", cp->my_rank, event.event, state);
    }
}  // end of state_WAIT_PARENT()
