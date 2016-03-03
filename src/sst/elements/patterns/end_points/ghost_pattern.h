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
//
// Copyright (c) 2011, IBM Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _GHOST_PATTERN_H
#define _GHOST_PATTERN_H

#include <sst/core/params.h>

#include "patterns.h"
#include "support/state_machine.h"
#include "support/comm_pattern.h"
#include "collective_patterns/collective_topology.h"
#include "collective_patterns/allreduce_op.h"
extern "C" {
#include "benchmarks/src_ghost_bench/memory.h"
#include "benchmarks/src_ghost_bench/neighbors.h"
#include "benchmarks/src_ghost_bench/ranks.h"
#include "benchmarks/src_ghost_bench/work.h"
}



class Ghost_pattern : public Comm_pattern    {
    public:
        Ghost_pattern(ComponentId_t id, Params& params) :
            Comm_pattern(id, params),
	    decomposition_only(0)
        {
	    // Defaults for paramters
	    time_steps= 1000;
	    x_elements= 400;
	    y_elements= 400;
	    z_elements= 400;
	    loops= 16;
	    reduce_steps= 20;
	    delay= 0;
	    compute_imbalance= 0;
	    verbose= 1;
	    time_per_flop= 10;


	    // Process the message rate specific paramaters
            Params::iterator it= params.begin();
            while (it != params.end())   {
		if (!SST::Params::getParamName(it->first).compare("time_steps"))   {
		    sscanf(it->second.c_str(), "%d", &time_steps);
		}

		if (!SST::Params::getParamName(it->first).compare("x_elements"))   {
		    sscanf(it->second.c_str(), "%d", &x_elements);
		}

		if (!SST::Params::getParamName(it->first).compare("y_elements"))   {
		    sscanf(it->second.c_str(), "%d", &y_elements);
		}

		if (!SST::Params::getParamName(it->first).compare("z_elements"))   {
		    sscanf(it->second.c_str(), "%d", &z_elements);
		}

		if (!SST::Params::getParamName(it->first).compare("loops"))   {
		    sscanf(it->second.c_str(), "%d", &loops);
		}

		if (!SST::Params::getParamName(it->first).compare("reduce_steps"))   {
		    sscanf(it->second.c_str(), "%d", &reduce_steps);
		}

		if (!SST::Params::getParamName(it->first).compare("delay"))   {
		    sscanf(it->second.c_str(), "%lf", &delay);
		}

		if (!SST::Params::getParamName(it->first).compare("imbalance"))   {
		    sscanf(it->second.c_str(), "%d", &compute_imbalance);
		}

		if (!SST::Params::getParamName(it->first).compare("time_per_flop"))   {
		    sscanf(it->second.c_str(), "%d", &time_per_flop);
		}

		if (!SST::Params::getParamName(it->first).compare("verbose"))   {
		    sscanf(it->second.c_str(), "%d", &verbose);
		}

                ++it;
            }


	    if (num_ranks < 2)   {
		if (my_rank == 0)   {
		    printf("#  |||  Need to run on at least 2 ranks!\n");
		}
		exit(-2);
	    }

	    // Install other state machines which we (Ghost) need as
	    // subroutines.
	    Allreduce_op *a= new Allreduce_op(this, sizeof(double), TREE_DEEP);
	    SMallreduce= a->install_handler();

	    // Let Comm_pattern know which handler we want to have called
	    // Make sure to call SM_create() last in the main pattern (Ghost)
	    // This is the SM that will run first
	    SMghost= SM->SM_create((void *)this, Ghost_pattern::wrapper_handle_events);


	    if (my_rank == 0)   {
		printf("#  |||  Ghost cell exchange benchmark\n");
		printf("#  |||\n");
		printf("#  |||  time_steps =      %d\n", time_steps);
		printf("#  |||  x_elements =      %d\n", x_elements);
		printf("#  |||  y_elements =      %d\n", y_elements);
		printf("#  |||  z_elements =      %d\n", z_elements);
		printf("#  |||  loops =           %d\n", loops);
		printf("#  |||  reduce_steps =    %d\n", reduce_steps);
		printf("#  |||  delay =           %.2f%%\n", delay);
		printf("#  |||  imbalance =       %d\n", compute_imbalance);
		printf("#  |||  verbose =         %d\n", verbose);
		printf("#  |||  time_per_flop =   %d %s\n", time_per_flop, TIME_BASE);
		printf("#  |||\n");
	    }


	    // Assign ranks to portions of the data.
	    do_decomposition();

	    // The benchmark allocates memory. We don't do that in the pattern,
	    // but we need to know how much would have been allocated. So we call
	    // mem_alloc() directly and set the flag to NULL to avoid allocation.
	    // do_mem_alloc(my_rank, TwoD, mem_estimate, &memory, x_elements, y_elements, z_elements);
	    mem_alloc(x_elements, y_elements, z_elements, &memory, NULL);

	    /* Some info about what happened so far */
	    if ((my_rank == 0) && (verbose))   {
		if (TwoD)   {
		    printf("Border to area ratio is %.3g\n",
			(2.0 * (x_elements + y_elements)) / ((float)x_elements * y_elements));
		} else   {
		    printf("Area to volume ratio is %.3g\n",
			(2.0 * x_elements * y_elements +
			 2.0 * x_elements * z_elements +
			 2.0 * y_elements * z_elements) /
			((float)x_elements * y_elements * z_elements));
		}
	    }

	    // Kickstart ourselves
	    done= false;
	    comm_time_total= 0;
	    comp_time_total= 0;
	    total_time_start= getCurrentSimTime();
	    rcv_cnt= 0;
	    send_done= 0;
	    bytes_sent= 0;
	    reduce_cnt= 0;
	    fop_cnt= 0;
	    num_sends= 0;
	    compute_delay= 0;
	    wait_last_receives=false;

	    if (TwoD)   {
		neighbor_cnt= 4;
	    } else   {
		neighbor_cnt= 6;
	    }
	    t= -1;
	    // MOVED TO setup() FOR PROPER INITIALIZATION - ALEVINE
//	    state_transition(E_START, STATE_INIT);
        }


