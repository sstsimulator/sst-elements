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


#ifndef COMPONENTS_TRIG_CPU_PINGPONG_BW_EAGER_H
#define COMPONENTS_TRIG_CPU_PINGPONG_BW_EAGER_H

#include "sst/elements/portals4_sm/trig_cpu/application.h"
#include <string.h>		       // for memcpy()

#define MAX(a, b)  (a < b) ? b : a

class pingpong_bw_eager :  public application {
public:
    pingpong_bw_eager(trig_cpu *cpu) : application(cpu)
    {
        start_len = 1;
        stop_len = 8 * 1024 * 1024;
        perturbation = 3;
        nrepeat = 1000;
        send_buf = (char*) malloc(stop_len * 2);
        recv_buf = (char*) malloc(stop_len * 2);
    }

    bool
    operator()(Event *ev)
    {
        crInit();

        crFuncStart(send);
        crFuncEnd();

        crFuncStart(recv);
        crFuncEnd();

        crStart();

        inc = (start_len > 1) ? start_len / 2 : 1;
        nq = (start_len > 1) ? 1 : 0;

        for (n = 0, len = start_len ; len <= stop_len ; len += inc, nq++) {
            if (nq > 2) inc = ((nq % 2))? inc + inc: inc;
            for (pert = ((perturbation > 0) && (inc > perturbation+1)) ? -perturbation : 0;
                 pert <= perturbation; 
                 n++, pert += ((perturbation > 0) && (inc > perturbation+1)) ? perturbation : perturbation+1) {

                start_time = cpu->getCurrentSimTimeNano();
                cur_len = len + pert;

                for (i = 0 ; i < nrepeat ; ++i) {
                    if (my_id == 0) {
                        peer = 1;
                        crFuncCall(send);
                        crFuncCall(recv);
                    } else if (my_id == 1) {
                        peer = 0;
                        crFuncCall(recv);
                        crFuncCall(send);
                    }
                }
                stop_time = cpu->getCurrentSimTimeNano();

                tlast = ((double) (stop_time - start_time)) / 1000000000.0 / nrepeat;

                if (my_id == 0) {
                    printf("%8d\t%lf\t%12.8lf\n", cur_len, cur_len / tlast, tlast);
                }
            }
        }
        crReturn();

        crFinish();
        return true;
    }

private:
    pingpong_bw_eager();
    pingpong_bw_eager(const application& a);
    void operator=(pingpong_bw_eager const&);

    SimTime_t start_time, stop_time;
    int i, n, len, start_len, stop_len, inc, nq, pert, perturbation, cur_len, nrepeat, peer, handle;
    char *send_buf, *recv_buf;
    double tlast;
};

#endif // COMPONENTS_TRIG_CPU_PINGPONG_BW_EAGER_H
