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
	    net_latency= 0;
	    net_bandwidth= 0;
	    node_latency= 0;
	    node_bandwidth= 0;
	    compute_time= 0;
	    application_end_time= 0;
	    exchange_msg_len= 128;
	    cores= -1;
	    chckpt_method= CHCKPT_NONE;
	    chckpt_interval= 0;
	    envelope_size= 64;
	    chckpt_size= 0;
	    msg_write_time= 0;



	    registerExit();

	    Params_t::iterator it= params.begin();
	    while (it != params.end())   {
		if (!it->first.compare("debug"))   {
		    sscanf(it->second.c_str(), "%d", &comm_pattern_debug);
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

		if (!it->first.compare("compute_time"))   {
		    sscanf(it->second.c_str(), "%lu", &compute_time);
		}

		if (!it->first.compare("application_end_time"))   {
		    sscanf(it->second.c_str(), "%lu", &application_end_time);
		}

		if (!it->first.compare("exchange_msg_len"))   {
		    sscanf(it->second.c_str(), "%d", &exchange_msg_len);
		}

		if (!it->first.compare("chckpt_method"))   {
		    if (!it->second.compare("none"))   {
			chckpt_method= CHCKPT_NONE;
		    } else if (!it->second.compare("coordinated"))   {
			chckpt_method= CHCKPT_COORD;
		    } else if (!it->second.compare("uncoordinated"))   {
			chckpt_method= CHCKPT_UNCOORD;
		    } else if (!it->second.compare("distributed"))   {
			chckpt_method= CHCKPT_RAID;
		    }
		}

		if (!it->first.compare("chckpt_interval"))   {
		    sscanf(it->second.c_str(), "%lu", &chckpt_interval);
		}

		if (!it->first.compare("envelope_size"))   {
		    sscanf(it->second.c_str(), "%d", &envelope_size);
		}

		if (!it->first.compare("msg_write_time"))   {
		    sscanf(it->second.c_str(), "%lu", &msg_write_time);
		}

		if (!it->first.compare("chckpt_size"))   {
		    sscanf(it->second.c_str(), "%d", &chckpt_size);
		}

		it++;
	    }

	    // Interface with SST

	    // Create a time converter
	    tc= registerTimeBase("1ns", true);

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
	    if (!common->init(x_dim, y_dim, NoC_x_dim, NoC_y_dim, my_rank, cores, net, self_link,
		    NoC, nvram, storage,
		    net_latency, net_bandwidth, node_latency, node_bandwidth,
		    chckpt_method, chckpt_size, chckpt_interval, envelope_size))   {
		_ABORT(Comm_pattern, "Patterns->init() failed!\n");
	    }

	    // Who are my four neighbors?
	    // The network is x_dim * NoC_x_dim by y_dim * NoC_y_dim
	    // The virtual network of the cores is
	    // (x_dim * NoC_x_dim * cores) * (y_dim * NoC_y_dim)
	    int logical_width= x_dim * NoC_x_dim * cores;
	    int logical_height= y_dim * NoC_y_dim;
	    int myX= my_rank % logical_width;
	    int myY= my_rank / logical_width;
	    num_ranks= x_dim * y_dim * NoC_x_dim * NoC_y_dim * cores;

	    if (my_rank == 0)   {
		printf("#  |||  Arranging %d ranks as a %d * %d logical mesh\n",
		    num_ranks, logical_width, logical_height);
	    }

	    right= ((myX + 1) % (logical_width)) + (myY * (logical_width));
	    left= ((myX - 1 + (logical_width)) % (logical_width)) + (myY * (logical_width));
	    down= myX + ((myY + 1) % logical_height) * (logical_width);
	    up= myX + ((myY - 1 + logical_height) % logical_height) * (logical_width);

        }  // Done with initialization

        ~Comm_pattern()
	{ }

	int my_rank;
	int num_ranks;

	uint32_t SM_create(void *obj, void (*handler)(void *obj, int event));
	void SM_transition(uint32_t machineID, int start_event);
	void SM_return(void);

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
	// FIXME: This needs to be more generic as well
	void data_send(int dest, int len, int event_type);

    private:

        Comm_pattern(const Comm_pattern &c);
	void handle_events(CPUNicEvent *e);
	void handle_net_events(Event *sst_event);
	void handle_NoC_events(Event *sst_event);
	void handle_self_events(Event *sst_event);
	void handle_nvram_events(Event *sst_event);
	void handle_storage_events(Event *sst_event);

	typedef struct SM_t   {
	    void (*handler)(void *obj, int event);
	    void *obj;
	    uint32_t tag;
	    std::vector <CPUNicEvent *>missed_events;
	} SM_t;
	std::vector <SM_t>SM;
	std::vector <uint32_t>SMstack;

	int currentSM;
	uint32_t maxSM;

	// Input paramters for simulation
	Patterns *common;
	int comm_pattern_debug;

	int cores;
	int x_dim;
	int y_dim;
	int NoC_x_dim;
	int NoC_y_dim;
	SimTime_t net_latency;
	SimTime_t net_bandwidth;
	SimTime_t node_latency;
	SimTime_t node_bandwidth;
	SimTime_t compute_time;
	SimTime_t application_end_time;
	int exchange_msg_len;
	chckpt_t chckpt_method;
	SimTime_t chckpt_interval;
	int chckpt_size;
	int envelope_size;
	SimTime_t msg_write_time;

	// Precomputed values
	int left, right, up, down;
	int chckpt_steps;

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
