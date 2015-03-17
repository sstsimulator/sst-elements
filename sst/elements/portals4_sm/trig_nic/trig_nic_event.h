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

#ifndef _TRIG_NIC_EVENT_H
#define _TRIG_NIC_EVENT_H

#include <sst/core/event.h>
#include "sst/elements/portals4_sm/trig_cpu/portals_types.h"

namespace SST {
namespace Portals4_sm {

    class trig_nic_event : public Event {
    public:
	trig_nic_event() : Event() {
	    portals = false;
	}

        ~trig_nic_event() {}

	int src;
        int dest;

        bool portals;
	bool head_packet;
	int stream;
        int latency; // Latency through NIC in ns
        int data_length;
        void *start;
      
	ptl_int_nic_op_type_t ptl_op;
	ptl_int_msg_info_t* msg_info;
	ptl_handle_eq_t eq_handle;
	
	//         ptl_int_nic_op_t* operation;
	union {
	    ptl_int_me_t* me;
	    ptl_int_trig_op_t* trig;
	    ptl_int_trig_op_t** trigV;
	    ptl_update_ct_event_t* ct;
	    ptl_handle_ct_t ct_handle;
	    ptl_int_dma_t* dma;
	    ptl_event_t* event;
	} data;

/*         uint32_t ptl_data[16]; */
        uint8_t ptl_data[64];

	void updateMsgHandle(uint16_t handle) {
	    if (!head_packet) return;
	    if ( ptl_op == PTL_NO_OP || ptl_op == PTL_DMA ) {
		// PIO or DMA from host.  Field is in ptl_data
		((ptl_header_t*)ptl_data)->out_msg_index = handle;
	    }
	}

    private:
	
        /*         BOOST_SERIALIZE { */
        /* 	  printf("Serializing MyMemEvent\n"); */
        /* 	  _AR_DBG(MemEvent,"\n"); */
        /*             BOOST_VOID_CAST_REGISTER( MyMemEvent*, Event* ); */
        /*             ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP( Event ); */
        /*             ar & BOOST_SERIALIZATION_NVP( address ); */
        /*             ar & BOOST_SERIALIZATION_NVP( type ); */
        /*             ar & BOOST_SERIALIZATION_NVP( tag ); */
        /*             _AR_DBG(MemEvent,"\n"); */
        /*         } */
    };


}
} //namespace SST

#endif
