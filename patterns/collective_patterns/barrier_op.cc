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
#include <sst_config.h>
#include "barrier_op.h"



void
Barrier_op::handle_events(state_event sm_event)
{
    switch (state)   {
	case START:
	    state_INIT(sm_event);
	    break;

	case WAIT_CHILDREN:
	    state_WAIT_CHILDREN(sm_event);
	    break;

	case WAIT_PARENT:
	    state_WAIT_PARENT(sm_event);
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
Barrier_op::state_INIT(state_event sm_event)
{

barrier_events_t e= (barrier_events_t)sm_event.event;
state_event barrier_event;


    switch (e)   {
	case E_START:
	    if (ctopo->is_root())   {
		state= WAIT_CHILDREN;
	    } else if (ctopo->is_leaf())   {
		barrier_event.event= E_FROM_CHILD;
		cp->send_msg(ctopo->parent_rank(), no_data, barrier_event);
		state= WAIT_PARENT;
	    } else   {
		// I must be an interior node
		state= WAIT_CHILDREN;
	    }
	    break;

	default:
	    out.fatal(CALL_INFO, -1, "[%3d] Invalid event %d in state %d\n", cp->my_rank, e, state);
    }

}  // end of state_INIT()



void
Barrier_op::state_WAIT_CHILDREN(state_event sm_event)
{

barrier_events_t e= (barrier_events_t)sm_event.event;
state_event barrier_event;


    switch (e)   {
	case E_FROM_CHILD:
	    // Count receives from my children. When I have them all, send to parent.
	    receives++;
	    if (receives == ctopo->num_children())   {
		if (ctopo->is_root())   {
		    // Send to my children and get out of here
		    barrier_event.event= E_FROM_PARENT;
		    std::list<int>::iterator it;
		    for (it= ctopo->children.begin(); it != ctopo->children.end(); it++)   {
			// FIXME: Should wait for these sends to finish
			cp->send_msg(*it, no_data, barrier_event);
		    }

		    done= true;
		} else   {
		    barrier_event.event= E_FROM_CHILD;
		    cp->send_msg(ctopo->parent_rank(), no_data, barrier_event);
		    state= WAIT_PARENT;
		}
		receives= 0;
	    }
	    break;

	default:
	    out.fatal(CALL_INFO, -1, "[%3d] Invalid event %d in state %d\n", cp->my_rank, e, state);
    }

}  // end of state_WAIT_CHILDREN()



void
Barrier_op::state_WAIT_PARENT(state_event sm_event)
{

std::list<int>::iterator it;
barrier_events_t e= (barrier_events_t)sm_event.event;
state_event barrier_event;



    switch (e)   {
	case E_FROM_PARENT:
	    // Send to my children and get out of here
	    barrier_event.event= E_FROM_PARENT;
	    for (it= ctopo->children.begin(); it != ctopo->children.end(); it++)   {
		// FIXME: Should wait for these sends to finish
		cp->send_msg(*it, no_data, barrier_event);
	    }
	    done= true;
	    break;

	default:
	    out.fatal(CALL_INFO, -1, "[%3d] Invalid event %d in state %d\n", cp->my_rank, e, state);
    }

}  // end of state_WAIT_PARENT()

#ifdef SERIALIZATION_WORKS_NOW
BOOST_CLASS_EXPORT(Barrier_op)
#endif // SERIALIZATION_WORKS_NOW
