//
// Copyright (c) 2011, IBM Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _PINGPONG_PATTERN_H
#define _PINGPONG_PATTERN_H

#include "state_machine.h"
#include "comm_pattern.h"
#include "barrier.h"
#include "allreduce.h"



class Pingpong_pattern : public Comm_pattern {
    public:
        Pingpong_pattern(ComponentId_t id, Params_t& params) :
	    // constructor initializer list                                                                   
	    Comm_pattern(id, params)
	{

	    // Messages are exchanged between rank 0 and "dest"
	    // The default for "dest" is to place it as far away
	    // as possible in the (logical) torus created by the
	    // Comm_pattern object
	    dest= NetWidth() * NetHeight() * NoCWidth() * NoCHeight() * NumCores() / 2;

	    // Set some more defaults
	    num_msg= 10;
	    end_len= 1024;
	    len_inc= 8;



	    // Process the ping/pong pattern specific parameters
	    Params_t::iterator it= params.begin();

	    while (it != params.end())   {
		if (!it->first.compare("destination"))   {
		    sscanf(it->second.c_str(), "%d", &dest);
		}

		if (!it->first.compare("num_msg"))   {
		    sscanf(it->second.c_str(), "%d", &num_msg);
		}

		if (!it->first.compare("end_len"))   {
		    sscanf(it->second.c_str(), "%d", &end_len);
		}

		if (!it->first.compare("len_inc"))   {
		    sscanf(it->second.c_str(), "%d", &len_inc);
		}

		it++;
	    }

	    // Install other state machines which we (pingpong) need as
	    // subroutines.
	    // For pingpong we don't really need them, but we include
	    // them here as an example, and for testing the gate keeper
	    // switiching mechanism among state machines
	    Barrier_pattern *b= new Barrier_pattern(this);
	    SMbarrier= b->install_handler();

	    Allreduce_pattern *a= new Allreduce_pattern(this);
	    SMallreduce= a->install_handler();

	    // Let Comm_pattern know which handler we want to have called
	    // Make sure to call SM_create() last in the main pattern
	    // This is the SM that will run first
	    SMpingpong= SM->SM_create((void *)this, Pingpong_pattern::wrapper_handle_events);

	    // Kickstart ourselves
	    state_transition(E_START, PP_INIT);
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
	Pingpong_pattern(const Pingpong_pattern &c);
	void handle_events(state_event sst_event);
	static void wrapper_handle_events(void *obj, state_event sst_event)
	{
	    Pingpong_pattern* mySelf = (Pingpong_pattern*) obj;
	    mySelf->handle_events(sst_event);
	}

	void state_INIT(pingpong_events_t event);
	void state_RECEIVING(pingpong_events_t event);
	void state_BARRIER(pingpong_events_t event);
	void state_ALLREDUCE(state_event event);
	Params_t params;

	// State machine identifiers
	uint32_t SMpingpong;
	uint32_t SMbarrier;
	uint32_t SMallreduce;

	// Some variables we need for pingpong to operate
	int cnt;
	int done;
	int len;
	int end_len;
	int num_msg;
	int len_inc;
	SimTime_t start_time;
	int first_receive;
	int dest;
	pingpong_state_t state;

        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version )
        {
            ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
	    ar & BOOST_SERIALIZATION_NVP(params);
	    ar & BOOST_SERIALIZATION_NVP(SMpingpong);
	    ar & BOOST_SERIALIZATION_NVP(SMbarrier);
	    ar & BOOST_SERIALIZATION_NVP(cnt);
	    ar & BOOST_SERIALIZATION_NVP(done);
	    ar & BOOST_SERIALIZATION_NVP(len);
	    ar & BOOST_SERIALIZATION_NVP(end_len);
	    ar & BOOST_SERIALIZATION_NVP(num_msg);
	    ar & BOOST_SERIALIZATION_NVP(len_inc);
	    ar & BOOST_SERIALIZATION_NVP(start_time);
	    ar & BOOST_SERIALIZATION_NVP(first_receive);
	    ar & BOOST_SERIALIZATION_NVP(dest);
	    ar & BOOST_SERIALIZATION_NVP(state);
	}

        template<class Archive>
        friend void save_construct_data(Archive & ar,
                                        const Pingpong_pattern * t,
                                        const unsigned int file_version)
        {
            _AR_DBG(Pingpong_pattern,"\n");
            ComponentId_t     id     = t->getId();
            Params_t          params = t->params;
            ar << BOOST_SERIALIZATION_NVP(id);
            ar << BOOST_SERIALIZATION_NVP(params);
        }

        template<class Archive>
        friend void load_construct_data(Archive & ar,
                                        Pingpong_pattern * t,
                                        const unsigned int file_version)
        {
            _AR_DBG(Pingpong_pattern,"\n");
            ComponentId_t     id;
            Params_t          params;
            ar >> BOOST_SERIALIZATION_NVP(id);
            ar >> BOOST_SERIALIZATION_NVP(params);
            ::new(t)Pingpong_pattern(id, params);
        }

};

#endif // _PINGPONG_PATTERN_H
