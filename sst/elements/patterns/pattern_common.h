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
#define TIME_BASE		"1ns"
#define TIME_BASE_FACTOR	1000000000.0



class Patterns   {
    public:
	Patterns()   {
	    // Nothing to do for now
	    my_net_link= NULL;
	    my_self_link= NULL;
	    mesh_width= -1;
	    mesh_height= -1;
	    NoC_width= -1;
	    NoC_height= -1;
	    total_cores= 1;
	    my_rank= -1;
	    msg_seq= 1;
	}

	int init(SST::Component::Params_t& params, 
		SST::Link *net_link, SST::Link *self_link,
		SST::Link *NoC_link, SST::Link *nvram_link, SST::Link *storage_link);

	void event_send(int dest, int event, int32_t tag= 0, uint32_t msg_len= 0,
		const char *payload= NULL, int payload_len= 0);
	void storage_write(int data_size, int return_event);
	void nvram_write(int data_size, int return_event);

	int get_total_cores(void)   {return total_cores;}
	int get_mesh_width(void)   {return mesh_width;}
	int get_mesh_height(void)   {return mesh_height;}
	int get_NoC_width(void)   {return NoC_width;}
	int get_NoC_height(void)   {return NoC_height;}
	int get_my_rank(void)   {return my_rank;}
	int get_cores_per_NoC_router(void)   {return cores_per_NoC_router;}
	int get_cores_per_Net_router(void)   {return cores_per_Net_router;}
	int get_num_router_nodes(void)   {return num_router_nodes;}
	int get_cores_per_node(void)   {return cores_per_node;}


    private:
	void NoCsend(SST::CPUNicEvent *e, int my_rank, int dest_rank);
	void Netsend(SST::CPUNicEvent *e, int my_node, int dest_node, int dest_rank);

	SST::Link *my_net_link;
	SST::Link *my_self_link;
	SST::Link *my_NoC_link;
	SST::Link *my_nvram_link;
	SST::Link *my_storage_link;

	int envelope_size;
	int mesh_width;
	int mesh_height;
	int NoC_width;
	int NoC_height;
	int my_rank;
	int total_cores;
	int cores_per_NoC_router;
	int cores_per_Net_router;
	int cores_per_node;
	int num_router_nodes;
	uint64_t msg_seq;		// Each message event gets a unique number for debugging

} ;  // end of class Patterns


#endif  /* _PATTERN_COMMON_H */
