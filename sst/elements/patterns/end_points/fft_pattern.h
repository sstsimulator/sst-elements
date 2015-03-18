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


#ifndef _FFT_PATTERN_H
#define _FFT_PATTERN_H

#include <sst/core/params.h>

#include "patterns.h"
#include "support/state_machine.h"
#include "support/comm_pattern.h"
#include "collective_patterns/collective_topology.h" 
#include "collective_patterns/scatter_op.h" 
#include "collective_patterns/gather_op.h" 



class FFT_pattern : public Comm_pattern    {
    public:
        FFT_pattern(ComponentId_t id, Params& params) :
            Comm_pattern(id, params)
        {
	    // Defaults for paramters
	    N= -1;
	    iter= 1;
	    tree_type= TREE_DEEP;
	    time_per_flop= 10;
	    verbose= 0;


	    // Process the message rate specific paramaters
            Params::iterator it= params.begin();
            while (it != params.end())   {
		if (!SST::Params::getParamName(it->first).compare("N"))   {
		    sscanf(it->second.c_str(), "%d", &N);
		}

		if (!SST::Params::getParamName(it->first).compare("iter"))   {
		    sscanf(it->second.c_str(), "%d", &iter);
		}

		if (!SST::Params::getParamName(it->first).compare("tree_type"))   {
		    if (!it->second.compare("deep"))   {
			tree_type= TREE_DEEP;
		    } else if (!it->second.compare("binary"))   {
			tree_type= TREE_BINARY;
		    } else   {
			if (my_rank == 0)   {
			    printf("#  |||  Unknown tree type!\n");
			}
			exit(-2);
		    }
		}

		if (!SST::Params::getParamName(it->first).compare("time_per_flop"))   {
		    sscanf(it->second.c_str(), "%d", &time_per_flop);
		}

		if (!SST::Params::getParamName(it->first).compare("verbose"))   {
		    sscanf(it->second.c_str(), "%d", &verbose);
		}

                ++it;
            }

	    if (N <= 0)   {
		if (my_rank == 0)   {
		    printf("#  |||  N must be specified in patter input file!\n");
		}
		exit(-3);
	    }

	    if (N % num_ranks != 0)   {
		if (my_rank == 0)   {
		    printf("#  |||  N should be an integer multiple of number of ranks!\n");
		}
		exit(-4);
	    }

	    // Install other state machines which we (FFT pattern) need as
	    // subroutines.
	    scatter_msglen= sizeof(double) * N / num_ranks;
	    Scatter_op *a= new Scatter_op(this, scatter_msglen, tree_type);
	    SMscatter= a->install_handler();

	    gather_msglen= sizeof(double) * 2 * N / num_ranks;
	    Gather_op *b= new Gather_op(this, gather_msglen, tree_type);
	    SMgather= b->install_handler();

	    // Let Comm_pattern know which handler we want to have called
	    // Make sure to call SM_create() last in the main pattern (FFT)
	    // This is the SM that will run first
	    SMfft_pattern= SM->SM_create((void *)this, FFT_pattern::wrapper_handle_events);


	    // Kickstart ourselves
	    done= false;
	    if (my_rank == 0)   {
		printf("#  |||  FFT Pattern test\n");
		printf("#  |||      Number of iterations   %d\n", iter);
		printf("#  |||      Vector size N is       %d\n", N);
		printf("#  |||      Tree type is           ");
		switch (tree_type)   {
		    case TREE_DEEP:
			printf("deep\n");
			break;
		    case TREE_BINARY:
			printf("binary\n");
			break;
		}
		printf("#  |||      time_per_flop =        %d %s\n", time_per_flop, TIME_BASE);
		printf("#\n");
	    }
	    // MOVED TO setup() FOR PROPER INITIALIZATION - ALEVINE
//	    state_transition(E_START, STATE_INIT);
        }


