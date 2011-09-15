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


#ifndef _ALLREDUCE_PATTERN_H
#define _ALLREDUCE_PATTERN_H

#include "state_machine.h"
#include "comm_pattern.h"
#include "collective_topology.h" 
#include "barrier_op.h" 
#include "allreduce_op.h" 



class Allreduce_pattern : public Comm_pattern    {
    public:
        Allreduce_pattern(ComponentId_t id, Params_t& params) :
            Comm_pattern(id, params)
        {
	    // Defaults for paramters
	    num_sets= 9;
	    num_ops= 200;
	    num_doubles= 1;
	    tree_type= TREE_DEEP;


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

		if (!it->first.compare("tree_type"))   {
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

                ++it;
            }


	    if (num_ranks < 2)   {
		if (my_rank == 0)   {
		    printf("#  |||  Need to run on at least two ranks!\n");
		}
		exit(-2);
	    }

	    // Install other state machines which we (allreduce pattern) need as
	    // subroutines.
	    Barrier_op *b= new Barrier_op(this);
	    SMbarrier= b->install_handler();

	    // We are going to install two allreduce state machines
	    // One we use for testing with different number of nodes.
	    // The other we use to collect the timing information from all
	    // the nodes
	    allreduce_msglen= sizeof(double);
	    a_collect= new Allreduce_op(this, allreduce_msglen, TREE_DEEP);
	    SMallreduce_collect= a_collect->install_handler();

	    allreduce_msglen= num_doubles * sizeof(double);
	    a_test= new Allreduce_op(this, allreduce_msglen, tree_type);
	    SMallreduce_test= a_test->install_handler();

	    // Let Comm_pattern know which handler we want to have called
	    // Make sure to call SM_create() last in the main pattern (allreduce)
	    // This is the SM that will run first
	    SMallreduce_pattern= SM->SM_create((void *)this, Allreduce_pattern::wrapper_handle_events);


	    // Kickstart ourselves
	    done= false;
	    nnodes= 0;
	    if (my_rank == 0)   {
		printf("#  |||  Allreduce Pattern test\n");
		printf("#  |||  Number of sets %d, with %d operations per set.\n",
		    num_sets, num_ops);
		printf("#  |||  Message length is %d doubles = %d bytes.\n", num_doubles,
		    (int)(num_doubles * sizeof(double)));
		printf("#  |||  Tree type is ");
		switch (tree_type)   {
		    case TREE_DEEP:
			printf("deep\n");
			break;
		    case TREE_BINARY:
			printf("binary\n");
			break;
		}
		printf("#  |||  nodes, min, mean, median, max, sd\n");
	    }
	    state_transition(E_START, STATE_INIT);
        }


	// The Allreduce pattern generator can be in these states and deals
	// with these events.
	typedef enum {STATE_INIT, STATE_INNER_LOOP, STATE_TEST, STATE_ALLREDUCE_TEST,
	    STATE_COLLECT_RESULT, STATE_DONE} allreduce_state_t;

	// The start event should always be SM_START_EVENT
	typedef enum {E_START= SM_START_EVENT, E_NEXT_OUTER_LOOP, E_NEXT_INNER_LOOP,
	    E_NEXT_TEST, E_BARRIER_EXIT, E_ALLREDUCE_ENTRY, E_ALLREDUCE_EXIT,
	    E_COLLECT, E_DONE} allreduce_events_t;

    private:

        Allreduce_pattern(const Allreduce_pattern &c);
	void handle_events(state_event sst_event);
	static void wrapper_handle_events(void *obj, state_event sst_event)
	{
	    Allreduce_pattern* mySelf = (Allreduce_pattern*) obj;
	    mySelf->handle_events(sst_event);
	}

	// The states we can be in
	void state_INIT(state_event sm_event);
	void state_INNER_LOOP(state_event sm_event);
	void state_TEST(state_event sm_event);
	void state_ALLREDUCE_TEST(state_event sm_event);
	void state_COLLECT_RESULT(state_event sm_event);
	void state_DONE(state_event sm_event);

	Params_t params;
	int allreduce_msglen;

	// State machine identifiers
	uint32_t SMallreduce_collect;
	uint32_t SMallreduce_test;
	uint32_t SMbarrier;
	uint32_t SMallreduce_pattern;

	// Parameters
	int num_sets;
	int num_ops;
	int num_doubles;
	tree_type_t tree_type;

	// Runtime variables
	allreduce_state_t state;
	Allreduce_op *a_collect;
	Allreduce_op *a_test;
	int set;
	int ops;
	int nnodes;
	int done;
	SimTime_t test_start_time_start;
	double duration;
	std::list <double>times;


	// Serialization
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version )
        {
            ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
	    ar & BOOST_SERIALIZATION_NVP(params);
	    ar & BOOST_SERIALIZATION_NVP(allreduce_msglen);
	    ar & BOOST_SERIALIZATION_NVP(SMallreduce_collect);
	    ar & BOOST_SERIALIZATION_NVP(SMallreduce_test);
	    ar & BOOST_SERIALIZATION_NVP(SMbarrier);
	    ar & BOOST_SERIALIZATION_NVP(SMallreduce_pattern);
	    ar & BOOST_SERIALIZATION_NVP(state);
	    ar & BOOST_SERIALIZATION_NVP(num_sets);
	    ar & BOOST_SERIALIZATION_NVP(num_ops);
	    ar & BOOST_SERIALIZATION_NVP(num_doubles);
	    ar & BOOST_SERIALIZATION_NVP(tree_type);
	    ar & BOOST_SERIALIZATION_NVP(set);
	    ar & BOOST_SERIALIZATION_NVP(ops);
	    ar & BOOST_SERIALIZATION_NVP(nnodes);
	    ar & BOOST_SERIALIZATION_NVP(done);
	    ar & BOOST_SERIALIZATION_NVP(test_start_time_start);
	    ar & BOOST_SERIALIZATION_NVP(duration);
	    ar & BOOST_SERIALIZATION_NVP(times);
        }

        template<class Archive>
        friend void save_construct_data(Archive & ar,
                                        const Allreduce_pattern * t,
                                        const unsigned int file_version)
        {
            _AR_DBG(Allreduce_pattern,"\n");
            ComponentId_t     id     = t->getId();
            Params_t          params = t->params;
            ar << BOOST_SERIALIZATION_NVP(id);
            ar << BOOST_SERIALIZATION_NVP(params);
        }

        template<class Archive>
        friend void load_construct_data(Archive & ar,
                                        Allreduce_pattern * t,
                                        const unsigned int file_version)
        {
            _AR_DBG(Allreduce_pattern,"\n");
            ComponentId_t     id;
            Params_t          params;
            ar >> BOOST_SERIALIZATION_NVP(id);
            ar >> BOOST_SERIALIZATION_NVP(params);
            ::new(t)Allreduce_pattern(id, params);
        }
};

#endif // _ALLREDUCE_PATTERN_H
