//
// Copyright (c) 2011, IBM Corporation
// All rights reserved.
//
// Rolf Riesen, October 2011
//

#ifndef _NIC_STATS_H_
#define _NIC_STATS_H_

#include "patterns.h"
#include <sst/core/sst_types.h>
#include "machine_info.h"


// Destination statistics
typedef struct   {
    int dest;
    uint64_t cnt;

} stat_dest_t;

struct _dest_compare   {
    bool operator () (const stat_dest_t& x, const stat_dest_t& y) const {return x.dest < y.dest;} 
};



class NIC_stats   {
    public:
	NIC_stats(int my_rank, const char *nic_name, bool do_print_stats, MachineInfo *machine) :
	    _my_rank(my_rank),
	    _nic_name(nic_name),
	    _print_at_end(do_print_stats),
	    _m(machine)
	{
	    sends= 0;
	    send_bytes= 0;
	    busy= 0;
	    busy_delay= 0;

	    recvs= 0;
	    recv_bytes= 0;
	    msg_hops= 0;
	    msg_congestion_cnt= 0;
	    msg_congestion_delay= 0;
	}

	~NIC_stats()   {
	    if (_print_at_end)   {
		stat_print(_nic_name);
	    }
	}

	void record_send(int dest, int32_t len);
	void record_busy(SST::SimTime_t congestion_delay);
	void record_rcv(int hops, int64_t congestion_cnt,
		SST::SimTime_t congestion_delay, int32_t msg_len);

    private:

	void stat_print(const char *type_name);

	int _my_rank;
	const char *_nic_name;
	bool _print_at_end;
	MachineInfo *_m;

	int64_t sends;
	int64_t send_bytes;
	int64_t busy;
	SST::SimTime_t busy_delay;

	int64_t recvs;
	int64_t recv_bytes;
	int64_t msg_hops;
	int64_t msg_congestion_cnt;
	SST::SimTime_t msg_congestion_delay;
	std::set<stat_dest_t, _dest_compare> destination_nodes;


} ;  // end of class NIC_stats


#endif  // _NIC_STATS_H_
