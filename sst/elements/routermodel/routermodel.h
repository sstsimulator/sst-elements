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


#ifndef _ROUTERMODEL_H
#define _ROUTERMODEL_H

#include <sst/core/event.h>
#include <sst/core/component.h>
#include <sst/core/link.h>

using namespace SST;

#define DBG_ROUTER_MODEL 1
#if DBG_ROUTER_MODEL
#define _ROUTER_MODEL_DBG(lvl, fmt, args...)\
    if (router_model_debug >= lvl)   { \
	printf("%d:Routermodel::%s():%d: " fmt, _debug_rank, __FUNCTION__, __LINE__, ## args); \
    }
#else
#define _ROUTER_MODEL_DBG(lvl, fmt, args...)
#endif



#define MAX_LINK_NAME		(16)


class Routermodel : public Component {
    public:
        Routermodel(ComponentId_t id, Params_t& params) :
            Component(id),
            params(params)
        {

            Params_t::iterator it= params.begin();
	    // Defaults
	    router_model_debug= 0;
	    num_ports= -1;
	    hop_delay= 20;
	    router_bw= 1200000000; // Bytes per second
	    congestion_out_cnt= 0;
	    congestion_out= 0;
	    congestion_in_cnt= 0;
	    congestion_in= 0;


            while (it != params.end())   {
                _ROUTER_MODEL_DBG(1, "Router %s: key=%s value=%s\n", component_name.c_str(),
		    it->first.c_str(), it->second.c_str());

		if (!it->first.compare("debug"))   {
		    sscanf(it->second.c_str(), "%d", &router_model_debug);
		}

		if (!it->first.compare("hop_delay"))   {
		    sscanf(it->second.c_str(), "%lud", (uint64_t *)&hop_delay);
		}

		if (!it->first.compare("bw"))   {
		    sscanf(it->second.c_str(), "%lud", (uint64_t *)&router_bw);
		}

		if (!it->first.compare("component_name"))   {
		    component_name= it->second;
		    _ROUTER_MODEL_DBG(1, "Component name for ID %lu is \"%s\"\n", id, component_name.c_str());
		}

		if (!it->first.compare("num_ports"))   {
		    sscanf(it->second.c_str(), "%d", &num_ports);
		}

                ++it;
            }


	    if (num_ports < 1)   {
		_abort(Routermodel, "Need to define the num_ports parameter!\n");
	    }

	    // Create a time converter
	    TimeConverter *tc= registerTimeBase("1ns", true);

	    /* Attach the handler to each port */
	    for (int i= 0; i < num_ports; i++)   {
		port_t new_port;
		char link_name[MAX_LINK_NAME];

		sprintf(link_name, "Link%dname", i);
		it= params.begin();
		while (it != params.end())   {
		    if (!it->first.compare(link_name))   {
			break;
		    }
		    it++;
		}

		if (it != params.end())   {
		    strcpy(link_name, it->second.c_str());
		    new_port.link= configureLink(link_name, new Event::Handler<Routermodel,int>
					(this, &Routermodel::handle_port_events, i));
		    new_port.link->setDefaultTimeBase(tc);
		    _ROUTER_MODEL_DBG(2, "Added handler for port %d, link \"%s\", on router %s\n",
			i, link_name, component_name.c_str());
		} else   {
		    /* Push a dummy port, so port numbering and order in list match */
		    strcpy(link_name, "Unused_port");
		    new_port.link= NULL;
		    _ROUTER_MODEL_DBG(2, "Recorded unused port %d, link \"%s\", on router %s\n",
			i, link_name, component_name.c_str());
		}

		new_port.cnt_in= 0;
		new_port.cnt_out= 0;
		new_port.next_out= 0;
		new_port.next_in= 0;
		port.push_back(new_port);
	    }

	    _ROUTER_MODEL_DBG(1, "Router model component \"%s\" is on rank %d\n",
		component_name.c_str(), _debug_rank);

            // Create a channel for "out of band" events sent to ourselves
	    self_link= configureSelfLink("Me", new Event::Handler<Routermodel>
		    (this, &Routermodel::handle_self_events));
	    if (self_link == NULL)   {
		_ABORT(Ghost_pattern, "That was no good!\n");
	    } else   {
		_ROUTER_MODEL_DBG(2, "Added a self link and a handler on router %s\n",
		    component_name.c_str());
	    }

	    self_link->setDefaultTimeBase(tc);

        }


    private:

        Routermodel(const Routermodel &c);
	void handle_port_events(Event *, int in_port);
	void handle_self_events(Event *);
	Link *initPort(int port, char *link_name);

        Params_t params;

	SimTime_t hop_delay;
	std::string component_name;

	typedef struct port_t   {
	    Link *link;
	    long long int cnt_in;
	    long long int cnt_out;
	    SimTime_t next_out;
	    SimTime_t next_in;
	} port_t;
	std::vector<port_t> port;

	Link *self_link;
	int num_ports;
	int router_model_debug;
	uint64_t router_bw;
	SimTime_t congestion_out;
	long long int congestion_out_cnt;
	SimTime_t congestion_in;
	long long int congestion_in_cnt;


        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version )
        {
            ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
	    ar & BOOST_SERIALIZATION_NVP(params);
	    // FIXME: Do we need this? ar & BOOST_SERIALIZATION_NVP(tc);
        }

        template<class Archive>
        friend void save_construct_data(Archive & ar,
                                        const Routermodel * t,
                                        const unsigned int file_version)
        {
            _AR_DBG(Routermodel,"\n");
            ComponentId_t     id     = t->getId();
            Params_t          params = t->params;
            ar << BOOST_SERIALIZATION_NVP(id);
            ar << BOOST_SERIALIZATION_NVP(params);
        }

        template<class Archive>
        friend void load_construct_data(Archive & ar,
                                        Routermodel * t,
                                        const unsigned int file_version)
        {
            _AR_DBG(Routermodel,"\n");
            ComponentId_t     id;
            Params_t          params;
            ar >> BOOST_SERIALIZATION_NVP(id);
            ar >> BOOST_SERIALIZATION_NVP(params);
            ::new(t)Routermodel(id, params);
        }
};

#endif
