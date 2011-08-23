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


#ifndef _ALLREDUCE_OP_H
#define _ALLREDUCE_OP_H

#include "state_machine.h"
#include "comm_pattern.h"
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
	    nnodes= 1;		// Start test with 1 node


	    // Process the message rate specific paramaters
            Params_t::iterator it= params.begin();
            while (it != params.end())   {
		if (!it->first.compare("num_sets"))   {
		    sscanf(it->second.c_str(), "%d", &num_sets);
		}

		if (!it->first.compare("num_ops"))   {
		    sscanf(it->second.c_str(), "%d", &num_ops);
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

	    allreduce_msglen= sizeof(double);
	    Allreduce_op *a= new Allreduce_op(this, allreduce_msglen);
	    SMallreduce= a->install_handler();

	    // Let Comm_pattern know which handler we want to have called
	    // Make sure to call SM_create() last in the main pattern (allreduce)
	    // This is the SM that will run first
	    SMallreduce_bench= SM->SM_create((void *)this, Allreduce_pattern::wrapper_handle_events);


	    // Kickstart ourselves
	    done= false;
	    state_transition(E_START, STATE_INIT);
        }


	// The Allreduce pattern generator can be in these states and deals
	// with these events.
	typedef enum {STATE_INIT, STATE_INNER_LOOP, STATE_TEST, STATE_ALLREDUCE_TEST,
	    STATE_COLLECT_RESULT, STATE_DONE} allreduce_state_t;

	// The start event should always be SM_START_EVENT
	typedef enum {E_START= SM_START_EVENT, E_NEXT_OUTER_LOOP, E_NEXT_INNER_LOOP} allreduce_events_t;

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
	uint32_t SMallreduce;
	uint32_t SMbarrier;
	uint32_t SMallreduce_bench;

	// Some variables we need for allreduce to operate
	allreduce_state_t state;
	int num_sets;
	int num_ops;
	int rcv_cnt;
	int nnodes;
	int done;
	SimTime_t test_start_time_start;
	double duration;


	// Serialization
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version )
        {
            ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
	    ar & BOOST_SERIALIZATION_NVP(params);
	    ar & BOOST_SERIALIZATION_NVP(allreduce_msglen);
	    ar & BOOST_SERIALIZATION_NVP(SMallreduce);
	    ar & BOOST_SERIALIZATION_NVP(SMbarrier);
	    ar & BOOST_SERIALIZATION_NVP(SMallreduce_bench);
	    ar & BOOST_SERIALIZATION_NVP(state);
	    ar & BOOST_SERIALIZATION_NVP(num_sets);
	    ar & BOOST_SERIALIZATION_NVP(num_ops);
	    ar & BOOST_SERIALIZATION_NVP(rcv_cnt);
	    ar & BOOST_SERIALIZATION_NVP(nnodes);
	    ar & BOOST_SERIALIZATION_NVP(done);
	    ar & BOOST_SERIALIZATION_NVP(test_start_time_start);
	    ar & BOOST_SERIALIZATION_NVP(duration);
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

#endif // _ALLREDUCE_OP_H
