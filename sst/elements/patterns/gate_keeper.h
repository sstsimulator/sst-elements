//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _GATE_KEEPER_H
#define _GATE_KEEPER_H

#include <sst/core/event.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include "pattern_common.h"

using namespace SST;

#define DBG_GATE_KEEPER 1
#if DBG_GATE_KEEPER
#define _GATE_KEEPER_DBG(lvl, fmt, args...)\
    if (gate_keeper_debug >= lvl)   { \
	printf("%d:Gate_keeper::%s():%4d: " fmt, _debug_rank, __FUNCTION__, __LINE__, ## args); \
    }
#else
#define _GATE_KEEPER_DBG(lvl, fmt, args...)
#endif



class Gate_keeper : public Component {
    public:
        Gate_keeper(ComponentId_t id, Params_t& params) :
            Component(id),
            params(params)
        {

            Params_t::iterator it= params.begin();

	    // Defaults for paramters
	    gate_keeper_debug= 0;
	    my_rank= -1;
	    net_latency= 0;
	    net_bandwidth= 0;
	    node_latency= 0;
	    node_bandwidth= 0;
	    exchange_msg_len= 0;
	    envelope_size= 64;
	    cores= -1;
	    num_msgs= 20;


	    registerExit();

            while (it != params.end())   {
                _GATE_KEEPER_DBG(2, "[%3d] key \"%s\", value \"%s\"\n", my_rank,
		    it->first.c_str(), it->second.c_str());

		if (!it->first.compare("debug"))   {
		    sscanf(it->second.c_str(), "%d", &gate_keeper_debug);
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
		_ABORT(Gate_keeper, "Need an even number of ranks (cores)!\n");
	    }

	    // Create a time converter
	    tc= registerTimeBase("1ns", true);

            // Create a handler for events from the Network
	    net= configureLink("NETWORK", new Event::Handler<Gate_keeper>
		    (this, &Gate_keeper::handle_net_events));
	    if (net == NULL)   {
		_GATE_KEEPER_DBG(1, "There is no network...\n");
	    } else   {
		net->setDefaultTimeBase(tc);
		_GATE_KEEPER_DBG(2, "[%3d] Added a link and a handler for the Network\n", my_rank);
	    }

            // Create a handler for events from the network on chip (NoC)
	    NoC= configureLink("NoC", new Event::Handler<Gate_keeper>
		    (this, &Gate_keeper::handle_NoC_events));
	    if (NoC == NULL)   {
		_GATE_KEEPER_DBG(2, "There is no NoC...\n");
	    } else   {
		NoC->setDefaultTimeBase(tc);
		_GATE_KEEPER_DBG(2, "[%3d] Added a link and a handler for the NoC\n", my_rank);
	    }

            // Create a channel for "out of band" events sent to ourselves
	    self_link= configureSelfLink("Me", new Event::Handler<Gate_keeper>
		    (this, &Gate_keeper::handle_self_events));
	    if (self_link == NULL)   {
		_ABORT(Gate_keeper, "That was no good!\n");
	    } else   {
		_GATE_KEEPER_DBG(2, "[%3d] Added a self link and a handler\n", my_rank);
	    }

            // Create a handler for events from local NVRAM
	    nvram= configureLink("NVRAM", new Event::Handler<Gate_keeper>
		    (this, &Gate_keeper::handle_nvram_events));
	    if (nvram == NULL)   {
		_GATE_KEEPER_DBG(0, "The msgrate pattern generator expects a link named \"NVRAM\"\n");
		_ABORT(Gate_keeper, "Check the input XML file!\n");
	    } else   {
		_GATE_KEEPER_DBG(2, "[%3d] Added a link and a handler for the NVRAM\n", my_rank);
	    }

            // Create a handler for events from the storage network
	    storage= configureLink("STORAGE", new Event::Handler<Gate_keeper>
		    (this, &Gate_keeper::handle_storage_events));
	    if (storage == NULL)   {
		_GATE_KEEPER_DBG(0, "The msgrate pattern generator expects a link named \"STORAGE\"\n");
		_ABORT(Gate_keeper, "Check the input XML file!\n");
	    } else   {
		_GATE_KEEPER_DBG(2, "[%3d] Added a link and a handler for the storage\n", my_rank);
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
		_ABORT(Gate_keeper, "Patterns->init() failed!\n");
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

        Gate_keeper(const Gate_keeper &c);
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
	int gate_keeper_debug;

	// Our connections to SST
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
	    ar & BOOST_SERIALIZATION_NVP(exchange_msg_len);
	    ar & BOOST_SERIALIZATION_NVP(envelope_size);
	    ar & BOOST_SERIALIZATION_NVP(gate_keeper_debug);
	    ar & BOOST_SERIALIZATION_NVP(params);
	    ar & BOOST_SERIALIZATION_NVP(net);
	    ar & BOOST_SERIALIZATION_NVP(self_link);
	    ar & BOOST_SERIALIZATION_NVP(NoC);
	    ar & BOOST_SERIALIZATION_NVP(nvram);
	    ar & BOOST_SERIALIZATION_NVP(storage);
	    ar & BOOST_SERIALIZATION_NVP(tc);
        }

        template<class Archive>
        friend void save_construct_data(Archive & ar,
                                        const Gate_keeper * t,
                                        const unsigned int file_version)
        {
            _AR_DBG(Gate_keeper,"\n");
            ComponentId_t     id     = t->getId();
            Params_t          params = t->params;
            ar << BOOST_SERIALIZATION_NVP(id);
            ar << BOOST_SERIALIZATION_NVP(params);
        }

        template<class Archive>
        friend void load_construct_data(Archive & ar,
                                        Gate_keeper * t,
                                        const unsigned int file_version)
        {
            _AR_DBG(Gate_keeper,"\n");
            ComponentId_t     id;
            Params_t          params;
            ar >> BOOST_SERIALIZATION_NVP(id);
            ar >> BOOST_SERIALIZATION_NVP(params);
            ::new(t)Gate_keeper(id, params);
        }
};

#endif // _GATE_KEEPER_H
