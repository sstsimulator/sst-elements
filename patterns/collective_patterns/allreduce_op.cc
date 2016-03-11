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
#include <sst_config.h>
#include "allreduce_op.h"



void
Allreduce_op::handle_events(state_event sm_event)
{

    switch (state)   {
	case START:
	    state_INIT(sm_event);
	    break;

	case WAIT_MEMCPY:
	    state_WAIT_MEMCPY(sm_event);
	    break;

	case WAIT_VECTOR_OP:
	    state_WAIT_VECTOR_OP(sm_event);
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
	assert(pending_msg[epoch % 2].empty());
	assert(sends_complete == ctopo->num_children());
	assert(receives == ctopo->num_children());

	// For allreduce, which returns data, we return the first field of
	// SM Fdata where the states keep the result
	state= START;
	active= false;
	done= false;
	sends_complete= 0;
	receives= 0;
	sm_event.set_Fdata(cp->SM->SM_data.get_Fdata());
	cp->SM->SM_return(sm_event);
    }

}  /* end of handle_events() */



void
Allreduce_op::state_INIT(state_event sm_event)
{

allreduce_events_t e= (allreduce_events_t)sm_event.event;


    switch (e)   {
	case E_START:
	    // Extract and store the value passed in by the caller
	    // The operation to be performed is stored in Idata(0),
	    // and the value is in Fdata(0)
	    assert(pending_msg[epoch % 2].empty()); // previous epoch must be empty!
	    epoch++;

	    assert(!active);
	    assert(epoch >= 0);
	    active= true;
	    assert(cp->SM);
	    cp->SM->SM_data.set_Idata(sm_event.get_Idata());
	    cp->SM->SM_data.set_Fdata(sm_event.get_Fdata());

	    if (ctopo->is_leaf() && ctopo->num_nodes() > 1)   {
		state_event send_event;

		// The value gets passed up the tree
		send_event.event= E_FROM_CHILD;
		send_event.set_epoch(epoch);
		send_event.set_Fdata(sm_event.get_Fdata());

		cp->send_msg(ctopo->parent_rank(), allreduce_msglen, send_event);
		state= WAIT_PARENT;
	    } else   {
		// I must be an interior node or root
		// Copy my data contribution to the output buffer
		assert(!ctopo->is_leaf() || ctopo->is_root());
		memcpy_done= false;
		cp->memcpy(E_MEMCPY_DONE, allreduce_msglen);
		state= WAIT_MEMCPY;
	    }
	    break;

	case E_FROM_CHILD:
	    // It's possible that other ranks have already entered the allreduce and
	    // sent us events before we entered the state machine for allreduce.
	    assert(!ctopo->is_leaf());
	    // I must be an interior node or root
	    assert(sm_event.get_epoch() >= 0);
	    pending_msg[sm_event.get_epoch() % 2].push_back(sm_event);
	    break;

	default:
	    Simulation::getSimulation()->getSimulationOutput().fatal(CALL_INFO, -1, "[%3d] Invalid event %d in state %d\n",
		cp->my_rank, e, state);
    }

}  // end of state_INIT()



void
Allreduce_op::state_WAIT_MEMCPY(state_event sm_event)
{

allreduce_events_t e= (allreduce_events_t)sm_event.event;
state_event send_event;


    switch (e)   {
	case E_MEMCPY_DONE:
	    assert(active);
	    assert(!memcpy_done);
	    memcpy_done= true;
	    if (ctopo->num_nodes() > 1)   {
		state= WAIT_CHILDREN;
		if (!pending_msg[epoch % 2].empty())   {
		    // We have at least one child event pending, possibly from the previous
		    // epoch. Process it now
		    assert((allreduce_events_t)pending_msg[epoch % 2].front().event == E_FROM_CHILD);
		    send_event= pending_msg[epoch % 2].front();
		    pending_msg[epoch % 2].pop_front();
		    state_WAIT_CHILDREN(send_event);
		}
	    } else   {
		// I guess we're the only one == root!
		assert(ctopo->is_root());
		send_event.event= E_FROM_PARENT;
		send_event.set_epoch(epoch);
		state= WAIT_PARENT;
		state_WAIT_PARENT(send_event);
	    }
	    break;

	case E_FROM_CHILD:
	    // It's possible that other ranks are sending us data while we are
	    // copying ours. This can only happen, if we are not a leaf
	    assert(!ctopo->is_leaf());
	    // I must be an interior node or root
	    assert(sm_event.get_epoch() >= 0);
	    pending_msg[sm_event.get_epoch() % 2].push_back(sm_event);
	    break;

	default:
	    Simulation::getSimulation()->getSimulationOutput().fatal(CALL_INFO, -1, "[%3d] Invalid event %d in state %d\n",
		cp->my_rank, e, state);
    }

}  // end of state_WAIT_MEMCPY()



