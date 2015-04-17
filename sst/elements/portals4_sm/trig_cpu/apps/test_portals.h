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


#ifndef COMPONENTS_TRIG_CPU_TEST_PORTALS_H
#define COMPONENTS_TRIG_CPU_TEST_PORTALS_H

#include "sst/elements/portals4_sm/trig_cpu/application.h"
#include "sst/elements/portals4_sm/trig_cpu/trig_cpu.h"

#include <vector>
#include <string>

namespace SST {
namespace Portals4_sm {

#define PT_BARRIER 0
#define PT_XFER    1

#define BUF_SIZE 8192

class test_portals :  public application {
public:
    test_portals(trig_cpu *cpu) : application(cpu)
    {
        ptl = cpu->getPortalsHandle();
	
	barrier_count = 0;
	init = false;
	
        msg_size = cpu->getMessageSize();
	if (msg_size < 8 ) {
	    printf("msg_size must be >= 8\n");
	    abort();
	}
	
	in_buf = (unsigned char*) malloc(BUF_SIZE);
	out_buf = (unsigned char*) malloc(BUF_SIZE);
	for ( int i = 0; i < BUF_SIZE; ++i ) {
	    out_buf[i] = i % 255;
	    in_buf[i] = 0;
	}
	if ( my_id == 1 ) {
	    overflow_buf1 = (unsigned char*)malloc(2*BUF_SIZE+4);
	    overflow_buf2 = (unsigned char*)malloc(2*BUF_SIZE+4);
	}

	peer_id = (my_id==0) ? 1 : 0;

	test_desc.push_back("Single Packet");
	test_desc.push_back("Multi Packet PIO");
	test_desc.push_back("DMA");

	test_size.push_back(8);
	test_size.push_back(1024);
	test_size.push_back(8192);
    }

