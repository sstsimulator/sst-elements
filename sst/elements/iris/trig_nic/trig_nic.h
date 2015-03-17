// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef COMPONENTS_TRIG_NIC_H
#define COMPONENTS_TRIG_NIC_H

#include <queue>
#include <map>

#include <sst/core/params.h>
#include "../RtrIF.h"
#include "../SST_interface.h"

#include "../../portals4_sm/trig_cpu/portals_types.h"
#include "../../portals4_sm/trig_nic/trig_nic_event.h"

using namespace SST::Portals4_sm;

namespace SST {
namespace Iris {

struct MessageStream {
    void* start;
    int current_offset;
    int remaining_length;
    ptl_handle_ct_t ct_handle;
    ptl_size_t ct_increment;
    trig_nic_event* ack_msg;
    ptl_event_t* event;
    ptl_handle_eq_t eq_handle;
    int remaining_mlength;
};


/* this file is identical to the one inside portals4_sm trig nic
 * but is duplicated here to include the correct iris router events etc..
 */
class iris_trig_nic : public iris_RtrIF {

private:

    Link*                   cpu_link;
    // Direct link where we send packets destined for the router
    Link*                   self_link;
    // Indirect link where we send packets going to the portals
    // offload unit
    Link*                   ptl_link;
    Link*                   dma_link;

    int                     msg_latency;
    int                     ptl_latency;
    int                     ptl_msg_latency;
    int                     rtr_q_size;
    int                     rtr_q_max_size;

    int                     timing_set;
    int                     latency_ct_post;
    int                     latency_ct_host_update;
    int                     ptl_unit_latency;
    
    int                     additional_atomic_latency;

    bool currently_clocking;
    Clock::Handler<iris_trig_nic>* clock_handler_ptr;

    // Data structures to support portals on NIC
    #define MAX_PORTAL_TABLE_ENTRY 32
    ptl_entry_t* ptl_table[MAX_PORTAL_TABLE_ENTRY];

    #define MAX_CT_EVENTS 32
    ptl_int_ct_t ptl_ct_events[MAX_CT_EVENTS];

    #define MAX_PT_ENTRIES 16

    std::queue<ptl_int_trig_op_t*> already_triggered_q;

    // Two queues to handle events coming from the CPU.  On is for
    // PIO, the other for DMA.
    std::queue<trig_nic_event*> pio_q;
    std::queue<trig_nic_event*> dma_q;
    std::queue<ptl_int_dma_t*> dma_req_q;
    std::queue<trig_nic_event*> dma_hdr_q;

    typedef union {
	int64_t int_val;
	double dbl_val;
    } atomic_cache_entry;
    
    std::map<int,MessageStream*> streams;
    std::map<unsigned long,atomic_cache_entry*> atomic_cache;
    
    ptl_int_dma_t* dma_req;
    bool dma_in_progress;

    bool rr_dma;
    bool new_dma;
    bool send_recv;
    bool send_atomic_from_cache;

    std::map<uint16_t,ptl_int_msg_info_t*> out_msg_q;
    uint16_t next_out_msg_handle;

    // This is a bit goofy, but it handles the case of roll-over
    inline uint16_t get_next_out_msg_handle() {
/* 	if ( out_msg_q.size() == 0x10000 ) { */
/* 	    printf("All outstanding message buffers used, aborting...\n"); */
/* 	    abort(); */
/* 	} */
/* 	while ( out_msg_q.count(next_out_msg_handle) == 1 ) { */
/* 	    ++next_out_msg_handle; */
/* 	} */
	return next_out_msg_handle++;
    }
    
public:
    iris_trig_nic( ComponentId_t id, Params& params );

    virtual void finish() {
    }

    virtual void setup() {
    }

private:

    // Next event to go to the router
    irisRtrEvent* nextToRtr;

    bool clock_handler(Cycle_t cycle);
    void processCPUEvent( Event* e);
    void processPtlEvent( Event* e);
    void processDMAEvent( Event* e);

    void setTimingParams(int set);

    inline bool PtlCTCheckThresh(ptl_handle_ct_t ct_handle, ptl_size_t test) {
	// If threshold is 0, then automatically trigger
	if ( test == 0 ) return true;
	
	if ( (ptl_ct_events[ct_handle].ct_event.success +
	      ptl_ct_events[ct_handle].ct_event.failure ) >= test ) {
// 	    printf("%5d: PtlCTCheckThresh(): success = %d failure = %d ct_handle = %d\n",
// 		   m_id,ptl_ct_events[ct_handle].ct_event.success,
// 		   ptl_ct_events[ct_handle].ct_event.failure,ct_handle);
	    return true;
	}
	return false;
	
    }

    void scheduleUpdateHostCT(ptl_handle_ct_t ct_handle);
    void scheduleCTInc(ptl_handle_ct_t ct_handle, ptl_size_t increment, SimTime_t delay);
    void scheduleEQ(ptl_handle_eq_t eq_handle, ptl_event_t* ptl_event);

    double computeDoubleAtomic(unsigned long addr, double value, ptl_op_t op);
    int64_t computeIntAtomic(unsigned long addr, int64_t value, ptl_op_t op);

    ptl_int_me_t* match_header(me_list_t* list, ptl_header_t* header, ptl_size_t* offset, ptl_size_t* length);
    
};

}
}

#endif
