//
// Copyright (c) 2011, IBM Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


/*



*/
#include <sst_config.h>
#include "sst/core/serialization/element.h"
#include <sst/core/cpunicEvent.h>
#include "ping_pong.h"

void
Pingpong_pattern::handle_events(Event *sst_event)
{

pattern_event_t event;
CPUNicEvent *e;


    // Extract the pattern event type from the SST event                                                      
    // (We are "misusing" the routine filed in CPUNicEvent to xmit the event type
    e= static_cast<CPUNicEvent *>(sst_event);
    event= (pattern_event_t)e->GetRoutine();

    switch (event)   {
	case START:
	    // If I'm rank 0 send, otherwise wait
	    cnt= 5;
	    done= false;
	    if (my_rank == 0)   {
		printf("[%3d] sending to %d\n", my_rank, num_ranks - 1);
		data_send(num_ranks - 1, 0);
	    } else if (my_rank != num_ranks - 1)   {
		done= true;
	    }
	    break;
	case RECEIVE:
	    // We're either rank 0 or num_ranks - 1. Others don't receive.
	    // Send it back, unless we're done
	    cnt--;
	    printf("[%3d] Received a message. Cnt now %d\n", my_rank, cnt);
	    if (my_rank != 0)   {
		// We always send back
		data_send(0, 0);
		printf("[%3d] sending to %d\n", my_rank, 0);
		if (cnt < 1)   {
		    done= true;
		}
	    } else   {
		if (cnt > 0)   {
		    data_send(num_ranks - 1, 0);
		    printf("[%3d] sending to %d\n", my_rank, num_ranks - 1);
		} else   {
		    done= true;
		}
	    }
	    break;
	default:
	    printf("[%3d] Didn't really expect that (%d)\n", my_rank, event);
    }

    if (done)   {
	printf("[%3d] Done\n", my_rank);
	unregisterExit();
    }

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
