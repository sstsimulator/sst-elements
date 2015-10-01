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


#ifndef COMPONENTS_TRIG_CPU_ALLREDUCE_RECDBL_H
#define COMPONENTS_TRIG_CPU_ALLREDUCE_RECDBL_H

#include "sst/elements/portals4_sm/trig_cpu/application.h"
#include "sst/elements/portals4_sm/trig_cpu/trig_cpu.h"

namespace SST {
namespace Portals4_sm {

class allreduce_recdbl :  public application {
public:
    allreduce_recdbl(trig_cpu *cpu) : application(cpu)
    {
        in_buf = 1;
        out_buf = 0;
    }

    bool
    operator()(Event *ev)
    {
        crBegin();

        /* Initialization case */
        for (adj = 0x1; adj <= num_nodes ; adj  <<= 1)
            ; 
        adj = adj >> 1;
        if (adj != num_nodes) {
            printf("recursive_doubling requires power of 2 nodes (%d)\n",
                   num_nodes);
            exit(1);
        }

        // 200ns startup time
        start_time = cpu->getCurrentSimTimeNano();
        cpu->addBusyTime("200ns");
        crReturn();

        out_buf = in_buf;

        for (level = 0x1 ; level < num_nodes ; level <<= 1) {
            remote = my_id ^ level;
            while (!cpu->irecv(remote, &tmp_buf, handle)) { crReturn(); }
            cpu->isend(remote, &out_buf, 8);
            crReturn();
            while (! cpu->waitall()) { crReturn(); }
            /* only one operand, so 100ns always to apply operation */
            out_buf += tmp_buf;
            cpu->addBusyTime("100ns");
            crReturn();
        }
        crReturn();

        trig_cpu::addTimeToStats(cpu->getCurrentSimTimeNano()-start_time);

        assert(out_buf == (uint64_t) cpu->getNumNodes());

        crFinish();
        return true;
    }

private:
    allreduce_recdbl();
    allreduce_recdbl(const application& a);
    void operator=(allreduce_recdbl const&);

    SimTime_t start_time;
    int level;
    int adj, handle, remote;
    uint64_t in_buf, out_buf, tmp_buf;
};

}
}

#endif // COMPONENTS_TRIG_CPU_ALLREDUCE_RECDBL_H
