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
#include "ping_pong.h"
#include "pattern_common.h"

// It's unrealistic to be able to send without any delay
#define SEND_DELAY	(1000)	// 1 usec


#define STATE_DBG(to) \
    _PINGPONG_PATTERN_DBG(1, "[%3d] state transition to %s\n", my_rank, to);



// The gate keeper will call this function whenever there is an event
// for us, and we are active.
void
Pingpong_pattern::handle_events(pattern_event_t event)
{

    // _PINGPONG_PATTERN_DBG(2, "[%3d] In state %d and got event %d at time %lu\n", my_rank,
// 	state, event, getCurrentSimTime());

    // An event causes a transition depending on which event we are in
    //switch (state)   {
    //}

    return;

}  /* end of handle_events() */



extern "C" {
Pingpong_pattern *
pingpong_patternAllocComponent(SST::ComponentId_t id,
                          SST::Component::Params_t& params)
{
    return new Pingpong_pattern(id, params);
}
}

BOOST_CLASS_EXPORT(Pingpong_pattern)



//
// -----------------------------------------------------------------------------
// For each state in the state machine we have a method to deal with incoming
// events.
