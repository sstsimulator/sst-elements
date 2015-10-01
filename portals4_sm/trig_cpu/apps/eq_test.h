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


#ifndef COMPONENTS_TRIG_CPU_EQ_TEST_H
#define COMPONENTS_TRIG_CPU_EQ_TEST_H

#include "sst/elements/portals4_sm/trig_cpu/application.h"
#include "sst/elements/portals4_sm/trig_cpu/trig_cpu.h"
#include "sst/elements/portals4_sm/trig_cpu/portals.h"

namespace SST {
namespace Portals4_sm {

class eq_test :  public application {
public:
    eq_test(trig_cpu *cpu) : application(cpu), init(false)
    {
        ptl = cpu->getPortalsHandle();

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
    }

    bool
    operator()(Event *ev)
    {
        ptl_md_t md;
        ptl_me_t me;

        crBegin();

        if (!init) {
	    // Get the PT entries
	    ptl->PtlEQAlloc(1,&eq_h);
	    
	    ptl->PtlPTAlloc(0,eq_h,0,&pte);
	    ptl->PtlPTAlloc(0,PTL_EQ_NONE,1,&pte2);
	    
            // setup md handles
            ptl->PtlCTAlloc(PTL_CT_OPERATION, ct_h);
            ptl->PtlCTAlloc(PTL_CT_OPERATION, ct2_h);

	    // Rank 0 will be initiator
	    if ( my_id == 0 ) {
		md.start = out_buf;
		md.length = msg_size;
		md.eq_handle = eq_h;
		md.ct_handle = ct_h;
		ptl->PtlMDBind(md, &out_md_h);

#if 0
		md.start = out_buf;
		md.length = msg_size;
		md.eq_handle = PTL_EQ_NONE;
		md.ct_handle = PTL_CT_NONE;
		ptl->PtlMDBind(md, &trig_md_h);

		md.start = in_buf;
		md.length = msg_size;
		md.eq_handle = eq_h;
		md.ct_handle = ct_h;
		ptl->PtlMDBind(md, &in_md_h);

// 		atomic_buf = 10;
		
		md.start = &atomic_buf;
		md.length = 8;
		md.eq_handle = PTL_EQ_NONE;
		md.ct_handle = ct_h;
		ptl->PtlMDBind(md, &atomic_md_h);

		// ME to be used to test the triggered ops
		me.start = in_buf;
		me.length = msg_size;
		me.ignore_bits = ~0x0;
		me.options = 0;
		me.ct_handle = ct2_h;
		me.min_free = 0;
		ptl->PtlMEAppend(pte2, me, PTL_PRIORITY_LIST, NULL, trig_me_h);
#endif
	    }
	    else {
		// For put
		me.start = in_buf;
		me.length = msg_size;
		me.ignore_bits = ~0x0;
		me.options = PTL_ME_USE_ONCE | PTL_ME_EVENT_CT_BYTES;
		me.ct_handle = ct_h;
		me.min_free = 0;
		ptl->PtlMEAppend(pte, me, PTL_PRIORITY_LIST, NULL, in_me_h);

		// Overflow MEs
		me.start = overflow_buf1;
		me.length = 2*msg_size+4;
		me.ignore_bits = ~0x0;
		me.options = PTL_ME_MANAGE_LOCAL;
		me.ct_handle = PTL_CT_NONE;
		me.min_free = msg_size;
		ptl->PtlMEAppend(pte, me, PTL_OVERFLOW, NULL, out_me_h);		


#if 0
		// For get
		me.start = out_buf;
		me.length = msg_size;
// 		me.ignore_bits = ~0x0;
		me.ignore_bits = 0;
		me.match_bits = 1;
		me.options = PTL_ME_USE_ONCE;
		me.ct_handle = ct_h;
		me.min_free = 0;
		ptl->PtlMEAppend(pte, me, PTL_PRIORITY_LIST, NULL, out_me_h);

		// For atomic
		me.start = &atomic_buf;
		me.length = 8;
		me.ignore_bits = ~0x0;
		me.options = PTL_ME_USE_ONCE;
		me.ct_handle = ct_h;
		me.min_free = 0;
		ptl->PtlMEAppend(pte, me, PTL_PRIORITY_LIST, NULL, atomic_me_h);

		// For triggered put, but we'll make it short to check truncation
		me.start = in_buf;
		me.length = msg_size/2;
		me.ignore_bits = ~0x0;
		me.options = PTL_ME_USE_ONCE;
		me.ct_handle = ct_h;
		me.min_free = 0;
		ptl->PtlMEAppend(pte, me, PTL_PRIORITY_LIST, NULL, in_me_h);

// 		// For triggered get
// 		me.start = out_buf;
// 		me.length = msg_size;
// 		me.ignore_bits = ~0x0;
// 		me.options = PTL_ME_USE_ONCE;
// 		me.ct_handle = ct_h;
// 		me.min_free = 0;
// 		ptl->PtlMEAppend(pte, me, PTL_PRIORITY_LIST, NULL, out_me_h);

		md.start = out_buf;
		md.length = msg_size;
		md.eq_handle = PTL_EQ_NONE;
		md.ct_handle = PTL_CT_NONE;
		ptl->PtlMDBind(md, &out_md_h);

// 		// Overflow MEs
// 		me.start = overflow_buf1;
// 		me.length = 2*msg_size+4;
// 		me.ignore_bits = ~0x0;
// 		me.options = PTL_ME_MANAGE_LOCAL;
// 		me.ct_handle = PTL_CT_NONE;
// 		me.min_free = msg_size;
// 		ptl->PtlMEAppend(pte, me, PTL_OVERFLOW, NULL, out_me_h);		

// 		me.start = overflow_buf2;
// 		me.length = 2*msg_size+4;
// 		me.ignore_bits = ~0x0;
// 		me.options = PTL_ME_MANAGE_LOCAL;
// 		me.ct_handle = PTL_CT_NONE;
// 		me.min_free = msg_size;
// 		ptl->PtlMEAppend(pte, me, PTL_OVERFLOW, NULL, out_me_h);		

#endif
	    }
	    
            init = true;
            crReturn();
//          start_noise_section();
        }

        // 200ns startup time
        start_time = cpu->getCurrentSimTimeNano();
        cpu->addBusyTime("200ns");
        crReturn();

	int bad;
	if ( my_id == 0 ) {
	    // Do a put to 1
	    ptl->PtlPut(out_md_h, 0, msg_size, PTL_ACK_REQ, 1, pte, 1, 0, NULL, 0);
	    crReturn();
	    
	    // Now I simply wait for the send event to increment.
	    while (!ptl->PtlCTWait(ct_h, 1)) { crReturn(); }

	    // Wait for the put to complete
	    while (!ptl->PtlEQWait(eq_h, &ptl_event)) { crReturn(); }
	    printf("Event on 0:\n");
	    ptl_event.print();

	    // Do another put, this one will be unexpected
	    ptl->PtlPut(out_md_h, 0, msg_size, PTL_ACK_REQ, 1, pte, 1, 0, NULL, 0);
	    crReturn();
	    
	    // Now I simply wait for the send event to increment.
	    while (!ptl->PtlCTWait(ct_h, 1)) { crReturn(); }

	    // Wait for the put to complete
	    while (!ptl->PtlEQWait(eq_h, &ptl_event)) { crReturn(); }
	    printf("Event on 0:\n");
	    ptl_event.print();

#if 0
	    // Now do a get
	    ptl->PtlGet(in_md_h, 0, msg_size, 1, pte, 1, NULL, 0);
	    crReturn();

// 	    // Wait for the get to complete
// 	    while (!ptl->PtlCTWait(ct_h, 2)) { crReturn(); }

	    // Wait for the get to complete
	    while (!ptl->PtlEQWait(eq_h, &ptl_event)) { crReturn(); }
	    printf("Event on 0:\n");
	    ptl_event.print();
	    
	    // Check results so far
	    bad = 0;
	    for (i = 0 ; i < msg_size ; ++i) {
		if ((in_buf[i] & 0xff) != i % 255) bad++;
	    }
	    if (bad) printf("**** %5d: bad results: %d\n",my_id,bad);

	    // Now test an atomic operation
	    ptl->PtlAtomic(atomic_md_h, 0, 8, 0, 1, pte, 0, 0, NULL, 0, PTL_SUM, PTL_LONG);
	    
	    // Wait for the atomic to complete
	    while (!ptl->PtlCTWait(ct_h, 3)) { crReturn(); }

	    
	    // Now test some triggered ops
	    // First clear in_buf
	    for (i = 0; i < msg_size; ++i ) {
		in_buf[i] = 0;
	    }

	    // Post triggereds.  We'll wait for both a notification
	    // locally and from the other rank.
	    ptl->PtlTriggeredPut(out_md_h, 0, msg_size, 0, 1, pte, 0, 0, NULL, 0, ct2_h, 2);
	    crReturn();
	    
// 	    ptl->PtlTriggeredGet(in_md_h, 0, msg_size, 1, pte, 0, NULL, 0, ct2_h, 4);
// 	    crReturn();
	    
	    // Now put to myself to trigger put
	    ptl->PtlPut(trig_md_h, 0, 0, 0, 0, pte2, 0, 0, NULL, 0);
	    crReturn();

// 	    while (!ptl->PtlCTWait(ct_h, 5)) { crReturn(); }
	    // Wait for the get to complete
	    while (!ptl->PtlEQWait(eq_h, &ptl_event)) { crReturn(); }
	    printf("Event on 0:\n");
	    ptl_event.print();

// 	    // Now put to myself to trigger get
// 	    ptl->PtlPut(trig_md_h, 0, 0, 0, 0, pte2, 0, 0, NULL, 0);
// 	    crReturn();

// // 	    while (!ptl->PtlCTWait(ct_h, 7)) { crReturn(); }
// 	    // Wait for the get to complete
// 	    while (!ptl->PtlEQWait(eq_h, &ptl_event)) { crReturn(); }
// 	    printf("Event on 0:\n");
// 	    ptl_event.print();

// 	    // Check results so far
// 	    bad = 0;
// 	    for (i = 0 ; i < msg_size ; ++i) {
// 		if ((in_buf[i] & 0xff) != i % 255) bad++;
// 	    }
// 	    if (bad) printf("%5d: bad results: %d\n",my_id,bad);

// 	    // Do a series of puts that will hit the overflow list and
// 	    // see if auto_unlink works
// 	    ptl->PtlPut(out_md_h, 0, msg_size, 0, 1, pte, 0, 0, NULL, 0);
// 	    crReturn();

// 	    ptl->PtlPut(out_md_h, 0, msg_size, 0, 1, pte, 0, 0, NULL, 0);
// 	    crReturn();

// 	    ptl->PtlPut(out_md_h, 0, msg_size, 0, 1, pte, 0, 0, NULL, 0);
// 	    crReturn();

	    // Ignore all the events for now
#endif	    
	}
	else {
	    // Wait for a put from 0
	    while (!ptl->PtlCTWait(ct_h, msg_size)) { crReturn(); }
	    printf("Received put from 0\n");

	    // Check event queue for PUT event
	    while (!ptl->PtlEQWait(eq_h, &ptl_event)) { crReturn(); }
	    printf("Event (PUT) on 1:\n");
	    ptl_event.print();
	    
	    bad = 0;
	    for (i = 0 ; i < msg_size ; ++i) {
		if ((in_buf[i] & 0xff) != i % 255) bad++;
	    }
	    if (bad) printf("%5d: bad results: %d\n",my_id,bad);

	    cpu->addBusyTime("10us");
	    crReturn();
	    
	    // For unexpected put
	    me.start = in_buf;
	    me.length = msg_size;
	    me.ignore_bits = ~0x0;
	    me.options = PTL_ME_USE_ONCE | PTL_ME_EVENT_CT_BYTES;
	    me.ct_handle = ct_h;
	    me.min_free = 0;
	    ptl->PtlMEAppend(pte, me, PTL_PRIORITY_LIST, NULL, in_me_h);
	    crReturn();

	    // Check event queue for PUT event
	    while (!ptl->PtlEQWait(eq_h, &ptl_event)) { crReturn(); }
	    printf("Event (PUT_OVERFLOW) on 1:\n");
	    ptl_event.print();

	    cpu->addBusyTime("10us");
	    crReturn();

	    ptl_ct_event_t ct;
	    ptl->PtlCTGet(ct_h,&ct);

	    printf("%d\n",ct.success);
	    
	    // Wait for a put from 0
	    while (!ptl->PtlCTWait(ct_h, 2*msg_size)) { crReturn(); }
	    printf("Received put from 0\n");

	    

#if 0
// 	    // Wait for get to complete 
// 	    while (!ptl->PtlCTWait(ct_h, 2)) { crReturn(); }

	    // Check event queue for GET event
	    while (!ptl->PtlEQWait(eq_h, &ptl_event)) { crReturn(); }
	    printf("Event (GET) on 1:\n");
	    ptl_event.print();
	    
	    // Wait for atomic to complete 
	    while (!ptl->PtlCTWait(ct_h, 3)) { crReturn(); }
 	    if (atomic_buf != 10) printf("Bad atomic result: %ld\n", (long) atomic_buf);

	    // Clear in_buf
	    for (i = 0; i < msg_size; ++i ) {
		in_buf[i] = 0;
	    }

	    // Now put to 0 to trigger put
	    ptl->PtlPut(out_md_h, 0, 0, 0, 0, pte2, 0, 0, NULL, 0);
	    crReturn();

	    // Wait for triggered put to complete, then check to see
	    // if is is correct.
// 	    while (!ptl->PtlCTWait(ct_h, 4)) { crReturn(); }
	    while (!ptl->PtlEQWait(eq_h, &ptl_event)) { crReturn(); }
	    printf("Event (PUT) on 1:\n");
	    ptl_event.print();
	    bad = 0;
	    // We only reported half the length to the ME, so we
	    // should truncate.
	    for (i = 0 ; i < msg_size/2 ; ++i) {
		if ((in_buf[i] & 0xff) != i % 255) bad++;
	    }
	    // From here on should be zeros
	    for (i = msg_size/2 ; i < msg_size ; ++i) {
		if ((in_buf[i] & 0xff) != 0) bad++;
	    }
	    if (bad) printf("%5d: bad results: %d\n",my_id,bad);
	    
// 	    // Now put to 0 to trigger get
// 	    ptl->PtlPut(out_md_h, 0, 0, 0, 0, pte2, 0, 0, NULL, 0);
// 	    crReturn();
	    
// 	    // Wait for triggered get to complete
// // 	    while (!ptl->PtlCTWait(ct_h, 5)) { crReturn(); }
// 	    while (!ptl->PtlEQWait(eq_h, &ptl_event)) { crReturn(); }
// 	    printf("Event on 1:\n");
// 	    ptl_event.print();

// 	    // Puts into overflow don't create events.  Should get one
// 	    // AUTO_UNLINK event
// 	    while (!ptl->PtlEQWait(eq_h, &ptl_event)) { crReturn(); }
// 	    printf("Event on 1:\n");
// 	    ptl_event.print();

// 	    // There should be data in overflow, check the PUT_OVERFLOW event
// 	    me.start = in_buf;
// 	    me.length = msg_size;
// 	    me.ignore_bits = ~0x0;
// 	    me.options = PTL_ME_USE_ONCE;
// 	    me.ct_handle = ct_h;
// 	    me.min_free = 0;
// 	    ptl->PtlMEAppend(pte, me, PTL_PRIORITY_LIST, NULL, in_me_h);

// 	    while (!ptl->PtlEQWait(eq_h, &ptl_event)) { crReturn(); }
// 	    printf("Event on 1:\n");
// 	    ptl_event.print();
	    
// 	    // And again.  There should be data in overflow, check the PUT_OVERFLOW event
// 	    me.start = in_buf;
// 	    me.length = msg_size;
// 	    me.ignore_bits = ~0x0;
// 	    me.options = PTL_ME_USE_ONCE;
// 	    me.ct_handle = ct_h;
// 	    me.min_free = 0;
// 	    ptl->PtlMEAppend(pte, me, PTL_PRIORITY_LIST, NULL, in_me_h);

// 	    while (!ptl->PtlEQWait(eq_h, &ptl_event)) { crReturn(); }
// 	    printf("Event on 1:\n");
// 	    ptl_event.print();

// 	    // See if we get an AUTO_FREE
// 	    while (!ptl->PtlEQWait(eq_h, &ptl_event)) { crReturn(); }
// 	    printf("Event on 1:\n");
// 	    ptl_event.print();
#endif	    
	}

        trig_cpu::addTimeToStats(cpu->getCurrentSimTimeNano()-start_time);
// 	printf("Checking results\n");
	
        crFinish();
        return true;
    }

private:
    eq_test();
    eq_test(const application& a);
    void operator=(eq_test const&);

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

    ptl_handle_ct_t ct_h, ct2_h;
    ptl_handle_me_t in_me_h, out_me_h, atomic_me_h, trig_me_h;
    ptl_handle_md_t in_md_h, out_md_h, atomic_md_h, trig_md_h;
    ptl_handle_eq_t eq_h;

    int64_t atomic_buf;
    
    ptl_pt_index_t pte, pte2;
    ptl_event_t ptl_event;
    
};

}
}
#endif // COMPONENTS_TRIG_CPU_ALLREDUCE_TREE_TRIGGERED_H
