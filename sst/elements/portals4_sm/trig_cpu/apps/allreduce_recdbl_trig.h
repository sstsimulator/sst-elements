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


#ifndef COMPONENTS_TRIG_CPU_ALLREDUCE_RECDBL_TRIGGERED_H
#define COMPONENTS_TRIG_CPU_ALLREDUCE_RECDBL_TRIGGERED_H

#include "sst/elements/portals4_sm/trig_cpu/application.h"
#include "sst/elements/portals4_sm/trig_cpu/trig_cpu.h"
#include "sst/elements/portals4_sm/trig_cpu/portals.h"

namespace SST {
namespace Portals4_sm {

class allreduce_recdbl_triggered :  public application {
public:
    allreduce_recdbl_triggered(trig_cpu *cpu) : application(cpu), init(false), algo_count(0)
    {
        ptl = cpu->getPortalsHandle();

        in_buf = 1;
        out_buf = 0;
        zero_buf = 0;
    }

    bool
    operator()(Event *ev)
    {
        ptl_md_t md;
        ptl_me_t me;

        crBegin();

        if (!init) {
            my_levels = -1;
            for (adj = 0x1; adj <= num_nodes ; adj  <<= 1) { my_levels++; } adj = adj >> 1;
            if (adj != num_nodes) {
                printf("recursive_doubling requires power of 2 nodes (%d)\n",
                       num_nodes);
                exit(1);
            }

            my_level_steps.resize(my_levels);
            my_level_ct_hs.resize(my_levels);
            my_level_me_hs.resize(my_levels);
            my_level_md_hs.resize(my_levels);

            for (i = 0 ; i < my_levels ; ++i) {
                my_level_steps[i] = 0;
                ptl->PtlCTAlloc(PTL_CT_OPERATION, my_level_ct_hs[i]);

                me.start = &my_level_steps[i];
                me.length = 8;
                me.match_bits = i;
                me.ignore_bits = 0;
                me.ct_handle = my_level_ct_hs[i];
                ptl->PtlMEAppend(0, me, PTL_PRIORITY_LIST, NULL, 
                                 my_level_me_hs[i]);

                md.start = &my_level_steps[i];
                me.length = 8;
                md.eq_handle = PTL_EQ_NONE;
                md.ct_handle = PTL_CT_NONE;
                ptl->PtlMDBind(md, &my_level_md_hs[i]);
            }

            md.start = &zero_buf;
            md.length = 8;
            md.eq_handle = PTL_EQ_NONE;
            md.ct_handle = PTL_CT_NONE;
            ptl->PtlMDBind(md, &zero_md_h);

            init = true;
            crReturn();
	    start_noise_section();
        }

	crReturn();
        // 200ns startup time
        start_time = cpu->getCurrentSimTimeNano();
        cpu->addBusyTime("200ns");
        crReturn();

        out_buf = in_buf;

        // Create description of user buffer.  We can't possibly have
        // a result to need this information before we add our portion
        // to the result, so this doesn't need to be persistent.
        ptl->PtlCTAlloc(PTL_CT_OPERATION, user_ct_h);
        me.start = &out_buf;
        me.length = 8;
        me.ignore_bits = ~0x0;
        me.ct_handle = user_ct_h;
        ptl->PtlMEAppend(1, me, PTL_PRIORITY_LIST, NULL, user_me_h);

        md.start = &out_buf;
        md.length = 8;
        md.eq_handle = PTL_EQ_NONE;
        md.ct_handle = PTL_CT_NONE;
        ptl->PtlMDBind(md, &user_md_h);

/*         ptl->PtlEnableCoalesce(); */
/*         crReturn(); */

        // start the trip
        ptl->PtlAtomic(user_md_h, 0, 8, 0, my_id, 0, 0, 0, NULL, 0, PTL_SUM, PTL_LONG);
        crReturn();
        ptl->PtlAtomic(user_md_h, 0, 8, 0, my_id ^ 0x1, 0, 0, 0, NULL, 0, PTL_SUM, PTL_LONG);
        crReturn();

        ptl->PtlEnableCoalesce();
        crReturn();

        for (i = 1 ; i < my_levels ; ++i) {
            next_level = 0x1 << i;
            remote = my_id ^ next_level;
            ptl->PtlTriggeredAtomic(my_level_md_hs[i - 1], 0, 8, 0, my_id, 0,
                                    i, 0, NULL, 0, PTL_SUM, PTL_LONG,
                                    my_level_ct_hs[i - 1], algo_count * 3 + 2);
            crReturn();
            ptl->PtlTriggeredAtomic(my_level_md_hs[i - 1], 0, 8, 0, remote, 0,
                                    i, 0, NULL, 0, PTL_SUM, PTL_LONG,
                                    my_level_ct_hs[i - 1], algo_count * 3 + 2);
            crReturn();
            ptl->PtlTriggeredAtomic(zero_md_h, 0, 8, 0, my_id, 0, 
                                    i - 1, 0, NULL, 0, PTL_LAND, PTL_LONG,
                                    my_level_ct_hs[i - 1], algo_count * 3 + 2);
            crReturn();
        }

        // copy into user buffer
        ptl->PtlTriggeredPut(my_level_md_hs[my_levels - 1], 0, 8, 0, my_id, 1,
                             0, 0, NULL, 0, my_level_ct_hs[my_levels - 1], algo_count * 3 + 2);
        crReturn();
        ptl->PtlTriggeredAtomic(zero_md_h, 0, 8, 0, my_id, 0, 
                                my_levels - 1, 0, NULL, 0, PTL_LAND, PTL_LONG,
                                my_level_ct_hs[my_levels - 1], algo_count * 3 + 2);
        crReturn();

        ptl->PtlDisableCoalesce();
        crReturn();

        while (!ptl->PtlCTWait(user_ct_h, 1)) { crReturn(); }
        while (!ptl->PtlCTWait(my_level_ct_hs[my_levels - 1], algo_count * 3 + 3)) { crReturn(); }

        ptl->PtlMEUnlink(user_me_h);
        crReturn();
        ptl->PtlCTFree(user_ct_h);
        crReturn();
        algo_count++;
/* 	printf("%5d: %lld ns\n",my_id,cpu->getCurrentSimTimeNano()-start_time); */
        trig_cpu::addTimeToStats(cpu->getCurrentSimTimeNano()-start_time);

        if (out_buf != (uint64_t) num_nodes) {
            printf("%05d: got %lu, expected %lu\n",
                   my_id, (unsigned long) out_buf, (unsigned long) num_nodes);
        }

        crFinish();
        return true;
    }

private:
    allreduce_recdbl_triggered();
    allreduce_recdbl_triggered(const application& a);
    void operator=(allreduce_recdbl_triggered const&);

    portals *ptl;
    SimTime_t start_time;
    int i;
    int my_levels;
    bool init;

    std::vector<uint64_t> my_level_steps;
    std::vector<ptl_handle_ct_t> my_level_ct_hs;
    std::vector<ptl_handle_me_t> my_level_me_hs;
    std::vector<ptl_handle_md_t> my_level_md_hs;

    ptl_handle_ct_t user_ct_h;
    ptl_handle_me_t user_me_h;
    ptl_handle_md_t user_md_h;

    ptl_handle_md_t zero_md_h;

    int adj;
    int next_level;
    int remote;

    uint64_t in_buf, out_buf, tmp_buf, zero_buf;

    uint64_t algo_count;
};

}
}
#endif // COMPONENTS_TRIG_CPU_ALLREDUCE_RECDBL_TRIGGERED_H
