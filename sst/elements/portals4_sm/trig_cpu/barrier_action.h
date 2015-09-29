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


#ifndef COMPONENTS_TRIG_CPU_BARRIER_ACTION_H
#define COMPONENTS_TRIG_CPU_BARRIER_ACTION_H

#include <vector>
#include <cstdlib>

#include <sst/core/action.h>
#include <sst/core/link.h>
#include <sst/core/simulation.h>
#include <sst/core/timeLord.h>

#ifdef SST_CONFIG_HAVE_MPI
#include <mpi.h>
#endif

namespace SST {
namespace Portals4_sm {

class barrier_action : public Action {
public:

    barrier_action() {
	setPriority(BARRIERPRIORITY);
	
	min = 0xffffffff;
	max = 0;
	total_time = 0;
        add_count = 0;
	
	overall_min = 0xffffffff;
	overall_max = 0;
	overall_total_time = 0;
	overall_total_num = 0;
	
	rand_init = false;
	max_init_rank = -1;

	job_size = Simulation::getSimulation()->getNumRanks().rank;

// 	if ( job_size > 1 ) {
	    Simulation::getSimulation()->insertActivity(Simulation::getSimulation()->getTimeLord()->getTimeConverter("1us")->getFactor(),this);
// 	}

    }

    void execute() {
	// Check to see if everybody has entered the barrier
	Simulation *sim = Simulation::getSimulation();

// 	printf("Current time: %lu\n", sim->getTimeLord()->getNano()->convertFromCoreTime(sim->getCurrentSimCycle()));

// #ifdef SST_CONFIG_HAVE_MPI	
// 	boost::mpi::communicator world;
// #endif

	// Figure out how many still need to report
	int value = wake_up.size() - num_reporting;
	int out;

#ifdef SST_CONFIG_HAVE_MPI	
	// all_reduce( world, &value, 1, &out, std::plus<int>() );  
	MPI_Allreduce( &value, &out, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD );  
#else
        out = value;
#endif	

	if ( 0 == out ) {
#ifdef SST_CONFIG_HAVE_MPI
        // all_reduce(world, &add_count, 1, &out, std::plus<int>() );
        MPI_Allreduce(&add_count, &out, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD );
#else
        out = add_count;
#endif
        if (0 != out) {
        // Barrier is done, exchange data
        SimTime_t total_num = wake_up.size();
        SimTime_t total_num_a;
        SimTime_t min_a;
        SimTime_t max_a;
        SimTime_t total_time_a;
        int rank;
        
#ifdef SST_CONFIG_HAVE_MPI
        // all_reduce( world, &total_num , 1, &total_num_a, std::plus<SimTime_t>() );
        // all_reduce( world, &min, 1, &min_a, boost::mpi::minimum<SimTime_t>() );
        // all_reduce( world, &max, 1, &max_a, boost::mpi::maximum<SimTime_t>() );
        // all_reduce( world, &total_time, 1, &total_time_a, std::plus<SimTime_t>() );
        MPI_Allreduce( &total_num , &total_num_a, 1, MPI_UINT64_T, MPI_SUM, MPI_COMM_WORLD );
        MPI_Allreduce( &min, &min_a, 1, MPI_UINT64_T, MPI_MIN, MPI_COMM_WORLD );
        MPI_Allreduce( &max, &max_a, 1, MPI_UINT64_T, MPI_MAX, MPI_COMM_WORLD );
        MPI_Allreduce( &total_time, &total_time_a, 1, MPI_UINT64_T, MPI_SUM, MPI_COMM_WORLD );
        MPI_Comm_rank( MPI_COMM_WORLD, &rank );
#else
        max_a = max;
        min_a = min;
        total_time_a = total_time;
        total_num_a = total_num;
        rank = 0;
#endif
        if ( rank == 0 ) {
            printf("Max time: %lu ns\n", (unsigned long) max_a);
            printf("Min time: %lu ns\n", (unsigned long) min_a);
            printf("Avg time: %lu ns\n", (unsigned long) (total_time_a/total_num_a));
            printf("Total num: %d\n", (int) total_num_a);
            fflush(NULL);
        }
        resetBarrier();
        addTimeToOverallStats(max_a);
    }
	    resetStats();
        for ( unsigned int i = 0; i < wake_up.size(); i++ ) {
            wake_up[i]->send(10,new trig_cpu_event); 
        }
	}
	SimTime_t next = sim->getCurrentSimCycle() + 
	    sim->getTimeLord()->getTimeConverter("1us")->getFactor();
	sim->insertActivity( next, this );
	
    }
    
    void
    barrier()
    {
        num_reporting++;
// 	if ( job_size > 1 ) return;
//         if ( num_reporting == wake_up.size() ) {
//             // Everyone has entered barrier, wake everyone up to start
//             // over
//             for ( int i = 0; i < wake_up.size(); i++ ) {
//                 wake_up[i]->send(10,NULL); 
//             }
//             resetBarrier();
//             printStats();
//             addTimeToOverallStats(max);
//             resetStats();
//         }
    }

    void
    addTimeToStats(SimTime_t time)
    {
        if ( time < min ) min = time;
        if ( time > max ) max = time;
        total_time += time;
        add_count++;
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
        add_count = 0;
    }

    void
    printStats()
    {
        printf("Max time: %lu ns\n", (unsigned long) max);
        printf("Min time: %lu ns\n", (unsigned long) min);
        printf("Avg time: %lu ns\n", (unsigned long) (total_time/wake_up.size()));
        printf("Total num: %d\n", (int) wake_up.size());
        fflush(NULL);
    }

    void
    printOverallStats()
    {
        if (overall_total_num != 0) {
            printf("Overall Max time: %lu ns\n", (unsigned long) overall_max);
            printf("Overall Min time: %lu ns\n", (unsigned long) overall_min);
            printf("Overall Avg time: %lu ns\n", (unsigned long) (overall_total_time/overall_total_num));
            printf("Overall Total num: %d\n", (int) overall_total_num);
            fflush(NULL);
        }
    }

    int
    getRand(int rank, int max)
    {
        if (!rand_init) {
            srand(1);
            rand_init = true;
        }
        if ( max == 0 ) return 0;
	if ( max_init_rank < rank ) {
	    for ( int i = max_init_rank + 1; i <= rank; i++ ) {
		rand_nums.push_back(rand());
	    }
	    
	}
	
        return rand_nums[rank] % max;
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


private:
    SimTime_t min;
    SimTime_t max;
    SimTime_t total_time;
    int num_reporting;
    int add_count;

    SimTime_t overall_min;
    SimTime_t overall_max;
    SimTime_t overall_total_time;
    int overall_total_num;

    bool rand_init;
    int max_init_rank;
    std::vector<int> rand_nums; 
    
    std::vector<Link*> wake_up;

    int job_size;
};

}
}

#endif // COMPONENTS_TRIG_CPU_BARRIER_ACTION_H
