//
// Copyright (c) 2011, IBM Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


/*

A simple allreduce operation on a tree. The exact tree is defined
in collective_topology.cc This state machine only assumes that
there is a root, interior nodes, and leaves.

During object creation, the calling module can set the default
message length.  It is used as the simulated message length for
allreduce messages.

Callers of this state machine pass in a double value in the first
Fdata field of the starting event. Allreduce returns the result of
the operation in the first Fdata field of the exit event.

The operation to be performed can be chosen by setting the first
Idata field. See allreduce_op_t in allreduce.h for possible values.
*/
#include <boost/serialization/list.hpp>
#include <sst_config.h>
#include <sst/core/serialization/element.h>
#include "allreduce_op.h"



void
Allreduce_op::handle_events(state_event sm_event)
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

    // Don't call unregisterExit()
    // Only "main" patterns should do that; i.e., patterns that use other
    // patterns like this one. Just return to our caller.
    if (done)   {
	// For allreduce, which returns data, we return the first field of
	// SM Fdata where the states keep the result
	state= START;
	done= false;
	sm_event.set_Fdata(cp->SM->SM_data.get_Fdata());
	cp->SM->SM_return(sm_event);
    }

}  /* end of handle_events() */



void
Allreduce_op::state_INIT(state_event sm_event)
{

allreduce_events_t e= (allreduce_events_t)sm_event.event;
state_event send_event;


    switch (e)   {
	case E_START:
	    // Extract and store the value passed in by the caller
	    // The operation to be performed is stored in Idata(0),
	    // and the value is in Fdata(0)
	    cp->SM->SM_data.set_Idata(sm_event.get_Idata());
	    cp->SM->SM_data.set_Fdata(sm_event.get_Fdata());

	    // The value gets passed up the tree
	    send_event.event= E_FROM_CHILD;
	    send_event.set_Fdata(sm_event.get_Fdata());

	    if (ctopo->is_leaf() && ctopo->num_nodes() > 1)   {
		cp->send_msg(ctopo->parent_rank(), allreduce_msglen, send_event);
		state= WAIT_PARENT;
	    } else   {
		// I must be an interior node or root
		if (ctopo->num_nodes() > 1)   {
		    state= WAIT_CHILDREN;
		    // See if I have already received some messages
		    while (!pending_msg.empty())   {
			assert((allreduce_events_t)pending_msg.front().event == E_FROM_CHILD);
			state_WAIT_CHILDREN(pending_msg.front());
			pending_msg.pop_front();
		    }

		} else   {
		    // I guess we're the only one == root!
		    send_event.event= E_FROM_PARENT;
		    state= WAIT_PARENT;
		    state_WAIT_PARENT(send_event);
		}
	    }
	    break;

	case E_FROM_CHILD:
	    // It's possible that other ranks have already entered the allreduce and
	    // sent us events before we entered the state machine for allreduce.
	    if (ctopo->is_leaf())   {
		// This cannot happen
		_abort(allreduce_pattern, "[%3d] Invalid event %d in state %d\n",
		    cp->my_rank, e, state);
	    } else   {
		// I must be an interior node or root
		pending_msg.push_back(sm_event);
		// FIXME: delete goto_state(state_WAIT_CHILDREN, WAIT_CHILDREN, E_FROM_CHILD);
	    }
	    break;

	default:
	    _abort(allreduce_pattern, "[%3d] Invalid event %d in state %d\n",
		cp->my_rank, e, state);
    }
}  // end of state_INIT()



