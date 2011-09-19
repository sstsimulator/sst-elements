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


#ifndef _GHOST_PATTERN_H
#define _GHOST_PATTERN_H

#include "state_machine.h"
#include "comm_pattern.h"
#include "collective_topology.h"
#include "allreduce_op.h"



class Ghost_pattern : public Comm_pattern    {
    public:
        Ghost_pattern(ComponentId_t id, Params_t& params) :
            Comm_pattern(id, params)
        {
	    // Defaults for paramters
	    time_steps= 1000;
	    x_elements= 400;
	    y_elements= 400;
	    z_elements= 400;
	    loops= 16;
	    reduce_steps= 20;
	    delay= 0;
	    imbalance= 0;


	    // Process the message rate specific paramaters
            Params_t::iterator it= params.begin();
            while (it != params.end())   {
		if (!it->first.compare("time_steps"))   {
		    sscanf(it->second.c_str(), "%d", &time_steps);
		}

		if (!it->first.compare("x_elements"))   {
		    sscanf(it->second.c_str(), "%d", &x_elements);
		}

		if (!it->first.compare("y_elements"))   {
		    sscanf(it->second.c_str(), "%d", &y_elements);
		}

		if (!it->first.compare("z_elements"))   {
		    sscanf(it->second.c_str(), "%d", &z_elements);
		}

		if (!it->first.compare("loops"))   {
		    sscanf(it->second.c_str(), "%d", &loops);
		}

		if (!it->first.compare("reduce_steps"))   {
		    sscanf(it->second.c_str(), "%d", &reduce_steps);
		}

		if (!it->first.compare("delay"))   {
		    sscanf(it->second.c_str(), "%f", &delay);
		}

		if (!it->first.compare("imbalance"))   {
		    sscanf(it->second.c_str(), "%d", &imbalance);
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


	    // Kickstart ourselves
	    done= false;
	    state_transition(E_START, STATE_INIT);
        }


	// The Ghost pattern generator can be in these states and deals
	// with these events.
	typedef enum {STATE_INIT, STATE_REDUCE} ghost_state_t;

	// The start event should always be SM_START_EVENT
	typedef enum {E_START= SM_START_EVENT,
	    E_ALLREDUCE_ENTRY, E_ALLREDUCE_EXIT} ghost_events_t;

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
	void state_REDUCE(state_event sm_event);

	Params_t params;

	// State machine identifiers
	uint32_t SMghost;
	uint32_t SMallreduce;

	// Some variables we need for ghost to operate
	ghost_state_t state;
	bool done;

	// Ghost parameters
	int time_steps;
	int x_elements;
	int y_elements;
	int z_elements;
	int loops;
	int reduce_steps;
	float delay;
	int imbalance;


	// Serialization
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version )
        {
            ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
	    ar & BOOST_SERIALIZATION_NVP(params);
	    ar & BOOST_SERIALIZATION_NVP(SMghost);
	    ar & BOOST_SERIALIZATION_NVP(SMallreduce);
	    ar & BOOST_SERIALIZATION_NVP(state);
	    ar & BOOST_SERIALIZATION_NVP(done);
	    ar & BOOST_SERIALIZATION_NVP(time_steps);
	    ar & BOOST_SERIALIZATION_NVP(x_elements);
	    ar & BOOST_SERIALIZATION_NVP(y_elements);
	    ar & BOOST_SERIALIZATION_NVP(z_elements);
	    ar & BOOST_SERIALIZATION_NVP(loops);
	    ar & BOOST_SERIALIZATION_NVP(reduce_steps);
	    ar & BOOST_SERIALIZATION_NVP(delay);
	    ar & BOOST_SERIALIZATION_NVP(imbalance);
        }

        template<class Archive>
        friend void save_construct_data(Archive & ar,
                                        const Ghost_pattern * t,
                                        const unsigned int file_version)
        {
            _AR_DBG(Ghost_pattern,"\n");
            ComponentId_t     id     = t->getId();
            Params_t          params = t->params;
            ar << BOOST_SERIALIZATION_NVP(id);
            ar << BOOST_SERIALIZATION_NVP(params);
        }

        template<class Archive>
        friend void load_construct_data(Archive & ar,
                                        Ghost_pattern * t,
                                        const unsigned int file_version)
        {
            _AR_DBG(Ghost_pattern,"\n");
            ComponentId_t     id;
            Params_t          params;
            ar >> BOOST_SERIALIZATION_NVP(id);
            ar >> BOOST_SERIALIZATION_NVP(params);
            ::new(t)Ghost_pattern(id, params);
        }
};

#endif // _GHOST_PATTERN_H