	// The Ghost pattern generator can be in these states and deals
	// with these events.
	typedef enum {STATE_INIT, STATE_CELL_EXCHANGE, STATE_COMPUTE_CELLS,
	    STATE_REDUCE, STATE_DONE} ghost_state_t;

	// The start event should always be SM_START_EVENT
	typedef enum {E_START= SM_START_EVENT, E_NEXT_LOOP, E_CELL_SEND,
	    E_SEND_DONE, E_COMPUTE, E_COMPUTE_DONE, E_CELL_RECEIVE,
	    E_ALLREDUCE_ENTRY, E_ALLREDUCE_EXIT, E_DONE} ghost_events_t;

    private:

        Ghost_pattern(const Ghost_pattern &c);
	void handle_events(state_event sst_event);
	static void wrapper_handle_events(void *obj, state_event sst_event)
	{
	    Ghost_pattern* mySelf = (Ghost_pattern*) obj;
	    mySelf->handle_events(sst_event);
	}

	// The states we can be in
	void state_INIT(state_event sm_event);
	void state_CELL_EXCHANGE(state_event sm_event);
	void state_COMPUTE_CELLS(state_event sm_event);
	void state_REDUCE(state_event sm_event);
	void state_DONE(state_event sm_event);

	// Other functions
	void do_decomposition(void);

	Params params;

	// State machine identifiers
	uint32_t SMghost;
	uint32_t SMallreduce;

	// Some variables we need for the ghost SM to operate
	ghost_state_t state;
	bool done;

	// Ghost parameters
	int time_steps;
	int x_elements;
	int y_elements;
	int z_elements;
	int loops;
	int reduce_steps;
	double delay;
	int verbose;
	int time_per_flop;	// In nano seconds

	// Variables for the benchmark
	int TwoD;
	int rank_width, rank_height, rank_depth;
	int decomposition_only;
	size_t mem_estimate;
	mem_ptr_t memory;
	neighbors_t neighbor_list;
	int t;
	SimTime_t total_time_start;
	SimTime_t total_time_end;
	SimTime_t comm_time_start;
	SimTime_t comm_time_total;
	SimTime_t comp_time_start;
	SimTime_t comp_time_total;
	SimTime_t compute_delay;
	int64_t num_sends;
	int64_t fop_cnt;
	int64_t reduce_cnt;
	int64_t bytes_sent;
	int compute_imbalance;
	int rcv_cnt;
	int send_done;
	bool wait_last_receives;
	int neighbor_cnt;

	// ADDED FOR PROPER INITIALIZATION - ALEVINE
	// SST Startup and Shutdown
	void setup();

};

#endif // _GHOST_PATTERN_H
