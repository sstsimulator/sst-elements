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


#ifndef COMPONENTS_TRIG_CPU_ALLREDUCE_TREE_H
#define COMPONENTS_TRIG_CPU_ALLREDUCE_TREE_H

#include "sst/elements/portals4_sm/trig_cpu/application.h"
#include "sst/elements/portals4_sm/trig_cpu/trig_cpu.h"

namespace SST {
namespace Portals4_sm {

class allreduce_tree :  public application {
public:
    allreduce_tree(trig_cpu *cpu, bool nary) : application(cpu)
    {
        radix = cpu->getRadix();

        if (nary) {
            boost::tie(my_root, my_children) = buildNaryTree(radix);
        } else {
            boost::tie(my_root, my_children) = buildBinomialTree(radix);
        }
        num_children = my_children.size();

        in_buf = 1;
        out_buf = 0;
        tmp_buf = (uint64_t*) malloc(sizeof(uint64_t) * (num_children == 0 ? 1 : num_children));
    }

    bool
    operator()(Event *ev)
    {
        crBegin();

        start_time = cpu->getCurrentSimTimeNano();
        cpu->addBusyTime("200ns");
        crReturn();

        // receive from below
        if (0 != num_children) {
            for (i = 0 ; i < num_children ; ++i) {
                while (!cpu->irecv(my_children[i], &tmp_buf[i], handle)) { crReturn(); }
            }
            while (!cpu->waitall()) { crReturn(); }
        }

        // compute value and time
        out_buf = in_buf;
        for (i = 0 ; i < num_children ; ++i) {
            out_buf += tmp_buf[i];
        }
        for (i = 0 ; i < (num_children / 8) + 1 ; ++i) {
            cpu->addBusyTime("100ns");
            crReturn();
        }

        // send up a level and wait for response if not root
        if (my_root != my_id) {
            cpu->isend(my_root, &out_buf, 8);
            crReturn();
            while (!cpu->irecv(my_root, &out_buf, handle)) { crReturn(); }
            while (!cpu->waitall()) { crReturn(); }
        }

        // broadcast down level
        for (i = 0 ; i < num_children ; ++i) {
            cpu->isend(my_children[i], &out_buf, 8);
            crReturn();
        }

        crReturn();
        trig_cpu::addTimeToStats(cpu->getCurrentSimTimeNano() - start_time);

        assert(out_buf == (uint64_t) cpu->getNumNodes());

        crFinish();
        return true;
    }

private:
    allreduce_tree();
    allreduce_tree(const application& a);
    void operator=(allreduce_tree const&);

    SimTime_t start_time;
    int radix;
    int i;
    int handle;

    uint64_t in_buf, out_buf;
    uint64_t *tmp_buf;

    int my_root;
    std::vector<int> my_children;
    int num_children;
};

}
}

#endif // COMPONENTS_TRIG_CPU_ALLREDUCE_TREE_H
