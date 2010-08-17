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


#ifndef _GHOST_PATTERN_H
#define _GHOST_PATTERN_H

#include <sst/core/event.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include "pattern_common.h"

using namespace SST;

#define DBG_GHOST_PATTERN 1
#if DBG_GHOST_PATTERN
#define _GHOST_PATTERN_DBG(lvl, fmt, args...)\
    if (ghost_pattern_debug >= lvl)   { \
	printf("%d:Ghost_pattern::%s():%4d: " fmt, _debug_rank, __FUNCTION__, __LINE__, ## args); \
    }
#else
#define _GHOST_PATTERN_DBG(lvl, fmt, args...)
#endif

typedef enum {INIT, COMPUTE, WAIT, DONE, COORDINATED_CHCKPT} state_t;

class Ghost_pattern : public Component {
    public:
        Ghost_pattern(ComponentId_t id, Params_t& params) :
            Component(id),
            params(params)
        { 

            Params_t::iterator it= params.begin();

	    // Defaults for paramters
	    ghost_pattern_debug= 0;
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
	    chckpt_delay= 0;
	    chckpt_interval= 0;

	    // Counters and computed values
	    execution_time= 0;
	    msg_wait_time= 0;
	    chckpt_time= 0;
	    timestep_cnt= 0;
	    state= INIT;
	    rcv_cnt= 0;
	    application_time_so_far= 0;
	    num_chckpts= 0;
	    chckpt_steps= 1;
	    application_done= FALSE;

	    registerExit();

            while (it != params.end())   {
                _GHOST_PATTERN_DBG(2, "[%3d] key \"%s\", value \"%s\"\n", my_rank,
		    it->first.c_str(), it->second.c_str());

		if (!it->first.compare("debug"))   {
		    sscanf(it->second.c_str(), "%d", &ghost_pattern_debug);
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

		if (!it->first.compare("chckpt_delay"))   {
		    sscanf(it->second.c_str(), "%lu", &chckpt_delay);
		}

		if (!it->first.compare("chckpt_interval"))   {
		    sscanf(it->second.c_str(), "%lu", &chckpt_interval);
		}

                ++it;
            }


            // Create a handler for events
	    net= configureLink("NETWORK", new Event::Handler<Ghost_pattern>
		    (this, &Ghost_pattern::handle_events));
	    if (net == NULL)   {
		_GHOST_PATTERN_DBG(0, "The ghost pattern generator expects a link to the network "
		    "named \"Network\" which is missing!\n");
		_ABORT(Ghost_pattern, "Check the input XML file!\n");
	    } else   {
		_GHOST_PATTERN_DBG(2, "[%3d] Added a link and a handler for the network\n", my_rank);
	    }

            // Create a channel for "out of band" events sent to ourselves
	    self_link= configureSelfLink("Me", new Event::Handler<Ghost_pattern>
		    (this, &Ghost_pattern::handle_self_events));
	    if (self_link == NULL)   {
		_ABORT(Ghost_pattern, "That was no good!\n");
	    } else   {
		_GHOST_PATTERN_DBG(2, "[%3d] Added a self link and a handler\n", my_rank);
	    }

	    // Create a time converter
	    tc= registerTimeBase("1ns", true);
	    net->setDefaultTimeBase(tc);
	    self_link->setDefaultTimeBase(tc);


	    // Initialize the common functions we need
	    common= new Patterns();
	    if (!common->init(x_dim, y_dim, my_rank, cores, net, self_link,
		    net_latency, net_bandwidth, node_latency, node_bandwidth,
		    chckpt_method, chckpt_delay, chckpt_interval))   {
		_ABORT(Ghost_pattern, "Patterns->init() failed!\n");
	    }

	    // Who are my four neighbors?
	    // The network is x_dim * y_dim
	    // The virtual network of the cores is (x_dim * cores) * ( y_dim * cores)
	    int myX= my_rank % (x_dim * cores);
	    int myY= my_rank / (x_dim * cores);
	    right= ((myX + 1) % (x_dim * cores)) + (myY * (x_dim * cores));
	    left= ((myX - 1 + (x_dim * cores)) % (x_dim * cores)) + (myY * (x_dim * cores));
	    down= myX + ((myY + 1) % y_dim) * (x_dim * cores);
	    up= myX + ((myY - 1 + y_dim) % y_dim) * (x_dim * cores);
	    // fprintf(stderr, "{%2d:%d,%d} right %d, left %d, up %d, down %d\n",
	    // 	my_rank, myX, myY, right, left, up, down);

	    // If we are doing coordinated checkpointing, we will do that
	    // every time x timesteps have been computed.
	    // This is OK as long as the checkpoint interval is larger than a time
	    // step. In the other case we would need to quiesce, lets do that later
	    switch (chckpt_method)   {
		case CHCKPT_NONE:
		    break;
		case CHCKPT_COORD:
		    if (chckpt_interval >= compute_time)   {
			chckpt_steps= chckpt_interval / compute_time;
			if (my_rank == 0)   {
			    printf("||| Will checkpoint every %d timesteps = %.9f s\n",
				chckpt_steps,
				(double)chckpt_steps * (double)compute_time / 1000000000.0);
			}
		    } else   {
			_ABORT(Ghost_pattern, "Can't handle checkpoint interval %lu < timestep %lu\n",
			    chckpt_interval, compute_time);
		    }
		    break;
		case CHCKPT_UNCOORD:
		case CHCKPT_RAID:
		    _ABORT(Ghost_pattern, "Can't handle checkpoint methods other than coord and none\n");
		    break;
	    }

	    // Ghost pattern specific info
	    if (my_rank == 0)   {
		printf("||| Each timestep will take %.9f s\n", (double)compute_time / 1000000000.0);
		printf("||| Total application time is %.9f s = %.2f timesteps\n",
		    (double)application_end_time / 1000000000.0,
		    (double)application_end_time / (double)compute_time);
		printf("||| Ghost cell exchange message size is %d bytes\n", exchange_msg_len);
	    }

	    // Send a start event to ourselves without a delay
	    common->event_send(my_rank, START);

        }

    private:

        Ghost_pattern(const Ghost_pattern &c);
	void handle_events(Event *);
	void handle_self_events(Event *);
	Patterns *common;

	int my_rank;
	int cores;
	int x_dim;
	int y_dim;
	SimTime_t net_latency;
	SimTime_t net_bandwidth;
	SimTime_t node_latency;
	SimTime_t node_bandwidth;
	SimTime_t compute_time;
	SimTime_t compute_segment_start;
	SimTime_t application_end_time;
	SimTime_t application_time_so_far;
	SimTime_t execution_time;
	int exchange_msg_len;
	state_t state;
	int left, right, up, down;
	int rcv_cnt;
	int ghost_pattern_debug;
	bool application_done;
	int timestep_cnt;
	chckpt_t chckpt_method;
	SimTime_t chckpt_delay;
	int chckpt_steps;
	SimTime_t chckpt_interval;
	int num_chckpts;
	SimTime_t msg_wait_time_start;
	SimTime_t msg_wait_time;
	SimTime_t chckpt_time;
	SimTime_t chckpt_segment_start;

        Params_t params;
	Link *net;
	Link *self_link;
	TimeConverter *tc;

	// Some local functions we need
	void handle_start(void);
	void handle_compute_done(void);
	void handle_receive(int hops);
	void handle_chckpt_done(void);
	void handle_fail(void);

        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version )
        {
            ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
	    ar & BOOST_SERIALIZATION_NVP(params);
	    ar & BOOST_SERIALIZATION_NVP(my_rank);
	    ar & BOOST_SERIALIZATION_NVP(cores);
	    ar & BOOST_SERIALIZATION_NVP(x_dim);
	    ar & BOOST_SERIALIZATION_NVP(y_dim);
	    ar & BOOST_SERIALIZATION_NVP(net_latency);
	    ar & BOOST_SERIALIZATION_NVP(net_bandwidth);
	    ar & BOOST_SERIALIZATION_NVP(node_latency);
	    ar & BOOST_SERIALIZATION_NVP(node_bandwidth);
	    ar & BOOST_SERIALIZATION_NVP(compute_time);
	    ar & BOOST_SERIALIZATION_NVP(compute_segment_start);
	    ar & BOOST_SERIALIZATION_NVP(application_end_time);
	    ar & BOOST_SERIALIZATION_NVP(application_time_so_far);
	    ar & BOOST_SERIALIZATION_NVP(execution_time);
	    ar & BOOST_SERIALIZATION_NVP(exchange_msg_len);
	    ar & BOOST_SERIALIZATION_NVP(state);
	    ar & BOOST_SERIALIZATION_NVP(left);
	    ar & BOOST_SERIALIZATION_NVP(right);
	    ar & BOOST_SERIALIZATION_NVP(up);
	    ar & BOOST_SERIALIZATION_NVP(down);
	    ar & BOOST_SERIALIZATION_NVP(rcv_cnt);
	    ar & BOOST_SERIALIZATION_NVP(ghost_pattern_debug);
	    ar & BOOST_SERIALIZATION_NVP(application_done);
	    ar & BOOST_SERIALIZATION_NVP(timestep_cnt);
	    ar & BOOST_SERIALIZATION_NVP(chckpt_method);
	    ar & BOOST_SERIALIZATION_NVP(chckpt_delay);
	    ar & BOOST_SERIALIZATION_NVP(chckpt_steps);
	    ar & BOOST_SERIALIZATION_NVP(chckpt_interval);
	    ar & BOOST_SERIALIZATION_NVP(num_chckpts);
	    ar & BOOST_SERIALIZATION_NVP(msg_wait_time_start);
	    ar & BOOST_SERIALIZATION_NVP(msg_wait_time);
	    ar & BOOST_SERIALIZATION_NVP(chckpt_time);
	    ar & BOOST_SERIALIZATION_NVP(chckpt_segment_start);
	    ar & BOOST_SERIALIZATION_NVP(net);
	    ar & BOOST_SERIALIZATION_NVP(self_link);
	    ar & BOOST_SERIALIZATION_NVP(tc);
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
