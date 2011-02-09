// Copyright 2009-2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2010, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _MSGRATE_PATTERN_H
#define _MSGRATE_PATTERN_H

#include <sst/core/event.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include "pattern_common.h"

using namespace SST;

#define DBG_MSGRATE_PATTERN 1
#if DBG_MSGRATE_PATTERN
#define _MSGRATE_PATTERN_DBG(lvl, fmt, args...)\
    if (msgrate_pattern_debug >= lvl)   { \
	printf("%d:Msgrate_pattern::%s():%4d: " fmt, _debug_rank, __FUNCTION__, __LINE__, ## args); \
    }
#else
#define _MSGRATE_PATTERN_DBG(lvl, fmt, args...)
#endif

typedef enum {INIT,			// First state in state machine
              SENDING1,			// Sending messages from ranks 0..n/2-1 to n/2..n-1
              SENDING2,			// Rank 0 sends messages to 1..n-1 round robin
              SENDING3,			// Ranks 1..n-1 send messages to 0
	      WAITING1,			// Waiting for messages in the 0..n/2-1 to n/2..n-1 case
	      WAITING2,			// Waiting for messages from rank 0
	      WAITING3,			// Waiting for messages from ranks 1..n-1
	      BCAST1,			// Performing broadcast after first test
	      BCAST2,			// Performing broadcast after second test
	      REDUCE1,			// Performing a reduce to rank 0 after first test
	      REDUCE2,			// Performing a reduce to rank 0 before second test
	      REDUCE3,			// Performing a reduce to rank 0 after second test
	      DONE,			// Work is all done
} state_t;



class Msgrate_pattern : public Component {
    public:
        Msgrate_pattern(ComponentId_t id, Params_t& params) :
            Component(id),
            params(params)
        {

            Params_t::iterator it= params.begin();

	    // Defaults for paramters
	    msgrate_pattern_debug= 0;
	    my_rank= -1;
	    net_latency= 0;
	    net_bandwidth= 0;
	    node_latency= 0;
	    node_bandwidth= 0;
	    exchange_msg_len= 0;
	    envelope_size= 64;
	    cores= -1;
	    num_msgs= 20;

	    // Counters, computed values, and internal state
	    state= INIT;
	    rcv_cnt= 0;
	    application_done= false;
	    bcast_done= false;



	    registerExit();

            while (it != params.end())   {
                _MSGRATE_PATTERN_DBG(2, "[%3d] key \"%s\", value \"%s\"\n", my_rank,
		    it->first.c_str(), it->second.c_str());

		if (!it->first.compare("debug"))   {
		    sscanf(it->second.c_str(), "%d", &msgrate_pattern_debug);
		}

		if (!it->first.compare("rank"))   {
		    sscanf(it->second.c_str(), "%d", &my_rank);
		}

		if (!it->first.compare("x_dim"))   {
		    sscanf(it->second.c_str(), "%d", &x_dim);
		}

		if (!it->first.compare("y_dim"))   {
		    sscanf(it->second.c_str(), "%d", &y_dim);
		}

		if (!it->first.compare("NoC_x_dim"))   {
		    sscanf(it->second.c_str(), "%d", &NoC_x_dim);
		}

		if (!it->first.compare("NoC_y_dim"))   {
		    sscanf(it->second.c_str(), "%d", &NoC_y_dim);
		}

		if (!it->first.compare("cores"))   {
		    sscanf(it->second.c_str(), "%d", &cores);
		}

		if (!it->first.compare("num_msgs"))   {
		    sscanf(it->second.c_str(), "%d", &num_msgs);
		}

		if (!it->first.compare("net_latency"))   {
		    sscanf(it->second.c_str(), "%lu", &net_latency);
		}

		if (!it->first.compare("net_bandwidth"))   {
		    sscanf(it->second.c_str(), "%lu", &net_bandwidth);
		}

		if (!it->first.compare("node_latency"))   {
		    sscanf(it->second.c_str(), "%lu", &node_latency);
		}

		if (!it->first.compare("node_bandwidth"))   {
		    sscanf(it->second.c_str(), "%lu", &node_bandwidth);
		}

		if (!it->first.compare("exchange_msg_len"))   {
		    sscanf(it->second.c_str(), "%d", &exchange_msg_len);
		}

		if (!it->first.compare("envelope_size"))   {
		    sscanf(it->second.c_str(), "%d", &envelope_size);
		}

                ++it;
            }


	    // How many ranks are we running on?
	    nranks= x_dim * y_dim * NoC_x_dim * NoC_y_dim * cores;
	    if (nranks % 2 != 0)   {
		_ABORT(Msgrate_pattern, "Need an even number of ranks (cores)!\n");
	    }

	    // For collectives, who are my children?
	    left= my_rank * 2 + 1;
	    right= my_rank * 2 + 2;
	    if (left >= nranks)   {
		// No child
		left= -1;
	    }
	    if (right >= nranks)   {
		// No child
		right= -1;
	    }

	    if (my_rank == 0)   {
		parent= -1;
	    } else   {
		parent= (my_rank - 1) / 2;
	    }

	    // Create a time converter
	    tc= registerTimeBase("1ns", true);

            // Create a handler for events from the Network
	    net= configureLink("NETWORK", new Event::Handler<Msgrate_pattern>
		    (this, &Msgrate_pattern::handle_net_events));
	    if (net == NULL)   {
		_MSGRATE_PATTERN_DBG(1, "There is no network...\n");
	    } else   {
		net->setDefaultTimeBase(tc);
		_MSGRATE_PATTERN_DBG(2, "[%3d] Added a link and a handler for the Network\n", my_rank);
	    }

            // Create a handler for events from the network on chip (NoC)
	    NoC= configureLink("NoC", new Event::Handler<Msgrate_pattern>
		    (this, &Msgrate_pattern::handle_NoC_events));
	    if (NoC == NULL)   {
		_MSGRATE_PATTERN_DBG(2, "There is no NoC...\n");
	    } else   {
		NoC->setDefaultTimeBase(tc);
		_MSGRATE_PATTERN_DBG(2, "[%3d] Added a link and a handler for the NoC\n", my_rank);
	    }

            // Create a channel for "out of band" events sent to ourselves
	    self_link= configureSelfLink("Me", new Event::Handler<Msgrate_pattern>
		    (this, &Msgrate_pattern::handle_self_events));
	    if (self_link == NULL)   {
		_ABORT(Msgrate_pattern, "That was no good!\n");
	    } else   {
		_MSGRATE_PATTERN_DBG(2, "[%3d] Added a self link and a handler\n", my_rank);
	    }

            // Create a handler for events from local NVRAM
	    nvram= configureLink("NVRAM", new Event::Handler<Msgrate_pattern>
		    (this, &Msgrate_pattern::handle_nvram_events));
	    if (nvram == NULL)   {
		_MSGRATE_PATTERN_DBG(0, "The msgrate pattern generator expects a link named \"NVRAM\"\n");
		_ABORT(Msgrate_pattern, "Check the input XML file!\n");
	    } else   {
		_MSGRATE_PATTERN_DBG(2, "[%3d] Added a link and a handler for the NVRAM\n", my_rank);
	    }

            // Create a handler for events from the storage network
	    storage= configureLink("STORAGE", new Event::Handler<Msgrate_pattern>
		    (this, &Msgrate_pattern::handle_storage_events));
	    if (storage == NULL)   {
		_MSGRATE_PATTERN_DBG(0, "The msgrate pattern generator expects a link named \"STORAGE\"\n");
		_ABORT(Msgrate_pattern, "Check the input XML file!\n");
	    } else   {
		_MSGRATE_PATTERN_DBG(2, "[%3d] Added a link and a handler for the storage\n", my_rank);
	    }

	    self_link->setDefaultTimeBase(tc);
	    nvram->setDefaultTimeBase(tc);
	    storage->setDefaultTimeBase(tc);


	    // Initialize the common functions we need
	    common= new Patterns();
	    if (!common->init(x_dim, y_dim, NoC_x_dim, NoC_y_dim, my_rank, cores, net, self_link,
		    NoC, nvram, storage,
		    net_latency, net_bandwidth, node_latency, node_bandwidth,
		    CHCKPT_NONE, 0, 0, envelope_size))   {
		_ABORT(Msgrate_pattern, "Patterns->init() failed!\n");
	    }

	    // msgrate pattern specific info
	    if (my_rank == 0)   {
		printf("||| Ranks 0...%d will send %d messages to ranks %d...%d\n",
		    nranks / 2 - 1, num_msgs, nranks / 2, nranks - 1);
	    }

	    // Send a start event to ourselves without a delay
	    common->event_send(my_rank, START);

        }

    private:

        Msgrate_pattern(const Msgrate_pattern &c);
	void handle_events(CPUNicEvent *e);
	void handle_net_events(Event *sst_event);
	void handle_NoC_events(Event *sst_event);
	void handle_self_events(Event *sst_event);
	void handle_nvram_events(Event *sst_event);
	void handle_storage_events(Event *sst_event);
	Patterns *common;

	// Input parameters for simulation
	int my_rank;
	int cores;
	int num_msgs;
	int nranks;
	int x_dim;
	int y_dim;
	int NoC_x_dim;
	int NoC_y_dim;
	SimTime_t net_latency;
	SimTime_t net_bandwidth;
	SimTime_t node_latency;
	SimTime_t node_bandwidth;
	int exchange_msg_len;
	int envelope_size;
	int msgrate_pattern_debug;

	// Keeping track of the simulation
	state_t state;
	int rcv_cnt;
	bool application_done;
	bool bcast_done;
	int timestep_needed;
	bool done_waiting;

	// Keeping track of time
	SimTime_t msg_wait_time_start;		// Time when we last entered wait

	// Our connections to SST
        Params_t params;
	Link *net;
	Link *self_link;
	Link *NoC;
	Link *nvram;
	Link *storage;
	TimeConverter *tc;

	// Information and data we need for the collectives
	int left;
	int right;
	int parent;
	std::queue<SimTime_t>reduce_queue;

	// Some local functions we need
	void state_INIT(pattern_event_t event, CPUNicEvent *e);
	void state_SENDING1(pattern_event_t event);
	void state_WAITING1(pattern_event_t event, CPUNicEvent *e);
	void state_SENDING2(pattern_event_t event);
	void state_WAITING2(pattern_event_t event, CPUNicEvent *e);
	void state_SENDING3(pattern_event_t event);
	void state_WAITING3(pattern_event_t event, CPUNicEvent *e);
	void state_BCAST1(pattern_event_t event, CPUNicEvent *e);
	void state_BCAST2(pattern_event_t event, CPUNicEvent *e);
	void state_REDUCE1(pattern_event_t event, CPUNicEvent *e);
	void state_REDUCE2(pattern_event_t event, CPUNicEvent *e);
	void state_REDUCE3(pattern_event_t event, CPUNicEvent *e);
	void state_DONE(pattern_event_t event);

	void count_receives1(void);
	void count_receives2(void);
	void count_receives3(void);


        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version )
        {
            ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
	    ar & BOOST_SERIALIZATION_NVP(params);
	    ar & BOOST_SERIALIZATION_NVP(my_rank);
	    ar & BOOST_SERIALIZATION_NVP(cores);
	    ar & BOOST_SERIALIZATION_NVP(num_msgs);
	    ar & BOOST_SERIALIZATION_NVP(nranks);
	    ar & BOOST_SERIALIZATION_NVP(x_dim);
	    ar & BOOST_SERIALIZATION_NVP(y_dim);
	    ar & BOOST_SERIALIZATION_NVP(NoC_x_dim);
	    ar & BOOST_SERIALIZATION_NVP(NoC_y_dim);
	    ar & BOOST_SERIALIZATION_NVP(net_latency);
	    ar & BOOST_SERIALIZATION_NVP(net_bandwidth);
	    ar & BOOST_SERIALIZATION_NVP(node_latency);
	    ar & BOOST_SERIALIZATION_NVP(node_bandwidth);
	    ar & BOOST_SERIALIZATION_NVP(done_waiting);
	    ar & BOOST_SERIALIZATION_NVP(exchange_msg_len);
	    ar & BOOST_SERIALIZATION_NVP(envelope_size);
	    ar & BOOST_SERIALIZATION_NVP(state);
	    ar & BOOST_SERIALIZATION_NVP(rcv_cnt);
	    ar & BOOST_SERIALIZATION_NVP(msgrate_pattern_debug);
	    ar & BOOST_SERIALIZATION_NVP(application_done);
	    ar & BOOST_SERIALIZATION_NVP(bcast_done);
	    ar & BOOST_SERIALIZATION_NVP(timestep_needed);
	    ar & BOOST_SERIALIZATION_NVP(msg_wait_time_start);
	    ar & BOOST_SERIALIZATION_NVP(net);
	    ar & BOOST_SERIALIZATION_NVP(self_link);
	    ar & BOOST_SERIALIZATION_NVP(tc);
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
