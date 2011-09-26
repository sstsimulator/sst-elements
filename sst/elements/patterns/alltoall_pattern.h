// Copyright 2009-2011 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2011, IBM Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _ALLTOALL_PATTERN_H
#define _ALLTOALL_PATTERN_H

#include "state_machine.h"
#include "comm_pattern.h"
#include "barrier_op.h" 
#include "allreduce_op.h" 
#include "alltoall_op.h" 



class Alltoall_pattern : public Comm_pattern    {
    public:
        Alltoall_pattern(ComponentId_t id, Params_t& params) :
            Comm_pattern(id, params)
        {
	    // Defaults for paramters
	    num_sets= 9;
	    num_ops= 200;
	    num_doubles= 1;


	    // Process the message rate specific paramaters
            Params_t::iterator it= params.begin();
            while (it != params.end())   {
		if (!it->first.compare("num_sets"))   {
		    sscanf(it->second.c_str(), "%d", &num_sets);
		}

		if (!it->first.compare("num_ops"))   {
		    sscanf(it->second.c_str(), "%d", &num_ops);
		}

		if (!it->first.compare("num_doubles"))   {
		    sscanf(it->second.c_str(), "%d", &num_doubles);
		}

                ++it;
            }


	    if (num_ranks < 2)   {
		if (my_rank == 0)   {
		    printf("#  |||  Need to run on at least two ranks!\n");
		}
		exit(-2);
	    }

	    // Install other state machines which we (alltoall pattern) need as
	    // subroutines.
	    Barrier_op *b= new Barrier_op(this);
	    SMbarrier= b->install_handler();

	    // We use an allreduce to collect the timing information from all
	    // the nodes
	    allreduce_msglen= sizeof(double);
	    a_collect= new Allreduce_op(this, allreduce_msglen, TREE_DEEP);
	    SMallreduce_collect= a_collect->install_handler();

	    // Then we need a state machine for the operation under test
	    alltoall_msglen= num_doubles;
	    a_test= new Alltoall_op(this, alltoall_msglen);
	    SMalltoall_test= a_test->install_handler();

	    // Let Comm_pattern know which handler we want to have called
	    // Make sure to call SM_create() last in the main pattern (alltoall)
	    // This is the SM that will run first
	    SMalltoall_pattern= SM->SM_create((void *)this, Alltoall_pattern::wrapper_handle_events);


	    // Kickstart ourselves
	    done= false;
	    nnodes= 1;
	    if (my_rank == 0)   {
		printf("#  |||  Alltoall Pattern test\n");
		printf("#  |||  Number of sets %d, with %d operations per set.\n",
		    num_sets, num_ops);
		printf("#  |||  Message length is %d doubles = %d bytes.\n", num_doubles,
		    (int)(num_doubles * sizeof(double)));
		printf("#  |||  nodes, min, mean, median, max, sd\n");
	    }
	    state_transition(E_START, STATE_INIT);
        }


	// The Alltoall pattern generator can be in these states and deals
	// with these events.
	typedef enum {STATE_INIT, STATE_INNER_LOOP, STATE_TEST, STATE_ALLTOALL_TEST,
	    STATE_COLLECT_RESULT, STATE_DONE} alltoall_state_t;

	// The start event should always be SM_START_EVENT
	typedef enum {E_START= SM_START_EVENT, E_NEXT_OUTER_LOOP, E_NEXT_INNER_LOOP,
	    E_NEXT_TEST, E_BARRIER_EXIT, E_ALLREDUCE_ENTRY, E_ALLREDUCE_EXIT,
	    E_ALLTOALL_ENTRY, E_ALLTOALL_EXIT, E_COLLECT, E_DONE} alltoall_events_t;

    private:

        Alltoall_pattern(const Alltoall_pattern &c);
	void handle_events(state_event sst_event);
	static void wrapper_handle_events(void *obj, state_event sst_event)
	{
	    Alltoall_pattern* mySelf = (Alltoall_pattern*) obj;
	    mySelf->handle_events(sst_event);
	}

	// The states we can be in
	void state_INIT(state_event sm_event);
	void state_INNER_LOOP(state_event sm_event);
	void state_TEST(state_event sm_event);
	void state_ALLTOALL_TEST(state_event sm_event);
	void state_COLLECT_RESULT(state_event sm_event);
	void state_DONE(state_event sm_event);

	Params_t params;
	int allreduce_msglen;
	int alltoall_msglen;

	// State machine identifiers
	uint32_t SMallreduce_collect;
	uint32_t SMalltoall_test;
	uint32_t SMbarrier;
	uint32_t SMalltoall_pattern;

	// Parameters
	int num_sets;
	int num_ops;
	int num_doubles;
	tree_type_t tree_type;

	// Runtime variables
	alltoall_state_t state;
	Allreduce_op *a_collect;
	Alltoall_op *a_test;
	int set;
	int ops;
	int nnodes;
	int done;
	SimTime_t test_start_time;
	SimTime_t duration;
	std::list <double>times;


	// Serialization
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version )
        {
            ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
	    ar & BOOST_SERIALIZATION_NVP(params);
	    ar & BOOST_SERIALIZATION_NVP(allreduce_msglen);
	    ar & BOOST_SERIALIZATION_NVP(alltoall_msglen);
	    ar & BOOST_SERIALIZATION_NVP(SMallreduce_collect);
	    ar & BOOST_SERIALIZATION_NVP(SMalltoall_test);
	    ar & BOOST_SERIALIZATION_NVP(SMbarrier);
	    ar & BOOST_SERIALIZATION_NVP(SMalltoall_pattern);
	    ar & BOOST_SERIALIZATION_NVP(state);
	    ar & BOOST_SERIALIZATION_NVP(num_sets);
	    ar & BOOST_SERIALIZATION_NVP(num_ops);
	    ar & BOOST_SERIALIZATION_NVP(num_doubles);
	    ar & BOOST_SERIALIZATION_NVP(tree_type);
	    ar & BOOST_SERIALIZATION_NVP(set);
	    ar & BOOST_SERIALIZATION_NVP(ops);
	    ar & BOOST_SERIALIZATION_NVP(nnodes);
	    ar & BOOST_SERIALIZATION_NVP(done);
	    ar & BOOST_SERIALIZATION_NVP(test_start_time);
	    ar & BOOST_SERIALIZATION_NVP(duration);
	    ar & BOOST_SERIALIZATION_NVP(times);
        }

        template<class Archive>
        friend void save_construct_data(Archive & ar,
                                        const Alltoall_pattern * t,
                                        const unsigned int file_version)
        {
            _AR_DBG(Alltoall_pattern,"\n");
            ComponentId_t     id     = t->getId();
            Params_t          params = t->params;
            ar << BOOST_SERIALIZATION_NVP(id);
            ar << BOOST_SERIALIZATION_NVP(params);
        }

        template<class Archive>
        friend void load_construct_data(Archive & ar,
                                        Alltoall_pattern * t,
                                        const unsigned int file_version)
        {
            _AR_DBG(Alltoall_pattern,"\n");
            ComponentId_t     id;
            Params_t          params;
            ar >> BOOST_SERIALIZATION_NVP(id);
            ar >> BOOST_SERIALIZATION_NVP(params);
            ::new(t)Alltoall_pattern(id, params);
        }
};

#endif // _ALLTOALL_PATTERN_H
