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


#ifndef _MSGRATE_PATTERN_H
#define _MSGRATE_PATTERN_H

#include "state_machine.h"
#include "comm_pattern.h"
#include "allreduce.h" 



class Msgrate_pattern : public Comm_pattern    {
    public:
        Msgrate_pattern(ComponentId_t id, Params_t& params) :
            Comm_pattern(id, params)
        {
	    // Defaults for paramters
	    num_msgs= 20;


	    // Process the message rate specific paramaters
            Params_t::iterator it= params.begin();
            while (it != params.end())   {
		if (!it->first.compare("num_msgs"))   {
		    sscanf(it->second.c_str(), "%d", &num_msgs);
		}

                ++it;
            }


	    // Install other state machines which we (msgrate) need as
	    // subroutines.
	    allreduce_msglen= sizeof(double);
	    Allreduce_pattern *a= new Allreduce_pattern(this, allreduce_msglen);
	    SMallreduce= a->install_handler();

	    // Let Comm_pattern know which handler we want to have called
	    // Make sure to call SM_create() last in the main pattern (msgrate)
	    // This is the SM that will run first
	    SMmsgrate= SM->SM_create((void *)this, Msgrate_pattern::wrapper_handle_events);


	    // Kickstart ourselves
	    state_transition(E_START_T1, MSGR_INIT);
        }


	// The Msgrate pattern generator can be in these states and deals
	// with these events.
	typedef enum {MSGR_INIT, MSGR_T1_SENDING, MSGR_T1_RECEIVING} msgrate_state_t;

	// The start event should always be SM_START_EVENT
	typedef enum {E_START_T1= SM_START_EVENT, E_ALLREDUCE_ENTRY, E_ALLREDUCE_EXIT} msgrate_events_t;

    private:

        Msgrate_pattern(const Msgrate_pattern &c);
	void handle_events(state_event sst_event);
	static void wrapper_handle_events(void *obj, state_event sst_event)
	{
	    Msgrate_pattern* mySelf = (Msgrate_pattern*) obj;
	    mySelf->handle_events(sst_event);
	}

	// The states we can be in
	void state_INIT(state_event sm_event);
	void state_T1_SENDING(state_event sm_event);
	void state_T1_RECEIVING(state_event sm_event);

	Params_t params;
	int allreduce_msglen;

	// State machine identifiers
	uint32_t SMmsgrate;
	uint32_t SMallreduce;

	// Some variables we need for msgrate to operate
	msgrate_state_t state;
	int num_msgs;
	int rcv_cnt;
	int done;
	SimTime_t msg_wait_time_start;


	// Serialization
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version )
        {
            ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
	    ar & BOOST_SERIALIZATION_NVP(params);
	    ar & BOOST_SERIALIZATION_NVP(allreduce_msglen);
	    ar & BOOST_SERIALIZATION_NVP(SMmsgrate);
	    ar & BOOST_SERIALIZATION_NVP(SMallreduce);
	    ar & BOOST_SERIALIZATION_NVP(state);
	    ar & BOOST_SERIALIZATION_NVP(num_msgs);
	    ar & BOOST_SERIALIZATION_NVP(rcv_cnt);
	    ar & BOOST_SERIALIZATION_NVP(done);
	    ar & BOOST_SERIALIZATION_NVP(msg_wait_time_start);
        }

        template<class Archive>
        friend void save_construct_data(Archive & ar,
                                        const Msgrate_pattern * t,
                                        const unsigned int file_version)
        {
            _AR_DBG(Msgrate_pattern,"\n");
            ComponentId_t     id     = t->getId();
            Params_t          params = t->params;
            ar << BOOST_SERIALIZATION_NVP(id);
            ar << BOOST_SERIALIZATION_NVP(params);
        }

        template<class Archive>
        friend void load_construct_data(Archive & ar,
                                        Msgrate_pattern * t,
                                        const unsigned int file_version)
        {
            _AR_DBG(Msgrate_pattern,"\n");
            ComponentId_t     id;
            Params_t          params;
            ar >> BOOST_SERIALIZATION_NVP(id);
            ar >> BOOST_SERIALIZATION_NVP(params);
            ::new(t)Msgrate_pattern(id, params);
        }
};

#endif // _MSGRATE_PATTERN_H
