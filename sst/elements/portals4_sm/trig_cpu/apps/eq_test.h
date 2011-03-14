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


#ifndef COMPONENTS_TRIG_CPU_EQ_TEST_H
#define COMPONENTS_TRIG_CPU_EQ_TEST_H

#include "sst/elements/portals4_sm/trig_cpu/application.h"
#include "sst/elements/portals4_sm/trig_cpu/trig_cpu.h"
#include "sst/elements/portals4_sm/trig_cpu/portals.h"

class eq_test :  public application {
public:
    eq_test(trig_cpu *cpu) : application(cpu), init(false)
    {
        ptl = cpu->getPortalsHandle();

        in_buf = my_id;
        out_buf = my_id;
    }

    bool
    operator()(Event *ev)
    {
        ptl_md_t md;
        ptl_me_t me;

        crBegin();

        if (!init) {
	    // Get the PT entries
	    ptl->PtlPTAlloc(0,PTL_EQ_NONE,0,&pte);
	    
            // setup md handles
            ptl->PtlCTAlloc(PTL_CT_OPERATION, ct_h);
            me.start = &in_buf;
            me.length = 8;
            me.ignore_bits = ~0x0;
            me.ct_handle = ct_h;
            ptl->PtlMEAppend(pte, me, PTL_PRIORITY_LIST, NULL, me_h);

            md.start = &out_buf;
            md.length = 8;
            md.eq_handle = PTL_EQ_NONE;
            md.ct_handle = ct_h;
            ptl->PtlMDBind(md, &md_h);

            init = true;
            crReturn();
	    start_noise_section();
        }

        // 200ns startup time
        start_time = cpu->getCurrentSimTimeNano();
        cpu->addBusyTime("200ns");
        crReturn();

	if ( my_id == 0 ) {
	    // Do a put to 1
	    ptl->PtlPut(md_h, 0, 8, 0, 1, pte, 0, 0, NULL, 0);
	    crReturn();
	    
	    // Now I simply wait to get something back from 1.
	    while (!ptl->PtlCTWait(ct_h, 1)) { crReturn(); }
	    printf("Done sending to 1\n");
	    
	    while (!ptl->PtlCTWait(ct_h, 2)) { crReturn(); }
	    printf("Received reponse from 1\n");

	    
	}
	else {
	    // Wait for a put from 0
	    while (!ptl->PtlCTWait(ct_h, 1)) { crReturn(); }
	    printf("Received put from 0\n");
	    
	    // Do a put to 1
	    ptl->PtlPut(md_h, 0, 8, 0, 0, pte, 0, 0, NULL, 0);
	    crReturn();

	    // Wait until I'm done sending
	    while (!ptl->PtlCTWait(ct_h, 2)) { crReturn(); }
	    printf("Done sending to 0\n");
	    
	}

	printf("%d: in_buf = %lu    out_buf = %lu\n",my_id,in_buf,out_buf);
        crFinish();
        return true;
	
    }

private:
    eq_test();
    eq_test(const application& a);
    void operator=(eq_test const&);

    portals *ptl;
    SimTime_t start_time;
    int i;
    bool init;
    int radix;

    int my_root;
    std::vector<int> my_children;
    int num_children;

    uint64_t in_buf, out_buf, tmp_buf, zero_buf;

    ptl_handle_ct_t ct_handle;

    ptl_handle_ct_t ct_h;
    ptl_handle_me_t me_h;
    ptl_handle_md_t md_h;

    ptl_pt_index_t pte;
    
    uint64_t algo_count;
};

#endif // COMPONENTS_TRIG_CPU_ALLREDUCE_TREE_TRIGGERED_H
