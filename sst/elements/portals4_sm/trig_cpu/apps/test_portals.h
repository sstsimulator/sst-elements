// Copyright 2009-2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef COMPONENTS_TRIG_CPU_TEST_PORTALS_H
#define COMPONENTS_TRIG_CPU_TEST_PORTALS_H

#include "sst/elements/portals4_sm/trig_cpu/application.h"
#include "sst/elements/portals4_sm/trig_cpu/trig_cpu.h"

#define BUF_SIZE 32

#define PT_BARRIER 0
#define PT_XFER    1

class test_portals :  public application {
public:
    test_portals(trig_cpu *cpu) : application(cpu)
    {
        ptl = cpu->getPortalsHandle();

	barrier_count = 0;

        msg_size = cpu->getMessageSize();
	if (msg_size < 8 ) {
	    printf("msg_size must be >= 8\n");
	    abort();
	}
	
	in_buf = (char*) malloc(msg_size);
	out_buf = (char*) malloc(msg_size);
	for ( int i = 0; i < msg_size; ++i ) {
	    out_buf[i] = i % 255;
	    in_buf[i] = 0;
	}
	if ( my_id == 1 ) {
	    overflow_buf1 = (char*)malloc(2*msg_size+4);
	    overflow_buf2 = (char*)malloc(2*msg_size+4);
	}

	peer_id = (my_id==0) ? 1 : 0;
    }

    bool
    operator()(Event *ev)
    {
	ptl_md_t md;
	ptl_me_t me;

	crInit();


	crFuncStart(barrier);
	{
	    barrier_count++;
	    ptl->PtlPut(barrier_md_h, 0, 0, PTL_NO_ACK_REQ, peer_id, barrier_pte, 0, 0, NULL, 0);
	    crReturn();
	    
	    while (!ptl->PtlCTWait(barrier_ct_h, barrier_count)) { crReturn(); }
	}
	crFuncEnd();


	// crFuncStart(fill_out_buf);
	// {
	//     for (int z = 0; z < msg_size; z++) {
		
	//     }
	// }
	// crFuncEnd();

	crFuncStart(test_put);
	{
	    // crFuncCall(barrier);
	    // if ( my_id == 0 ) {

		
	    // 	barrier();

	    // 	ptl->PtlPut(md,0,8096,PTL_ACK_REQ, peer_id, pte, 0, 0, NULL, 0);
		
	    // }
	    // else {
	    // }
	    
	}
	crFuncEnd();

	crStart();
	
	if ( !init ) {
	    // Set up barrier MDs and MEs
	    ptl->PtlPTAlloc(0,PTL_EQ_NONE,PT_BARRIER,&barrier_pte);
	    ptl->PtlCTAlloc(PTL_CT_OPERATION, barrier_ct_h);

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
	    }
	    else {
		// MEs will be set up by the tests
	    }
	    
	    init = true;
	}
	
	printf("%d, %lu\n",my_id,cpu->getCurrentSimTimeNano());
	crFuncCall(barrier);

	
	printf("%d, %lu\n",my_id,cpu->getCurrentSimTimeNano());
	crFuncCall(barrier);

	printf("%d, %lu\n",my_id,cpu->getCurrentSimTimeNano());

	crFinish();

	return true;
    }

private:
    test_portals();
    test_portals(const application& a);
    void operator=(test_portals const&);

    bool init;
    portals *ptl;

    SimTime_t start_time;
    int radix;
    int i;

    int msg_size;

    char *in_buf;
    char *out_buf;

    char *overflow_buf1;
    char *overflow_buf2;
    
    ptl_handle_ct_t ct_handle;

    int barrier_count;
    
    ptl_handle_ct_t ct_h, ct2_h, barrier_ct_h;
    ptl_handle_me_t in_me_h, out_me_h, atomic_me_h, trig_me_h, barrier_me_h;
    ptl_handle_md_t in_md_h, out_md_h, atomic_md_h, trig_md_h, barrier_md_h;;
    ptl_handle_eq_t eq_h;

    int64_t atomic_buf;
    
    ptl_pt_index_t barrier_pte, pte, pte2;
    ptl_event_t ptl_event;

    ptl_process_t peer_id;
};

#endif // COMPONENTS_TRIG_CPU_TEST_PORTALS_H
