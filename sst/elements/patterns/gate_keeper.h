//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _GATE_KEEPER_H
#define _GATE_KEEPER_H

#include <sst/core/params.h>
#include <sst/core/event.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include "pattern_common.h"



class Gate_keeper {
    public:
        Gate_keeper()   {
	    // Defaults for paramters
	    gate_keeper_debug= 0;
	    my_rank= -1;
	    net_latency= 0;
	    net_bandwidth= 0;
	    node_latency= 0;
	    node_bandwidth= 0;
	    envelope_size= 64;
	    cores= -1;
	}

	void init(SST::ComponentId_t id, SST::Params& params);

	int my_rank;

    private:

	void handle_events(SST::CPUNicEvent *e);
	void handle_net_events(SST::Event *sst_event);
	void handle_NoC_events(SST::Event *sst_event);
	void handle_self_events(SST::Event *sst_event);
	void handle_nvram_events(SST::Event *sst_event);
	void handle_storage_events(SST::Event *sst_event);
	Patterns *common;

	// Input parameters for simulation
	int cores;
	int num_msgs;
	int nranks;
	int x_dim;
	int y_dim;
	int NoC_x_dim;
	int NoC_y_dim;
	SST::SimTime_t net_latency;
	SST::SimTime_t net_bandwidth;
	SST::SimTime_t node_latency;
	SST::SimTime_t node_bandwidth;
	int exchange_msg_len;
	int envelope_size;
	int gate_keeper_debug;

	// Our connections to SST
	SST::Link *net;
	SST::Link *self_link;
	SST::Link *NoC;
	SST::Link *nvram;
	SST::Link *storage;
	SST::TimeConverter *tc;
};

#endif // _GATE_KEEPER_H
