//
// Copyright (c) 2011, IBM Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


/*

A gather operation on a tree. The exact tree is defined
in collective_topology.cc This state machine only assumes that
there is a root, interior nodes, and leaves.

During object creation, the calling module can set the default
message length.  It is used as the simulated message length for
gather messages.

*/
#include <sst_config.h>
#include "gather_op.h"



void
Gather_op::handle_events(state_event sm_event)
{

    switch (state)   {
	case START:
	    state_INIT(sm_event);
	    break;

	case WAIT_CHILDREN:
	    state_WAIT_CHILDREN(sm_event);
	    break;
    }

    // Don't call primaryComponentOKToEndSim()
    // Only "main" patterns should do that; i.e., patterns that use other
    // patterns like this one. Just return to our caller.
    if (done)   {
	state= START;
	receives= 0;
	done= false;
	cp->SM->SM_return(sm_event);
    }

}  /* end of handle_events() */



void
Gather_op::state_INIT(state_event sm_event)
{

gather_events_t e= (gather_events_t)sm_event.event;
state_event send_event;


    switch (e)   {
	case E_START:
	    if (ctopo->is_leaf() && ctopo->num_nodes() > 1)   {
		// printf("[%3d] Gather: sending %d * %d bytes to parent %d\n", cp->my_rank, 1, gather_msglen, ctopo->parent_rank());
		send_event.event= E_FROM_CHILD;
		cp->send_msg(ctopo->parent_rank(), gather_msglen, send_event);
		done= true;
	    } else   {
		// I must be an interior node or root
		if (ctopo->num_nodes() > 1)   {
		    state= WAIT_CHILDREN;
		} else   {
		    // I guess we're the only one
		    done= true;
		}
	    }
	    break;

	case E_FROM_CHILD:
	    // It's possible that other ranks have already entered the allreduce and
	    // sent us events before we entered the state machine for allreduce.
	    if (ctopo->is_leaf())   {
		// This cannot happen
		Simulation::getSimulation()->getSimulationOutput().fatal(CALL_INFO, -1, "[%3d] Invalid event %d in state %d\n",
		    cp->my_rank, e, state);
	    } else   {
		// I must be an interior node or root
		goto_state(state_WAIT_CHILDREN, WAIT_CHILDREN, E_FROM_CHILD);
	    }
	    break;


	default:
	    Simulation::getSimulation()->getSimulationOutput().fatal(CALL_INFO, -1, "[%3d] Invalid event %d in state %d\n",
		cp->my_rank, e, state);
    }

}  // end of state_INIT()



void
Gather_op::state_WAIT_CHILDREN(state_event sm_event)
{

gather_events_t e= (gather_events_t)sm_event.event;
state_event send_event;
int msglen;


    switch (e)   {
	case E_FROM_CHILD:
	    // Count receives from my children. When I have them all, send to parent.
	    receives++;

	    if (receives == ctopo->num_children())   {
		if (!ctopo->is_root())   {
		    // Pass it up the tree
		    send_event.event= E_FROM_CHILD;

		    // Send what we received plus our own contribution
		    msglen= (ctopo->num_descendants(cp->my_rank) + 1) * gather_msglen;
		    // printf("[%3d] Gather: sending %d * %d bytes on to parent %d\n", cp->my_rank, ctopo->num_descendants(cp->my_rank) + 1, gather_msglen, ctopo->parent_rank());
		    send_event.event= E_FROM_CHILD;
		    cp->send_msg(ctopo->parent_rank(), msglen, send_event);
		}
		done= true;
	    }
	    break;

	default:
	    Simulation::getSimulation()->getSimulationOutput().fatal(CALL_INFO, -1, "[%3d] Invalid event %d in state %d\n",
		cp->my_rank, e, state);
    }

}  // end of state_WAIT_CHILDREN()

