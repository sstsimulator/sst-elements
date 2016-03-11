//
// Copyright (c) 2011, IBM Corporation
// All rights reserved.
//
// Rolf Riesen, October 2011
//

#include <sst_config.h>
#include <stdio.h>
#include <inttypes.h>		// For PRId64
#include <string>
#include "NIC_stats.h"



// Record the vitals of a send
void
NIC_stats::record_send(int dest, int32_t len)
{

stat_dest_t dl;
std::set<stat_dest_t>::iterator pos;


    sends++;
    send_bytes += len;

    if (_print_at_end)   {
	// Only record this if we are going to use it, since it takes
	// up space and time
	dl.dest= _m->destNode(dest);
	pos= destination_nodes.find(dl);
	if (pos != destination_nodes.end())   {
	    // Update. FIXME: This seems overly complicated...
	    // See also record() in msg_counter.cc with the same problem
	    dl.cnt= pos->cnt + 1;
	    destination_nodes.erase(dl);
	    destination_nodes.insert(dl);
	    pos= destination_nodes.find(dl);
	} else   {
	    // Never sent to this node before. Create an entry.
	    dl.dest= _m->destNode(dest);
	    dl.cnt= 0;
	    destination_nodes.insert(dl);
	}
    }

}  // end of record_send()



void
NIC_stats::record_busy(SST::SimTime_t congestion_delay)
{

    busy++;
    busy_delay += congestion_delay;

}  // end of record_busy()



void
NIC_stats::record_rcv(int hops, int64_t congestion_cnt,
	SST::SimTime_t congestion_delay, int32_t msg_len)
{

    recvs++;
    recv_bytes += msg_len;
    msg_hops += hops;
    msg_congestion_cnt += congestion_cnt;
    msg_congestion_delay += congestion_delay;

}  // End of record_rcv()



void
NIC_stats::stat_print(const char *type_name)
{

    printf("# [%3d] %s NIC model statistics\n", _my_rank, type_name);
    printf("# [%3d]     Total receives           %12" PRId64 "\n", _my_rank, recvs);
    printf("# [%3d]     Bytes received           %12" PRId64 "\n", _my_rank, recv_bytes);
    if (recvs > 0)   {
	printf("# [%3d]     Average hop count        %12.2f\n", _my_rank,
	    (float)msg_hops / (float)recvs);
    } else   {
	printf("# [%3d]     Average hop count        %12.2f\n", _my_rank, 0.0);
    }
    if (msg_congestion_cnt > 0)   {
	printf("# [%3d]     Congested %8" PRId64 " times, average delay %12" PRId64 " %s\n", _my_rank,
	    msg_congestion_cnt, msg_congestion_delay / msg_congestion_cnt, TIME_BASE);
    } else   {
	printf("# [%3d]     Congested %d times, average delay %12d %s\n",
	    _my_rank, 0, 0, TIME_BASE);
    }


    printf("# [%3d]     Total sends              %12" PRId64 "\n", _my_rank, sends);
    printf("# [%3d]     Bytes sent               %12" PRId64 "\n", _my_rank, send_bytes);
    if (sends > 0)   {
	printf("# [%3d]     NIC send busy            %12.1f %%\n", _my_rank,
	    (100.0 / sends) * busy);
    } else   {
	printf("# [%3d]     NIC send busy                     0.0 %%\n", _my_rank);
    }
    if (busy > 0)   {
	printf("# [%3d]     Busy      %8" PRId64 " times, average delay %12" PRId64 " %s\n", _my_rank,
	    busy, busy_delay / busy, TIME_BASE);
    } else   {
	printf("# [%3d]     Busy      %d times, average delay %12d %s\n",
	    _my_rank, 0, 0, TIME_BASE);
    }
    printf("#\n");


    printf("# [%3d]     Destination nodes:\n", _my_rank);
    printf("# [%3d]         ", _my_rank);
    std::set<stat_dest_t>::iterator node;
    for (node= destination_nodes.begin(); node != destination_nodes.end(); node++)   {
	printf("%d: %" PRIu64 ", ", node->dest, node->cnt);
    }
    printf("\n");
    printf("#\n");

}  // end of stat_print()