void
Allreduce_op::state_WAIT_CHILDREN(state_event sm_event)
{

allreduce_events_t e= (allreduce_events_t)sm_event.event;
allreduce_op_t op;
state_event send_event;


    switch (e)   {
	case E_FROM_CHILD:
	    // Count receives from my children. When I have them all, send to parent.
	    receives++;

	    // Extract child's contribution and perform the appropriate operation on it
	    op= (allreduce_op_t)cp->SM->SM_data.get_Idata();
	    switch (op)   {
		case OP_SUM:
		    cp->SM->SM_data.set_Fdata(cp->SM->SM_data.get_Fdata() + sm_event.get_Fdata());
		    break;

		case OP_PROD:
		    cp->SM->SM_data.set_Fdata(cp->SM->SM_data.get_Fdata() * sm_event.get_Fdata());
		    break;

		case OP_MIN:
		    if (cp->SM->SM_data.get_Fdata() < sm_event.get_Fdata())   {
			// Use mine
			// cp->SM->SM_data.set_Fdata(cp->SM->SM_data.get_Fdata());
		    } else   {
			// Use the child's
			cp->SM->SM_data.set_Fdata(sm_event.get_Fdata());
		    }
		    break;

		case OP_MAX:
		    if (cp->SM->SM_data.get_Fdata() > sm_event.get_Fdata())   {
			// Use mine
			// cp->SM->SM_data.set_Fdata(cp->SM->SM_data.get_Fdata());
		    } else   {
			// Use the child's
			cp->SM->SM_data.set_Fdata(sm_event.get_Fdata());
		    }
		    break;

		default:
		    _abort(allreduce_pattern, "[%3d] Invalid operation %d\n", cp->my_rank, op);
	    }

	    if (receives == ctopo->num_children())   {
		if (ctopo->is_root())   {
		    // Send to my children (if any) and get out of here
		    if (ctopo->num_children() > 0)   {
			send_event.event= E_FROM_PARENT;
			send_event.set_Fdata(cp->SM->SM_data.get_Fdata());
			std::list<int>::iterator it;
			for (it= ctopo->children.begin(); it != ctopo->children.end(); it++)   {
			    cp->send_msg(*it, allreduce_msglen, send_event, E_SEND_DONE);
			}
			sends_complete= 0;
		    } else   {
			done= true;
		    }

		} else   {
		    send_event.event= E_FROM_CHILD;
		    send_event.set_Fdata(cp->SM->SM_data.get_Fdata());
		    cp->send_msg(ctopo->parent_rank(), allreduce_msglen, send_event);
		    state= WAIT_PARENT;
		}
		receives= 0;
	    }
	    break;

	case E_SEND_DONE:
	    assert(ctopo->is_root());
	    sends_complete++;
	    if (sends_complete == ctopo->num_children())   {
		done= true;
	    }
	    break;

	default:
	    _abort(allreduce_pattern, "[%3d] Invalid event %d in state %d\n",
		cp->my_rank, e, state);
    }

}  // end of state_WAIT_CHILDREN()



void
Allreduce_op::state_WAIT_PARENT(state_event sm_event)
{

allreduce_events_t e= (allreduce_events_t)sm_event.event;
state_event send_event;
std::list<int>::iterator it;


    switch (e)   {
	case E_FROM_PARENT:
	    // Save allreduce value received from root
	    cp->SM->SM_data.set_Fdata(sm_event.get_Fdata());
	    send_event.set_Fdata(sm_event.get_Fdata());
	    send_event.event= E_FROM_PARENT;

	    // Send to my children (if any) and get out of here
	    if (!ctopo->is_leaf())   {
		for (it= ctopo->children.begin(); it != ctopo->children.end(); it++)   {
		    cp->send_msg(*it, allreduce_msglen, send_event, E_SEND_DONE);
		}
		sends_complete= 0;
	    } else   {
		done= true;
	    }
	    break;

	case E_SEND_DONE:
	    sends_complete++;
	    if (sends_complete == ctopo->num_children())   {
		done= true;
	    }
	    break;

	case E_FROM_CHILD:
	    // A child of us may have received our data already and sent us more,
	    // while we are here stuck waiting for our sends to the other children
	    // to finish.
	    pending_msg.push_back(sm_event);
	    break;

	default:
	    _abort(allreduce_pattern, "[%3d] Invalid event %d in state %d\n",
		cp->my_rank, e, state);
    }

}  // end of state_WAIT_PARENT()


#ifdef SERIALIZARION_WORKS_NOW
BOOST_CLASS_EXPORT(Allreduce_op)
#endif // SERIALIZARION_WORKS_NOW
