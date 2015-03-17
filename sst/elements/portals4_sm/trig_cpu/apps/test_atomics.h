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


#ifndef COMPONENTS_TRIG_CPU_TEST_ATOMICS_H
#define COMPONENTS_TRIG_CPU_TEST_ATOMICS_H

#include "sst/elements/portals4_sm/trig_cpu/application.h"
#include "sst/elements/portals4_sm/trig_cpu/trig_cpu.h"
#include "sst/elements/portals4_sm/trig_cpu/portals.h"

namespace SST {
namespace Portals4_sm {

class test_atomics :  public application {
public:
    test_atomics(trig_cpu *cpu) : application(cpu), init(false)
    {
        ptl = cpu->getPortalsHandle();

    }

    bool
    operator()(Event *ev)
    {
        ptl_md_t md;
        ptl_me_t me;

        crBegin();

	// setup md handles
	ptl->PtlCTAlloc(PTL_CT_OPERATION, ct_h);
	crReturn();
	
	accum = my_id;
	me.start = &accum;
	me.length = 8;
	me.match_bits = 0;
	me.ignore_bits = 0;
	me.ct_handle = ct_h;
	ptl->PtlMEAppend(1, me, PTL_PRIORITY_LIST, NULL, up_tree_me_h);
	crReturn();

	md.start = &accum;
	md.length = 8;
	md.eq_handle = PTL_EQ_NONE;
	md.ct_handle = PTL_EQ_NONE;
	ptl->PtlMDBind(md, &my_md_h);
	crReturn();
	

        // 200ns startup time
        start_time = cpu->getCurrentSimTimeNano();
        cpu->addBusyTime("200ns");
        crReturn();

	ptl->PtlTriggeredAtomic(my_md_h, 0, 8, 0, (my_id+2) % num_nodes, 1, 0, 0, NULL, 0, PTL_SUM, PTL_INT, ct_h, 1);
	crReturn();

	ptl->PtlAtomic(my_md_h, 0, 8, 0, (my_id+1) % num_nodes, 1, 0, 0, NULL, 0, PTL_SUM, PTL_INT);
	crReturn();
	
	
	while (!ptl->PtlCTWait(ct_h, 2)) { crReturn(); }
	crReturn();

	printf("%d: %lld\n",my_id,(long long int)accum);


        trig_cpu::addTimeToStats(cpu->getCurrentSimTimeNano()-start_time);

        crFinish();
        return true;
    }

private:
    test_atomics();
    test_atomics(const application& a);
    void operator=(test_atomics const&);

    portals *ptl;

    int64_t accum;
    
    SimTime_t start_time;
    int radix;
    bool init;

    ptl_handle_ct_t ct_h;
    ptl_handle_me_t up_tree_me_h;

    ptl_handle_ct_t down_tree_ct_h;
    ptl_handle_me_t down_tree_me_h;

    ptl_handle_md_t my_md_h;

    int i;
    int my_root;
    std::vector<int> my_children;
    int num_children;

    static const int PT_UP = 0;
    static const int PT_DOWN = 1;
};

}
}
#endif // COMPONENTS_TRIG_CPU_TEST_ATOMICS_H
