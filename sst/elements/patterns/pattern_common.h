/*
** Copyright 2009-2011 Sandia Corporation. Under the terms
** of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
** Government retains certain rights in this software.
**
** Copyright (c) 2009-2011, Sandia Corporation
** All rights reserved.
**
** This file is part of the SST software package. For license
** information, see the LICENSE file in the top level directory of the
** distribution.
*/

#ifndef _PATTERN_COMMON_H
#define _PATTERN_COMMON_H

#ifndef FALSE
#define FALSE (0)
#endif
#ifndef TRUE
#define TRUE (1)
#endif

#include <sst_config.h>
#include <sst/core/component.h>
#include <sst/core/sst_types.h>
#include <sst/core/link.h>


// Make sure those two correspond
#define TIME_BASE		"ns"
#define TIME_BASE_FACTOR	1000000000.0


// Storage for NIC parameters
typedef struct NICparams_t   {
    int index;
    int64_t inflectionpoint;
    int64_t latency;
} NICparams_t;


// Describe a far link
typedef struct FarLink_t   {
    // int src;   Implicit, it is my node
    int src_port;
    int dest;
    int dest_port;
} FarLink_t;

struct _FLcompare   {
    bool operator () (const FarLink_t& x, const FarLink_t& y) const {return x.dest < y.dest;} 
};



// Destination statistics
typedef struct stat_dest_t   {
    int dest;
    uint64_t cnt;
} stat_dest_t;

struct _dest_compare   {
    bool operator () (const stat_dest_t& x, const stat_dest_t& y) const {return x.dest < y.dest;} 
};



class Patterns   {
    public:
	Patterns()   {
	    my_net_link= NULL;
	    my_self_link= NULL;
	    my_NoC_link= NULL;
	    my_nvram_link= NULL;
	    my_storage_link= NULL;
	}

	~Patterns()   {
	    stat_print();
	}

	int init(SST::Component::Params_t& params, 
		SST::Link *net_link, SST::Link *self_link,
		SST::Link *NoC_link, SST::Link *nvram_link, SST::Link *storage_link);

	void self_event_send(int event, int32_t tag= 0, SST::SimTime_t delay= 0);
	void event_send(int dest, int event, SST::SimTime_t CurrentSimTime, int32_t tag= 0,
		uint32_t msg_len= 0, const char *payload= NULL, int payload_len= 0, int blocking= -1);
	void storage_write(int data_size, int return_event);
	void nvram_write(int data_size, int return_event);

	int get_total_cores(void)   {return total_cores;}
	int get_mesh_width(void)   {return mesh_width;}
	int get_mesh_height(void)   {return mesh_height;}
	int get_mesh_depth(void)   {return mesh_depth;}
	int get_NoC_width(void)   {return NoC_width;}
	int get_NoC_height(void)   {return NoC_height;}
	int get_NoC_depth(void)   {return NoC_depth;}
	int get_my_rank(void)   {return my_rank;}
	int get_cores_per_NoC_router(void)   {return cores_per_NoC_router;}
	int get_cores_per_Net_router(void)   {return cores_per_Net_router;}
	int get_num_router_nodes(void)   {return num_router_nodes;}
	int get_cores_per_node(void)   {return cores_per_node;}
	void record_Net_msg_stat(int hops, long long congestion_cnt,
		SST::SimTime_t congestion_delay, uint32_t msg_len);
	void record_NoC_msg_stat(int hops, long long congestion_cnt,
		SST::SimTime_t congestion_delay, uint32_t msg_len);


    private:
	void NoCsend(SST::CPUNicEvent *e, int my_rank, int dest_rank,
		SST::SimTime_t CurrentSimTime, int blocking, int tag);
	void Netsend(SST::CPUNicEvent *e, int my_node, int dest_node, int dest_rank,
		SST::SimTime_t CurrentSimTime, int blocking, int tag);
	void FarLinksend(SST::CPUNicEvent *e, int my_node, int dest_node, int dest_rank,
		SST::SimTime_t CurrentSimTime, int blocking, int tag);
	void send_msg_complete(SST::SimTime_t delay, int blocking, int tag);
	void stat_print();
	void get_NICparams(std::list<NICparams_t> params, int64_t msg_len, 
	    int64_t *latency, int64_t *msg_duration);

	SST::Link *my_net_link;
	SST::Link *my_self_link;
	SST::Link *my_NoC_link;
	SST::Link *my_nvram_link;
	SST::Link *my_storage_link;

	// Input parameters for the machine architecture
	int mesh_width;
	int mesh_height;
	int mesh_depth;
	int NoC_width;
	int NoC_height;
	int NoC_depth;
	int my_rank;
	int cores_per_NoC_router;
	int num_router_nodes;

	// Are any dimensions wrapped?
	int NetXwrap;
	int NetYwrap;
	int NetZwrap;
	int NoCXwrap;
	int NoCYwrap;
	int NoCZwrap;

	// Calculated values
	int total_cores;
	int cores_per_Net_router;
	int cores_per_node;

	// Input parameters for the NIC models
	SST::SimTime_t NetNICgap;
	SST::SimTime_t NoCNICgap;
	std::list<NICparams_t> NetNICparams;
	std::list<NICparams_t> NoCNICparams;
	std::set<FarLink_t, _FLcompare> FarLink;
	int FarLinkPortFieldWidth;
	int64_t NetLinkBandwidth;
	int64_t NetLinkLatency;
	int64_t NoCLinkBandwidth;
	int64_t NoCLinkLatency;
	int64_t IOLinkBandwidth;
	int64_t IOLinkLatency;
	int64_t NoCIntraLatency;
	int64_t NetIntraLatency;

	// Each message event gets a unique number for debugging
	uint64_t msg_seq;

	// NIC model: When can next message leave?
	SST::SimTime_t NextNoCNICslot;
	SST::SimTime_t NextNetNICslot;
	SST::SimTime_t NextFarNICslot;

	// Statistics
	long long int stat_NoCNICsend;
	long long int stat_NoCNICsend_bytes;
	long long int stat_NoCNICbusy;

	long long int stat_NetNICsend;
	long long int stat_NetNICsend_bytes;
	long long int stat_NetNICbusy;

	long long int stat_FarNICsend;
	long long int stat_FarNICsend_bytes;
	long long int stat_FarNICbusy;

	std::list<int> NICstat_ranks;
	std::set<stat_dest_t, _dest_compare> stat_dest;

	long long int stat_NetNICrecv;
	long long int stat_NetNICrecv_bytes;
	long long int stat_Netmsg_hops;
	long long int stat_Netmsg_congestion_cnt;
	SST::SimTime_t stat_Netmsg_congestion_delay;

	long long int stat_NoCNICrecv;
	long long int stat_NoCNICrecv_bytes;
	long long int stat_NoCmsg_hops;
	long long int stat_NoCmsg_congestion_cnt;
	SST::SimTime_t stat_NoCmsg_congestion_delay;


} ;  // end of class Patterns


#endif  /* _PATTERN_COMMON_H */
