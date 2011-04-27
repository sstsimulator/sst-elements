// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


/*


*/
#include <sst_config.h>
#include "sst/core/serialization/element.h"
#include <sst/core/cpunicEvent.h>
#include <queue>
#include <assert.h>
#include "gate_keeper.h"
#include "pattern_common.h"

#define STATE_DBG(to) \
    _GATE_KEEPER_DBG(1, "[%3d] state transition to %s\n", my_rank, to);



void
Gate_keeper::handle_events(CPUNicEvent *e)
{

pattern_event_t event;


    // Extract the pattern event type from the SST event
    // (We are "misusing" the routine filed in CPUNicEvent to xmit the event type
    event= (pattern_event_t)e->GetRoutine();

    // We need to figure out which of the state machines is currently active
    // If this even is of the active machine, we send it down. Otherwise we
    // queue it.


    // We're done
    delete(e);

    return;

}  /* end of handle_events() */



// Messages from the global network
void
Gate_keeper::handle_net_events(Event *sst_event)
{

CPUNicEvent *e;


    e= static_cast<CPUNicEvent *>(sst_event);

    if (e->dest != my_rank)   {
	_abort(gate_keeper, "NETWORK dest %d != my rank %d\n", e->dest, my_rank);
    }

    handle_events(e);

}  /* end of handle_net_events() */



// Messages from the local chip network
void
Gate_keeper::handle_NoC_events(Event *sst_event)
{

CPUNicEvent *e;


    e= static_cast<CPUNicEvent *>(sst_event);

    if (e->dest != my_rank)   {
	_abort(gate_keeper, "NoC dest %d != my rank %d\n", e->dest, my_rank);
    }

    handle_events(e);

}  /* end of handle_NoC_events() */



// When we send to ourselves, we come here.
// Just pass it on to the main handler above
void
Gate_keeper::handle_self_events(Event *sst_event)
{

CPUNicEvent *e;


    e= static_cast<CPUNicEvent *>(sst_event);
    handle_events(e);

}  /* end of handle_self_events() */



// Events from the local NVRAM
void
Gate_keeper::handle_nvram_events(Event *sst_event)
{

CPUNicEvent *e;


    // _abort(gate_keeper, "We should not get NVRAM events in this pattern!\n");
    e= static_cast<CPUNicEvent *>(sst_event);
    handle_events(e);

}  /* end of handle_nvram_events() */



// Events from stable storage
void
Gate_keeper::handle_storage_events(Event *sst_event)
{

CPUNicEvent *e;


    // _abort(gate_keeper, "We should not get stable storage events in this pattern!\n");
    e= static_cast<CPUNicEvent *>(sst_event);
    handle_events(e);

}  /* end of handle_storage_events() */



extern "C" {
Gate_keeper *
gate_keeperAllocComponent(SST::ComponentId_t id,
                          SST::Component::Params_t& params)
{
    return new Gate_keeper(id, params);
}
}

BOOST_CLASS_EXPORT(Gate_keeper)



//
// -----------------------------------------------------------------------------
// Implementation of the gate keeper API
