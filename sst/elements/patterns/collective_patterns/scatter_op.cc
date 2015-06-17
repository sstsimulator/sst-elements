//
// Copyright (c) 2011, IBM Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


/*

A scatter operation on a tree. The exact tree is defined
in collective_topology.cc This state machine only assumes that
there is a root, interior nodes, and leaves.

During object creation, the calling module can set the default
message length.  It is used as the simulated message length for
scatter messages.

*/
#include <sst_config.h>
#include <boost/serialization/list.hpp>
#include "scatter_op.h"



void
Scatter_op::handle_events(state_event sm_event)
{

    switch (state)   {
	case START:
	    state_INIT(sm_event);
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
Scatter_op::state_INIT(state_event sm_event)
{

scatter_events_t e= (scatter_events_t)sm_event.event;
state_event send_event;
std::list<int>::iterator it;
int msglen;


    switch (e)   {
	case E_START:
	    if (ctopo->is_root())   {
		// Send to my children and get out of here
		send_event.event= E_FROM_PARENT;

		for (it= ctopo->children.begin(); it != ctopo->children.end(); it++)   {
		    // How many grand-children do I have?
		    // That number determines how much data I have to send to each child.
		    // This does not take into consideration how much time would be
		    // required to pack (memcpy) the data before sending
		    msglen= (ctopo->num_descendants(*it) + 1) * scatter_msglen;
		    // printf("[%3d] Scatter: sending %d * %d bytes to child %d\n", cp->my_rank, ctopo->num_descendants(*it) + 1, scatter_msglen, *it);
		    cp->send_msg(*it, msglen, send_event);
		}

		done= true;
	    } else   {
		/* Have to wait until some data arrives */
		state= WAIT_PARENT;
	    }
	    break;

	default:
	    Simulation::getSimulation()->getSimulationOutput().fatal(CALL_INFO, -1, "[%3d] Invalid event %d in state %d\n",
		cp->my_rank, e, state);
    }

}  // end of state_INIT()



void
Scatter_op::state_WAIT_PARENT(state_event sm_event)
{

scatter_events_t e= (scatter_events_t)sm_event.event;
state_event send_event;
std::list<int>::iterator it;
int msglen;


    switch (e)   {
	case E_FROM_PARENT:
	    // Send to my children and get out of here
	    send_event.event= E_FROM_PARENT;

	    for (it= ctopo->children.begin(); it != ctopo->children.end(); it++)   {
		// How many grand-children do I have?
		// That number determines how much data I have to send to each child.
		// This does not take into consideration how much time would be
		// required to pack (memcpy) the data before sending
		msglen= (ctopo->num_descendants(*it) + 1) * scatter_msglen;
		// printf("[%3d] Scatter: sending %d * %d bytes to child %d\n", cp->my_rank, ctopo->num_descendants(*it) + 1, scatter_msglen, *it);
		cp->send_msg(*it, msglen, send_event);
	    }

	    done= true;
	    break;

	default:
	    Simulation::getSimulation()->getSimulationOutput().fatal(CALL_INFO, -1, "[%3d] Invalid event %d in state %d\n",
		cp->my_rank, e, state);
    }

}  // end of state_WAIT_PARENT()

#ifdef SERIALIZATION_WORKS_NOW
BOOST_CLASS_EXPORT(Scatter_op)
#endif // SERIALIZATION_WORKS_NOW
