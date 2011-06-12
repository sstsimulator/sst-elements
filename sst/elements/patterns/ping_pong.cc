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
Pingpong_pattern::handle_events(int sst_event)
{

pattern_event_t event;
double execution_time;
double latency;


    // Extract the pattern event type from the SST event                                                      
    // (We are "misusing" the routine filed in CPUNicEvent to xmit the event type
    event= (pattern_event_t)sst_event;

    switch (event)   {
	case START:
	    cnt= num_msg;
	    done= false;
	    first_receive= true;
	    len= 0; // Starting length


	    if (my_rank == 0)   {
		printf("# [%3d] I'm at X,Y %3d/%-3d in the network, and x,y %3d/%-3d in the NoC\n",
			my_rank, myNetX(), myNetY(), myNoCX(), myNoCY());
		printf("# [%3d] Num msg per msg size: %d\n", my_rank, num_msg);
		start_time= getCurrentSimTime();

		// If I'm rank 0 send, otherwise wait
		data_send(dest, len);
	    } else if (my_rank != dest)   {
		done= true;
	    } else   {
		printf("# [%3d] I'm at X,Y %3d/%-3d in the network, and x,y %3d/%-3d in the NoC\n",
			my_rank, myNetX(), myNetY(), myNoCX(), myNoCY());
	    }
	    break;

	case RECEIVE:
	    // We're either rank 0 or dest. Others don't receive.
	    // Send it back, unless we're done
	    cnt--;
	    if (first_receive)   {
		// printf("# [%3d] Number of hops (routers) along path: %d\n", my_rank, e->hops);
		if (my_rank == 0)   {
		    printf("#\n");
		    printf("# Msg size (bytes)   Latency (seconds)\n");
		}
		first_receive= false;
	    }

	    if (my_rank != 0)   {
		// We always send back (to 0)
		data_send(0, len);

		if (cnt < 1)   {
		    if (len > 0)   {
			len= len + len_inc;
		    } else   {
			len= len_inc;
		    }
		    cnt= num_msg;
		    if (len > end_len)   {
			done= true;
		    }
		}

	    } else   {
		// I'm rank 0
		if (cnt > 0)   {
		    data_send(dest, len);
		} else   {
		    execution_time= (double)(getCurrentSimTime() - start_time) / 1000000000.0;
		    latency= execution_time / num_msg / 2.0;
		    printf("%9d %.9f\n", len, latency);
		    // Start next message size, if we haven't reached 0
		    if (len > 0)   {
			len= len + len_inc;
		    } else   {
			len= len_inc;
		    }
		    if (len > end_len)   {
			done= true;
		    } else   {
			cnt= num_msg;
			start_time= getCurrentSimTime();
			data_send(dest, len);
		    }
		}
	    }
	    break;
	default:
	    printf("[%3d] Didn't really expect that (%d)\n", my_rank, event);
    }

    if (done)   {
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