	// The FFT pattern generator can be in these states and deals
	// with these events.
	typedef enum {STATE_INIT, STATE_PHASE1, STATE_PHASE2, STATE_PHASE3,
	    STATE_PHASE4, STATE_PHASE5, STATE_DONE} fft_state_t;

	// The start event should always be SM_START_EVENT
	typedef enum {E_START= SM_START_EVENT, E_START_PHASE2, E_COMPUTE_DONE, E_COMPUTE,
	    E_SCATTER_ENTRY, E_SCATTER_EXIT, E_PHASE3_ENTRY, E_PHASE3_EXIT,
	    E_PHASE4_ENTRY, E_PHASE4_EXIT, E_GATHER_ENTRY, E_GATHER_EXIT,
	    E_DONE} fft_events_t;

    private:

#ifdef SERIALIZATION_WORKS_NOW
        FFT_pattern();  // For serialization only
#endif  // SERIALIZATION_WORKS_NOW
        FFT_pattern(const FFT_pattern &c);
	void handle_events(state_event sst_event);
	static void wrapper_handle_events(void *obj, state_event sst_event)
	{
	    FFT_pattern* mySelf = (FFT_pattern*) obj;
	    mySelf->handle_events(sst_event);
	}

	// The states we can be in
	void state_INIT(state_event sm_event);
	void state_PHASE1(state_event sm_event);
	void state_PHASE2(state_event sm_event);
	void state_PHASE3(state_event sm_event);
	void state_PHASE4(state_event sm_event);
	void state_PHASE5(state_event sm_event);
	void state_DONE(state_event sm_event);

	Params params;
	int scatter_msglen;
	int gather_msglen;

	// State machine identifiers
	uint32_t SMgather;
	uint32_t SMscatter;
	uint32_t SMfft_pattern;

	// Parameters
	int N;
	int iter;
	tree_type_t tree_type;

	// Runtime variables
	fft_state_t state;
	int i;
	int done;
	int time_per_flop;		// In nano seconds
	int verbose;
	SimTime_t test_start_time;
	SimTime_t phase1_time;
	SimTime_t phase2_time;
	SimTime_t phase3_time;
	SimTime_t phase4_time;
	SimTime_t phase5_time;
	SimTime_t total_time;
	int M;

	// ADDED FOR PROPER INITIALIZATION - ALEVINE
	// SST Startup and Shutdown
	void setup();

	// Serialization
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version )
        {
            ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Comm_pattern);
	    ar & BOOST_SERIALIZATION_NVP(params);
	    ar & BOOST_SERIALIZATION_NVP(scatter_msglen);
	    ar & BOOST_SERIALIZATION_NVP(gather_msglen);
	    ar & BOOST_SERIALIZATION_NVP(SMgather);
	    ar & BOOST_SERIALIZATION_NVP(SMscatter);
	    ar & BOOST_SERIALIZATION_NVP(SMfft_pattern);
	    ar & BOOST_SERIALIZATION_NVP(N);
	    ar & BOOST_SERIALIZATION_NVP(iter);
	    ar & BOOST_SERIALIZATION_NVP(tree_type);
	    ar & BOOST_SERIALIZATION_NVP(state);
	    ar & BOOST_SERIALIZATION_NVP(i);
	    ar & BOOST_SERIALIZATION_NVP(done);
	    ar & BOOST_SERIALIZATION_NVP(time_per_flop);
	    ar & BOOST_SERIALIZATION_NVP(verbose);
	    ar & BOOST_SERIALIZATION_NVP(test_start_time);
	    ar & BOOST_SERIALIZATION_NVP(phase1_time);
	    ar & BOOST_SERIALIZATION_NVP(phase2_time);
	    ar & BOOST_SERIALIZATION_NVP(phase3_time);
	    ar & BOOST_SERIALIZATION_NVP(phase4_time);
	    ar & BOOST_SERIALIZATION_NVP(phase5_time);
	    ar & BOOST_SERIALIZATION_NVP(total_time);
	    ar & BOOST_SERIALIZATION_NVP(M);
        }
};

#endif // _FFT_PATTERN_H
