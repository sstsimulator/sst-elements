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


#ifndef COMPONENTS_TRIG_CPU_BARRIER_DISSEMINATION_H
#define COMPONENTS_TRIG_CPU_BARRIER_DISSEMINATION_H

#include "sst/elements/portals4_sm/trig_cpu/application.h"
#include "sst/elements/portals4_sm/trig_cpu/trig_cpu.h"

namespace SST {
namespace Portals4_sm {


class barrier_dissemination :  public application {
public:
    barrier_dissemination(trig_cpu *cpu) : application(cpu)
    {
        radix = cpu->getRadix();
    }

    bool
    operator()(Event *ev)
    {
        int handle;
        crBegin();

        // 200ns startup time
        start_time = cpu->getCurrentSimTimeNano();
        cpu->addBusyTime("200ns");
        crReturn();

        shiftval = floorLog2(radix);

        for (level = 0x1 ; level < num_nodes ; level <<= shiftval) {
            for (i = 0 ; i < (radix - 1) ; i++) {
                cpu->isend((my_id + level + i) % num_nodes, NULL, 0);
                crReturn();
                while (!cpu->irecv((my_id + num_nodes - (level + i)) % num_nodes, 
                                   NULL, handle)) { crReturn(); }
                crReturn();
                while (!cpu->waitall()) { crReturn(); }
                crReturn();
            }
        }

        trig_cpu::addTimeToStats(cpu->getCurrentSimTimeNano()-start_time);

        crFinish();
        return true;
    }

private:
    barrier_dissemination();
    barrier_dissemination(const application& a);
    void operator=(barrier_dissemination const&);

    SimTime_t start_time;
    int radix;
    int shiftval;

    int i;
    int level;
};

}
}

#endif // COMPONENTS_TRIG_CPU_BARRIER_DISSEMINATION_H
