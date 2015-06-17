//
// Copyright (c) 2011, IBM Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _PINGPONG_PATTERN_H
#define _PINGPONG_PATTERN_H

#include <sst/core/params.h>

#include "patterns.h"
#include "support/state_machine.h"
#include "support/comm_pattern.h"
#include "collective_patterns/barrier_op.h"
#include "collective_patterns/collective_topology.h"
#include "collective_patterns/allreduce_op.h"



class Pingpong_pattern : public Comm_pattern {
    public:
        Pingpong_pattern(ComponentId_t id, Params& params) :
	    // constructor initializer list                                                                   
	    Comm_pattern(id, params)
	{

	    // Messages are exchanged between rank 0 and "dest"
	    // The default for "dest" is to place it as far away
	    // as possible in the (logical) torus created by the
	    // Comm_pattern object
	    dest= machine->get_total_cores() / 2;

	    // Set some more defaults
	    num_msgs= 10;
	    end_len= 1048576;
	    user_len_inc= -1;
	    len_inc= 8;
	    allreduce_msglen= sizeof(double);



	    // Process the ping/pong pattern specific parameters
	    Params::iterator it= params.begin();

	    while (it != params.end())   {
		if (!SST::Params::getParamName(it->first).compare("destination"))   {
		    sscanf(it->second.c_str(), "%d", &dest);
		}

		if (!SST::Params::getParamName(it->first).compare("num_msgs"))   {
		    sscanf(it->second.c_str(), "%d", &num_msgs);
		}

		if (!SST::Params::getParamName(it->first).compare("end_len"))   {
		    sscanf(it->second.c_str(), "%d", &end_len);
		}

		if (!SST::Params::getParamName(it->first).compare("len_inc"))   {
		    sscanf(it->second.c_str(), "%d", &user_len_inc);
		}

		// Parameter for allreduce
		if (!SST::Params::getParamName(it->first).compare("allreduce_msglen"))   {
		    sscanf(it->second.c_str(), "%d", &allreduce_msglen);
		}

		it++;
	    }

	    if (dest >= num_ranks)   {
		out.fatal(CALL_INFO, -1, "[%3d] Invalid destination %d for %d ranks\n",
		    my_rank, dest, num_ranks);
	    }

	    // Install other state machines which we (pingpong) need as
	    // subroutines.
	    // For pingpong we don't really need them, but we include
	    // them here as an example, and for testing the gate keeper
	    // switiching mechanism among state machines
	    Barrier_op *b= new Barrier_op(this);
	    SMbarrier= b->install_handler();

	    Allreduce_op *a= new Allreduce_op(this, allreduce_msglen, TREE_DEEP);
	    SMallreduce= a->install_handler();

	    // Let Comm_pattern know which handler we want to have called
	    // Make sure to call SM_create() last in the main pattern (pingpong)
	    // This is the SM that will run first
	    SMpingpong= SM->SM_create((void *)this, Pingpong_pattern::wrapper_handle_events);

	    // Kickstart ourselves
	    // MOVED TO setup() FOR PROPER INITIALIZATION - ALEVINE
//	    state_transition(E_START, PP_INIT);
        }

        ~Pingpong_pattern()
	{}


	// The Pingpong pattern generator can be in these states and deals
	// with these events.
	typedef enum {PP_INIT, PP_RECEIVING, PP_BARRIER, PP_ALLREDUCE, PP_DONE} pingpong_state_t;

	// The start event should always be SM_START_EVENT
	typedef enum {E_START= SM_START_EVENT, E_BARRIER_ENTRY,
	    E_BARRIER_EXIT, E_ALLREDUCE_ENTRY, E_ALLREDUCE_EXIT, E_RECEIVE} pingpong_events_t;



    private:
#ifdef SERIALIZATION_WORKS_NOW
	Pingpong_pattern();  // For serialization only
#endif  // SERIALIZATION_WORKS_NOW
	Pingpong_pattern(const Pingpong_pattern &c);
	void handle_events(state_event sst_event);
	static void wrapper_handle_events(void *obj, state_event sst_event)
	{
	    Pingpong_pattern* mySelf = (Pingpong_pattern*) obj;
	    mySelf->handle_events(sst_event);
	}

	void state_INIT(state_event sm_event);
	void state_RECEIVING(state_event sm_event);
	void state_BARRIER(state_event sm_event);
	void state_ALLREDUCE(state_event sm_event);
	Params params;
	int allreduce_msglen;

	// State machine identifiers
	uint32_t SMpingpong;
	uint32_t SMbarrier;
	uint32_t SMallreduce;

	// Some variables we need for pingpong to operate
	int cnt;
	int done;
	int len;
	int end_len;
	int num_msgs;
	int len_inc;
	int user_len_inc;
	SimTime_t start_time;
	int first_receive;
	int dest;
	pingpong_state_t state;

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
	    ar & BOOST_SERIALIZATION_NVP(allreduce_msglen);
	    ar & BOOST_SERIALIZATION_NVP(SMpingpong);
	    ar & BOOST_SERIALIZATION_NVP(SMbarrier);
	    ar & BOOST_SERIALIZATION_NVP(SMallreduce);
	    ar & BOOST_SERIALIZATION_NVP(cnt);
	    ar & BOOST_SERIALIZATION_NVP(done);
	    ar & BOOST_SERIALIZATION_NVP(len);
	    ar & BOOST_SERIALIZATION_NVP(end_len);
	    ar & BOOST_SERIALIZATION_NVP(num_msgs);
	    ar & BOOST_SERIALIZATION_NVP(len_inc);
	    ar & BOOST_SERIALIZATION_NVP(user_len_inc);
	    ar & BOOST_SERIALIZATION_NVP(start_time);
	    ar & BOOST_SERIALIZATION_NVP(first_receive);
	    ar & BOOST_SERIALIZATION_NVP(dest);
	    ar & BOOST_SERIALIZATION_NVP(state);
	}

};

#endif // _PINGPONG_PATTERN_H
