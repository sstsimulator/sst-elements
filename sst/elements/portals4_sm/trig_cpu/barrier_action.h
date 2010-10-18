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


#ifndef COMPONENTS_TRIG_CPU_BARRIER_ACTION_H
#define COMPONENTS_TRIG_CPU_BARRIER_ACTION_H

#include <vector>
#include <cstdlib>

#include <sst/core/action.h>
#include <sst/core/link.h>

class barrier_action : public Action {
public:

    barrier_action() {
	min = 0xffffffff;
	max = 0;
	total_time = 0;
	
	overall_min = 0xffffffff;
	overall_max = 0;
	overall_total_time = 0;
	overall_total_num = 0;
	
	rand_init = false;
    }

    void execute() {}
    
    void
    addTimeToStats(SimTime_t time)
    {
        if ( time < min ) min = time;
        if ( time > max ) max = time;
        total_time += time;
    }

    void
    addTimeToOverallStats(SimTime_t time)
    {
        if ( time < overall_min ) overall_min = time;
        if ( time > overall_max ) overall_max = time;
        overall_total_time += time;
        overall_total_num++;
    }

    void
    resetStats()
    {
        min = 0xffffffff;
        max = 0;
        total_time = 0;
    }

    void
    printStats()
    {
        printf("Max time: %lu ns\n", (unsigned long) max);
        printf("Min time: %lu ns\n", (unsigned long) min);
        printf("Avg time: %lu ns\n", (unsigned long) (total_time/wake_up.size()));
        printf("Total num: %d\n", wake_up.size());
        fflush(NULL);
    }

    void
    printOverallStats()
    {
        printf("Overall Max time: %lu ns\n", (unsigned long) overall_max);
        printf("Overall Min time: %lu ns\n", (unsigned long) overall_min);
        printf("Overall Avg time: %lu ns\n", (unsigned long) (overall_total_time/overall_total_num));
        printf("Overall Total num: %d\n", overall_total_num);
        fflush(NULL);
    }

    int
    getRand(int max)
    {
        if (!rand_init) {
            srand(0);
            rand_init = true;
        }
        if ( max == 0 ) return 0;
        return rand() % max;
    }

    void
    addWakeUp(Link* link)
    {
        wake_up.push_back(link);
    }

/*     void */
/*     setTotalNodes(int total) */
/*     { */
/*         if ( wake_up == NULL ) wake_up = new Link*[total]; */
/*         total_nodes = total; */
/*     } */

    void
    resetBarrier()
    {
        num_reporting = 0;
    }

    void
    barrier()
    {
        num_reporting++;
// 	printf("  num_reporting = %d\n",num_reporting);
        if ( num_reporting == wake_up.size() ) {
            // Everyone has entered barrier, wake everyone up to start
            // over
            for ( int i = 0; i < wake_up.size(); i++ ) {
                wake_up[i]->Send(10,NULL);
            }
            resetBarrier();
            printStats();
            addTimeToOverallStats(max);
            resetStats();
        }
    }

private:
    SimTime_t min;
    SimTime_t max;
    SimTime_t total_time;
    int num_reporting;

    SimTime_t overall_min;
    SimTime_t overall_max;
    SimTime_t overall_total_time;
    int overall_total_num;

    bool rand_init;

    std::vector<Link*> wake_up;
    
};

#endif // COMPONENTS_TRIG_CPU_BARRIER_ACTION_H