    bool
    operator()(Event *ev)
    {
	ptl_md_t md;
	ptl_me_t me;


	crInit();


	// Define barrier function.  This uses it's own portal table
	// entry, persistent ME and a counting event
	crFuncStart(barrier);
	{
	    barrier_count++;
	    ptl->PtlPut(barrier_md_h, 0, 0, PTL_NO_ACK_REQ, peer_id, barrier_pte, 0, 0, NULL, 0);
	    crReturn();
	    
	    while (!ptl->PtlCTWait(barrier_ct_h, barrier_count)) { crReturn(); }
	}
	crFuncEnd();

	/*******************************
         *       SIMPLE PUT TEST       *
         *******************************/
	crFuncStart(test_put_simple);
	{
	    if ( my_id == 0 ) {
		printf("\nSimple PUT Test\n");
	    }
	    else {
		reset_buffer(in_buf, BUF_SIZE);

		// Post a persistent ME for the tests
		me.start = in_buf;
		me.length = 8192;
		me.ignore_bits = ~0x0;
		me.options = 0;
		//me.options = PTL_ME_USE_ONCE;
		me.ct_handle = PTL_CT_NONE;
		me.min_free = 0;
		ptl->PtlMEAppend(pte, me, PTL_PRIORITY_LIST, NULL, in_me_h);
		crReturn();	
	    }
	    
	    crFuncCall(barrier);

	    for ( i = 0; (unsigned int)i < test_desc.size(); ++i ) {
		if ( my_id == 0 ) {
		    printf("  %s\n",test_desc[i].c_str());
		    crFuncCall(barrier);

		    ptl->PtlPut(out_md_h, 0, test_size[i], PTL_ACK_REQ, peer_id, pte,
				0, 0, &peer_id, test_size[i]/2);
		    crReturn();
		    
		    while (!ptl->PtlEQPoll(&return_value,eq_h,10000,&ptl_event)) { crReturn(); }
		    passed = true;
		    if ( return_value == PTL_OK ) {
			passed = check_event(&ptl_event, PTL_EVENT_ACK,
					     PTL_UNDEF, PTL_UNDEF,
					     PTL_UNDEF, PTL_UNDEF,
					     test_size[i], 0,
					     PTL_PTR_UNDEF, &peer_id,
					     PTL_UNDEF, PTL_OK);
			// ptl_op_t atomic_operation, ptl_datatype_t atomic_type) {
		    
		    }
		    else {
			passed = false;
		    }

		    if ( passed ) printf("    ACK Event:\t\t\tPASSED\n");
		    else printf("    ACK Event:\t\t\t\tFAILED\n");
		    
		}
		else {
		    crFuncCall(barrier);
		
		    while (!ptl->PtlEQPoll(&return_value,eq_h,10000,&ptl_event)) { crReturn(); }
		    passed = true;
		    if ( return_value == PTL_OK ) {
			passed = check_event(&ptl_event, PTL_EVENT_PUT,
					     peer_id, PT_XFER,
					     0, test_size[i],
					     test_size[i], 0,
					     in_buf, NULL,
					     test_size[i]/2, PTL_OK);
			// ptl_op_t atomic_operation, ptl_datatype_t atomic_type) {
			
		    }
		    else {
			passed = false;
		    }
		    
		    if ( passed ) printf("    PUT event\t\t\tPASSED\n");
		    else printf("    PUT event\t\t\t\tFAILED\n");
		    
		    passed = check_data(in_buf, BUF_SIZE, 0, 0, test_size[i]);
		    reset_buffer(in_buf, BUF_SIZE);
		    
		    if ( passed ) printf("    PUT data xfer\t\tPASSED\n");
		    else printf("    PUT data xfer\t\t\tFAILED\n");
		}
	    }

	    if ( my_id == 1 ) {
		ptl->PtlMEUnlink(in_me_h);
	    }
	}
	crFuncEnd();

	/*******************************
         *     SIMPLE ATOMIC TEST      *
         *******************************/
	crFuncStart(test_atomic_simple);
	{
	    if ( my_id == 0 ) {
		atomic_buf = 10;
		printf("\nSimple ATOMIC Test\n");
	    }
	    else {
		atomic_buf = 15;
		// Post a persistent ME for the test
		me.start = &atomic_buf;
		me.length = 8;
		me.ignore_bits = ~0x0;
		me.options = 0;
		//me.options = PTL_ME_USE_ONCE;
		me.ct_handle = atomic_ct_h;
		me.min_free = 0;
		ptl->PtlMEAppend(pte, me, PTL_PRIORITY_LIST, NULL, atomic_me_h);
		crReturn();
	    }
	    
	    crFuncCall(barrier);

	    if ( my_id == 0 ) {
		// printf("  Value\n",test_desc[i].c_str());
		crFuncCall(barrier);
		
		ptl->PtlAtomic(atomic_md_h, 0, 8, PTL_ACK_REQ, peer_id, pte,
			       0, 0, &peer_id, 155, PTL_SUM, PTL_LONG);
		crReturn();
		
		// while (!ptl->PtlEQPoll(&return_value,eq_h,10000,&ptl_event)) { crReturn(); }
		while (!ptl->PtlCTWait(atomic_ct_h,1)) { crReturn(); }		
		// passed = (atomic_buf == 25);
		// passed = true;
		// if ( return_value == PTL_OK ) {
		//     passed = check_event(&ptl_event, PTL_EVENT_ACK,
		// 			 PTL_UNDEF, PTL_UNDEF,
		// 			 PTL_UNDEF, PTL_UNDEF,
		// 			 test_size[i], 0,
		// 			 PTL_PTR_UNDEF, &peer_id,
		// 			 PTL_UNDEF, PTL_OK);
		//     // ptl_op_t atomic_operation, ptl_datatype_t atomic_type) {
		    
		// }
		// else {
		//     passed = false;
		// }

		// if ( passed ) printf("    Value:\t\t\tPASSED\n");
		// else printf("    Value:\t\t\t\tFAILED\n");
		
	    }
	    else {
		crFuncCall(barrier);
		
		while (!ptl->PtlCTWait(atomic_ct_h,1)) { crReturn(); }
		// while (!ptl->PtlEQPoll(&return_value,eq_h,10000,&ptl_event)) { crReturn(); }
		// passed = true;
		// if ( return_value == PTL_OK ) {
		//     passed = check_event(&ptl_event, PTL_EVENT_PUT,
		// 			 peer_id, PT_XFER,
		// 			 0, test_size[i],
		// 			 test_size[i], 0,
		// 			 in_buf, NULL,
		// 			 test_size[i]/2, PTL_OK);
		//     // ptl_op_t atomic_operation, ptl_datatype_t atomic_type) {
		    
		// }
		// else {
		//     passed = false;
		// }
		
		// if ( passed ) printf("    PUT event\t\t\tPASSED\n");
		// else printf("    PUT event\t\t\t\tFAILED\n");
		
		// passed = check_data(in_buf, BUF_SIZE, 0, 0, test_size[i]);
		// reset_buffer(in_buf, BUF_SIZE);
		
		passed = (atomic_buf == 25);
		if ( passed ) printf("    ATOMIC data xfer\t\tPASSED\n");
		else printf("    ATOMIC data xfer\t\t\tFAILED\n");
	    }

	    if ( my_id == 1 ) {
		ptl->PtlMEUnlink(atomic_me_h);
	    }
	}
	crFuncEnd();

	crStart();

	if ( !init ) {
	    // Set up barrier MDs and MEs
	    ptl->PtlPTAlloc(0,PTL_EQ_NONE,PT_BARRIER,&barrier_pte);
	    ptl->PtlCTAlloc(PTL_CT_OPERATION, barrier_ct_h);
	    ptl->PtlCTAlloc(PTL_CT_OPERATION, atomic_ct_h);

	    md.start = out_buf;
	    md.length = 0;
	    md.eq_handle = PTL_EQ_NONE;
	    md.ct_handle = PTL_CT_NONE;
	    ptl->PtlMDBind(md, &barrier_md_h);
	    crReturn();
	    
	    me.start = in_buf;
	    me.length = 0;
	    me.ignore_bits = ~0x0;
	    me.options = 0;
	    me.ct_handle = barrier_ct_h;
	    me.min_free = 0;
	    ptl->PtlMEAppend(barrier_pte, me, PTL_PRIORITY_LIST, NULL, barrier_me_h);
	    crReturn();

	    // Set up MD's on initiator side
	    if ( my_id == 0 ) {
		ptl->PtlEQAlloc(32,&eq_h);
		ptl->PtlPTAlloc(0,eq_h,PT_XFER,&pte);

		md.start = out_buf;
		md.length = 8096;
		md.eq_handle = eq_h;
		md.ct_handle = PTL_CT_NONE;
		ptl->PtlMDBind(md, &out_md_h);

		md.start = in_buf;
		md.length = 8096;
		md.eq_handle = eq_h;
		md.ct_handle = PTL_CT_NONE;
		ptl->PtlMDBind(md, &in_md_h);

		md.start = &atomic_buf;
		md.length = 8;
		md.eq_handle = PTL_EQ_NONE;
		md.ct_handle = atomic_ct_h;
		ptl->PtlMDBind(md, &atomic_md_h);
	    }
	    else {
		// MEs will be set up by the tests
		ptl->PtlEQAlloc(32,&eq_h);
		ptl->PtlPTAlloc(0,eq_h,PT_XFER,&pte);
	    }
	    
	    init = true;
	}
	
	crFuncCall(test_put_simple);
	crFuncCall(test_atomic_simple);

	crFinish();

	return true;
    }

private:
    test_portals();
    test_portals(const application& a);
    void operator=(test_portals const&);

