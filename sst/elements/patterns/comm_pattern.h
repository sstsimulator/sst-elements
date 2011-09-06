//
// Copyright (c) 2011, IBM Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _COMM_PATTERN_H
#define _COMM_PATTERN_H

#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/cpunicEvent.h>
#include "state_machine.h"
#include "pattern_common.h"

using namespace SST;

#define DBG_COMM_PATTERN 1
#if DBG_COMM_PATTERN
#define _COMM_PATTERN_DBG(lvl, fmt, args...)\
    if (comm_pattern_debug >= lvl)   { \
	printf("%d:Comm_pattern::%s():%4d: " fmt, _debug_rank, __FUNCTION__, __LINE__, ## args); \
    }
#else
#define _COMM_PATTERN_DBG(lvl, fmt, args...)
#endif


class Comm_pattern : public Component {
    public:
	// The constructor
        Comm_pattern(ComponentId_t id, Params_t& params) :
	    // constructor initializer list
            Component(id), params(params)
        {

	    // Some defaults
	    comm_pattern_debug= 0;
	    my_rank= -1;



	    registerExit();

	    Params_t::iterator it= params.begin();
	    while (it != params.end())   {
		if (!it->first.compare("debug"))   {
		    sscanf(it->second.c_str(), "%d", &comm_pattern_debug);
		}

		if (!it->first.compare("rank"))   {
		    sscanf(it->second.c_str(), "%d", &my_rank);
		}

		it++;
	    }

	    // FIXME: Do some input checking here; e.g., cores and nodes must be > 0

	    // Interface with SST

	    // Create a time converter
	    tc= registerTimeBase(TIME_BASE, true);

            // Create a handler for events from the Network
	    net= configureLink("NETWORK", new Event::Handler<Comm_pattern>
		    (this, &Comm_pattern::handle_net_events));
	    if (net == NULL)   {
		_COMM_PATTERN_DBG(1, "There is no network... Single node!\n");
	    } else   {
		net->setDefaultTimeBase(tc);
		_COMM_PATTERN_DBG(2, "[%3d] Added a link and a handler for the Network\n", my_rank);
	    }

            // Create a handler for events from the network on chip (NoC)
	    NoC= configureLink("NoC", new Event::Handler<Comm_pattern>
		    (this, &Comm_pattern::handle_NoC_events));
	    if (NoC == NULL)   {
		_COMM_PATTERN_DBG(1, "There is no NoC...\n");
	    } else   {
		NoC->setDefaultTimeBase(tc);
		_COMM_PATTERN_DBG(2, "[%3d] Added a link and a handler for the NoC\n", my_rank);
	    }

            // Create a channel for "out of band" events sent to ourselves
	    self_link= configureSelfLink("Me", new Event::Handler<Comm_pattern>
		    (this, &Comm_pattern::handle_self_events));
	    if (self_link == NULL)   {
		_ABORT(Comm_pattern, "That was no good!\n");
	    } else   {
		_COMM_PATTERN_DBG(2, "[%3d] Added a self link and a handler\n", my_rank);
	    }

            // Create a handler for events from local NVRAM
	    nvram= configureLink("NVRAM", new Event::Handler<Comm_pattern>
		    (this, &Comm_pattern::handle_nvram_events));
	    if (nvram == NULL)   {
		_COMM_PATTERN_DBG(0, "The Comm pattern generator expects a link named \"NVRAM\"\n");
		_ABORT(Comm_pattern, "Check the input XML file!\n");
	    } else   {
		_COMM_PATTERN_DBG(2, "[%3d] Added a link and a handler for the NVRAM\n", my_rank);
	    }

            // Create a handler for events from the storage network
	    storage= configureLink("STORAGE", new Event::Handler<Comm_pattern>
		    (this, &Comm_pattern::handle_storage_events));
	    if (storage == NULL)   {
		_COMM_PATTERN_DBG(0, "The Comm pattern generator expects a link named \"STORAGE\"\n");
		_ABORT(Comm_pattern, "Check the input XML file!\n");
	    } else   {
		_COMM_PATTERN_DBG(2, "[%3d] Added a link and a handler for the storage\n", my_rank);
	    }

	    self_link->setDefaultTimeBase(tc);
	    nvram->setDefaultTimeBase(tc);
	    storage->setDefaultTimeBase(tc);

	    // Initialize the common functions we need
	    common= new Patterns();
	    if (!common->init(params, net, self_link, NoC, nvram, storage))   {
		_ABORT(Comm_pattern, "Patterns->init() failed!\n");
	    }

	    num_ranks= common->get_total_cores();
	    SM= new State_machine(my_rank);

        }  // Done with initialization

        ~Comm_pattern()
	{delete common;}


	int my_rank;
	int num_ranks;

	int myNetX(void);
	int myNetY(void);
	int NetWidth(void);
	int NetHeight(void);
	int NoCWidth(void);
	int NoCHeight(void);
	int NumCores(void);
	int myNoCX(void);
	int myNoCY(void);

	void self_event_send(int event_type);
	void send_msg(int dest, int len, state_event sm_event);

	State_machine *SM;

    private:

        Comm_pattern(const Comm_pattern &c);
	void handle_sst_events(CPUNicEvent *sst_event, const char *err_str);
	void handle_net_events(Event *sst_event);
	void handle_NoC_events(Event *sst_event);
	void handle_self_events(Event *sst_event);
	void handle_nvram_events(Event *sst_event);
	void handle_storage_events(Event *sst_event);


	// Input paramters for simulation
	Patterns *common;
	int comm_pattern_debug;



	// Interfacing with SST
        Params_t params;
	Link *net;
	Link *self_link;
	Link *NoC;
	Link *nvram;
	Link *storage;
	TimeConverter *tc;

        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version )
        {
            ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
	    ar & BOOST_SERIALIZATION_NVP(params);
	    ar & BOOST_SERIALIZATION_NVP(my_rank);
	    ar & BOOST_SERIALIZATION_NVP(comm_pattern_debug);
        }

        template<class Archive>
        friend void save_construct_data(Archive & ar,
                                        const Comm_pattern * t,
                                        const unsigned int file_version)
        {
            _AR_DBG(Comm_pattern,"\n");
            ComponentId_t     id     = t->getId();
            Params_t          params = t->params;
            ar << BOOST_SERIALIZATION_NVP(id);
            ar << BOOST_SERIALIZATION_NVP(params);
        }

        template<class Archive>
        friend void load_construct_data(Archive & ar,
                                        Comm_pattern * t,
                                        const unsigned int file_version)
        {
            _AR_DBG(Comm_pattern,"\n");
            ComponentId_t     id;
            Params_t          params;
            ar >> BOOST_SERIALIZATION_NVP(id);
            ar >> BOOST_SERIALIZATION_NVP(params);
            ::new(t)Comm_pattern(id, params);
        }
};

#endif // _COMM_PATTERN_H