void
Allreduce_op::state_WAIT_CHILDREN(state_event sm_event)
{

allreduce_events_t e= (allreduce_events_t)sm_event.event;
allreduce_op_t op;


    assert(active);
    switch (e)   {
	case E_FROM_CHILD:
	    assert(memcpy_done);
	    if ((epoch % 2) != (sm_event.get_epoch() % 2))   {
		// This is a child event from the next epoch. Save it for now
		assert(sm_event.get_epoch() >= 0);
		pending_msg[sm_event.get_epoch() % 2].push_back(sm_event);
		break;
	    }

	    // Count receives from my children. When I have them all, send to parent.

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
		    Simulation::getSimulation()->getSimulationOutput().fatal(CALL_INFO, -1, "[%3d] Invalid operation %d\n", cp->my_rank, op);
	    }

	    // Fake doing the operation on each double
	    state= WAIT_VECTOR_OP;
	    cp->vector_op(E_VECTOR_OP_DONE, allreduce_msglen / sizeof(double));
	    assert(!vector_op_pending);
	    vector_op_pending++;
	    break;

	case E_SEND_DONE:
	    assert(ctopo->is_root());
	    sends_complete++;
	    if (sends_complete == ctopo->num_children())   {
		done= true;
	    }
	    break;

	default:
	    Simulation::getSimulation()->getSimulationOutput().fatal(CALL_INFO, -1, "[%3d] Invalid event %d in state %d\n",
		cp->my_rank, e, state);
    }

}  // end of state_WAIT_CHILDREN()



void
Allreduce_op::state_WAIT_VECTOR_OP(state_event sm_event)
{

allreduce_events_t e= (allreduce_events_t)sm_event.event;


    assert(active);
    switch (e)   {
	case E_VECTOR_OP_DONE:
	    // Unless we're done, continue waiting for child data
	    vector_op_pending--;
	    receives++;
	    state= WAIT_CHILDREN;
	    assert(!vector_op_pending);

	    // process next pending child receive, if any, and we are not expecting any
	    // more events
	    if (!pending_msg[epoch % 2].empty() && (receives + (int)pending_msg[epoch % 2].size() == ctopo->num_children()))   {
		state_event send_event;

		// We received all child messages, but have not processed them all yet
		assert((allreduce_events_t)pending_msg[epoch % 2].front().event == E_FROM_CHILD);
		send_event= pending_msg[epoch % 2].front();
		pending_msg[epoch % 2].pop_front();
		state_WAIT_CHILDREN(send_event);
	    }

	    if (receives == ctopo->num_children())   {
		state_event send_event;

		if (ctopo->is_root())   {
		    // Send to my children (if any) and get out of here
		    assert(ctopo->num_children() > 0);
		    send_event.event= E_FROM_PARENT;
		    send_event.set_epoch(epoch);
		    send_event.set_Fdata(cp->SM->SM_data.get_Fdata());
		    std::list<int>::iterator it;
		    for (it= ctopo->children.begin(); it != ctopo->children.end(); it++)   {
			cp->send_msg(*it, allreduce_msglen, send_event, E_SEND_DONE);
		    }
		    sends_complete= 0;

		} else   {
		    send_event.event= E_FROM_CHILD;
		    send_event.set_epoch(epoch);
		    send_event.set_Fdata(cp->SM->SM_data.get_Fdata());
		    cp->send_msg(ctopo->parent_rank(), allreduce_msglen, send_event);
		    state= WAIT_PARENT;
		}
	    }
	    break;

	case E_SEND_DONE:
	    assert(ctopo->is_root());
	    sends_complete++;
	    if (sends_complete == ctopo->num_children())   {
		done= true;
	    }
	    break;

	case E_FROM_CHILD:
	    // It's possible that other ranks are sending us data while we are
	    // working on ours.
	    assert(!ctopo->is_leaf());
	    // I must be an interior node or root
	    assert(sm_event.get_epoch() >= 0);
	    pending_msg[sm_event.get_epoch() % 2].push_back(sm_event);
	    break;

	default:
	    Simulation::getSimulation()->getSimulationOutput().fatal(CALL_INFO, -1, "[%3d] Invalid event %d in state %d\n",
		cp->my_rank, e, state);
    }

}  // end of state_WAIT_VECTOR_OP()



void
Allreduce_op::state_WAIT_PARENT(state_event sm_event)
{

allreduce_events_t e= (allreduce_events_t)sm_event.event;
std::list<int>::iterator it;


    switch (e)   {
	case E_FROM_PARENT:
	    assert(sm_event.get_epoch() == epoch);
	    // Save allreduce value received from root
	    cp->SM->SM_data.set_Fdata(sm_event.get_Fdata());

	    // Send to my children (if any) and get out of here
	    if (!ctopo->is_leaf())   {
		state_event send_event;

		send_event.set_Fdata(sm_event.get_Fdata());
		send_event.event= E_FROM_PARENT;
		send_event.set_epoch(epoch);

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
	    assert(sm_event.get_epoch() >= 0);
	    pending_msg[sm_event.get_epoch() % 2].push_back(sm_event);
	    break;

	default:
	    Simulation::getSimulation()->getSimulationOutput().fatal(CALL_INFO, -1, "[%3d] Invalid event %d in state %d\n",
		cp->my_rank, e, state);
    }

}  // end of state_WAIT_PARENT()