    bool check_data(unsigned char* buf, int buf_len, int src_offset, int dest_offset, int length) {
	// Assume we start with a buffer full of zeros

	// Check the values before inserted data
	for ( int i = 0; i < dest_offset; i++ ) {
	    if ( buf[i] != 0 ) return false;
	}

	// Check the actual data segment
	for ( int i = 0; i < length; i++ ) {
	    if ( (i + dest_offset) >= buf_len ) break;
	    // if ( (buf[dest_offset+i] & 0xff) != ((src_offset+i) % 255) ) return false;
	    if ( buf[dest_offset+i] != ((src_offset+i) % 255) ) return false;
	}

	// Check the entries after the data segment (if there are any)
	for ( int i = dest_offset + length; i < buf_len; i++ ) {
	    if ( in_buf[i] != 0 ) return false;
	}

	return true;
    }

    void reset_buffer(unsigned char* buf, int length) {
	for ( int i = 0; i < length; i++ ) {
	    buf[i] = 0;
	}
    }
    
    bool check_event(ptl_event_t* event, ptl_event_kind_t type,
		     ptl_process_t initiator, ptl_pt_index_t pt_index,
		     // ptl_uid_t uid, ptl_jid_t jid,
		     ptl_match_bits_t match_bits, ptl_size_t rlength,
		     ptl_size_t mlength, ptl_size_t remote_offset,
		     void* start, void* user_ptr,
		     ptl_hdr_data_t hdr_data, ptl_ni_fail_t ni_fail_type) {
		     // ptl_op_t atomic_operation, ptl_datatype_t atomic_type) {
//	if ( event->type != type && type != PTL_UNDEF ) {  // NOTE: type != PTL_UNDEF is always true PTL_UNDEF defined to be ~0 which is -1. type is an enum that is 0 or more.
	if ( event->type != type && true ) {  
	    printf("type\n"); return false; }
	if ( event->initiator != initiator && ~initiator != 0 ) {
	    printf("initiator\n"); return false; }
	if ( event->pt_index != pt_index && pt_index != PTL_UNDEF ) {
	    printf("pt_index\n"); return false; }
	// if ( event->uid != uid ) return false;
	// if ( event->jid != jid ) return false;
	if ( event->match_bits != match_bits && ~match_bits != 0 ) {
	    printf("match_bits\n"); return false; }
	if ( event->rlength != rlength && ~rlength != 0 ) {
	    printf("rlength\n"); return false; }
	if ( event->mlength != mlength && ~mlength != 0 ) {
	    printf("mlength\n"); return false; }
	if ( event->remote_offset != remote_offset && ~remote_offset != 0 )
	    {printf("remote_offset"); return false; }
	if ( event->start != start && start != PTL_PTR_UNDEF ) {
	    printf("start\n"); return false; }
	if ( event->user_ptr != user_ptr && user_ptr != PTL_PTR_UNDEF ) {
	    printf("user_ptr\n"); return false; }
	if ( event->hdr_data != hdr_data && ~hdr_data != 0 ) {
	    printf("hdr_data\n"); return false; }
	if ( event->ni_fail_type != ni_fail_type ) {printf("ni_fail_type\n"); return false;}
	// if ( event->atomic_operation != atomic_operation ) return false;
	// if ( event->atomic_type != atomic_type ) return false;
	return true;
    }


    bool init;
    bool passed;
    portals *ptl;

    SimTime_t start_time;
    int radix;
    int i;
    int return_value;
    
    int msg_size;

    unsigned char *in_buf;
    unsigned char *out_buf;

    unsigned char *overflow_buf1;
    unsigned char *overflow_buf2;
    
    ptl_handle_ct_t ct_handle;

    int barrier_count;
    
    ptl_handle_ct_t ct_h, ct2_h, barrier_ct_h, atomic_ct_h;
    ptl_handle_me_t in_me_h, out_me_h, atomic_me_h, trig_me_h, barrier_me_h;
    ptl_handle_md_t in_md_h, out_md_h, atomic_md_h, trig_md_h, barrier_md_h;
    ptl_handle_eq_t eq_h;

    int64_t atomic_buf;
    
    ptl_pt_index_t barrier_pte, pte, pte2;
    ptl_event_t ptl_event;

    ptl_process_t peer_id;

    std::vector<std::string> test_desc;
    std::vector<int> test_size;

};

}
}

#endif // COMPONENTS_TRIG_CPU_TEST_PORTALS_H
