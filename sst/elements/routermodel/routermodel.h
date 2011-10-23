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

#include <stdio.h>
#define __STDC_FORMAT_MACROS		(1)
#include <inttypes.h>			// For PRId64
#include "sst/core/serialization/element.h"
#include <sst/core/element.h>
#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/introspectedComponent.h>
#ifdef WITH_POWER
#include "../power/power.h"
#endif


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


#ifdef WITH_POWER
bool Power::p_hasUpdatedTemp __attribute__((weak));
#endif



// Storage for Router parameters
typedef struct   {
    int index;
    int64_t inflectionpoint;
    int64_t latency;

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
	ar & BOOST_SERIALIZATION_NVP(index);
	ar & BOOST_SERIALIZATION_NVP(inflectionpoint);
	ar & BOOST_SERIALIZATION_NVP(latency);
    }
} Rtrparams_t;



// Sort by inflectionpoint value
static bool
compare_Rtrparams(Rtrparams_t first, Rtrparams_t second)
{
    if (first.inflectionpoint < second.inflectionpoint)   {
	return true;
    } else   {
	return false;
    }
}  // end of compare_Rtrparams






class Routermodel : public IntrospectedComponent {
    public:
        Routermodel(ComponentId_t id, Params_t& params) :
            IntrospectedComponent(id),
            params(params),
            frequency("1ns")
        {
            Params_t::iterator it= params.begin();
	    // Defaults
	    router_model_debug= 0;
	    num_ports= -1;
	    hop_delay= 20;
	    router_bw= 1200000000; // Bytes per second
	    aggregator= false;
	    congestion_out_cnt= 0;
	    congestion_out= 0;
	    congestion_in_cnt= 0;
	    congestion_in= 0;

#ifdef WITH_POWER
	    // Power modeling
	    router_totaldelay= 0;
	    ifModelPower= false;
	    num_local_message= 0;
#endif



            while (it != params.end())   {
                _ROUTER_MODEL_DBG(1, "Router %s: key=%s value=%s\n", component_name.c_str(),
		    it->first.c_str(), it->second.c_str());

		if (!it->first.compare("debug"))   {
		    sscanf(it->second.c_str(), "%d", &router_model_debug);
		}

		else if (!it->first.compare("hop_delay"))   {
		    sscanf(it->second.c_str(), "%" PRIu64, (uint64_t *)&hop_delay);
		}

		else if (!it->first.compare("bw"))   {
		    sscanf(it->second.c_str(), "%" PRIu64, (uint64_t *)&router_bw);
		}

		else if (!it->first.compare("aggregator"))   {
		    int tmp;
		    sscanf(it->second.c_str(), "%d", &tmp);
		    if (tmp != 0)   {
			aggregator= true;
		    }
		}

		else if (!it->first.compare("component_name"))   {
		    component_name= it->second;
		    _ROUTER_MODEL_DBG(1, "Component name for ID %lu is \"%s\"\n", id,
			component_name.c_str());
		}

		else if (!it->first.compare("num_ports"))   {
		    sscanf(it->second.c_str(), "%d", &num_ports);
		}

		// Power modeling paramters
    		else if (!it->first.compare("frequency"))   {
        	    frequency= it->second;
    		}

    		else if (!it->first.compare("push_introspector"))   {
#ifdef WITH_POWER
        	    pushIntrospector= it->second;
#endif
    		}

		else if (!it->first.compare("router_power_model"))   {
#ifdef WITH_POWER
		    if (!it->second.compare("McPAT"))   {
			powerModel= McPAT;
			ifModelPower= true;
			_ROUTER_MODEL_DBG(1, "%s: Power modeling enabled, using McPAT\n",
			    component_name.c_str());
		    } else if (!it->second.compare("ORION"))   {
			powerModel= ORION;
			ifModelPower= true;
			_ROUTER_MODEL_DBG(1, "%s: Power modeling enabled, using ORION\n",
			    component_name.c_str());
		    } else   {
			_abort(Routermodel, "Unknown power model!\n");
		    }
#else
		    _abort(Routermodel, "You can't specify a power model, if you have selected the plain router!");
#endif
    		}

		if (it->first.find("Rtrinflection") != std::string::npos)   {
		    int index;
		    bool found;
		    std::list<Rtrparams_t>::iterator k;

		    if (sscanf(it->first.c_str(), "Rtrinflection%d", &index) != 1)   {
			it++;
			continue;
		    }
		    // Is that index in the list already?
		    found= false;
		    for (k= Rtrparams.begin(); k != Rtrparams.end(); k++)   {
			if (k->index == index)   {
			    // Yes? Update the entry
			    k->inflectionpoint= strtoll(it->second.c_str(), (char **)NULL, 0);
			    found= true;
			}
		    }
		    if (!found)   {
			// No? Create a new entry
			Rtrparams_t another;
			another.inflectionpoint= strtoll(it->second.c_str(), (char **)NULL, 0);
			another.index= index;
			another.latency= -1;
			Rtrparams.push_back(another);
		    }
		}

		if (it->first.find("Rtrlatency") != std::string::npos)   {
		    int index;
		    bool found;
		    std::list<Rtrparams_t>::iterator k;

		    if (sscanf(it->first.c_str(), "Rtrlatency%d", &index) != 1)   {
			it++;
			continue;
		    }
		    // Is that index in the list already?
		    found= false;
		    for (k= Rtrparams.begin(); k != Rtrparams.end(); k++)   {
			if (k->index == index)   {
			    // Yes? Update the entry
			    k->latency= strtoll(it->second.c_str(), (char **)NULL, 0);
			    found= true;
			}
		    }
		    if (!found)   {
			// No? Create a new entry
			Rtrparams_t another;
			another.latency= strtoll(it->second.c_str(), (char **)NULL, 0);
			another.index= index;
			another.inflectionpoint= -1;
			Rtrparams.push_back(another);
		    }
		}


                ++it;
            }


	    if (num_ports < 1)   {
		_abort(Routermodel, "Need to define the num_ports parameter!\n");
	    }

	    // Create a time converter for the NIC simulator.
	    // TimeConverter *tc= registerTimeBase("1ns", true);
	    // FIXME: I'm not sure this is the right way to do it. The router assumes that
	    // the time base is 1ns....

      	    tc= registerTimeBase(frequency, true);

#ifdef WITH_POWER
	    if (!aggregator)   {
		// for power introspection
		if (ifModelPower)   {
		    registerClock(frequency, new Clock::Handler<Routermodel>
			(this, &Routermodel::pushData));
		}
	    }
#endif


	    if (Rtrparams.size() < 2)   {
		new_model= false;

	    } else   {
		std::list<Rtrparams_t>::iterator k;


		_ROUTER_MODEL_DBG(1, "%s: Found %d inflection points. Using new router model.\n",
		    component_name.c_str(), (int)Rtrparams.size());
		new_model= true;
		hop_delay= 0;		// We'll read this out of the router params

		// Check it
		Rtrparams.sort(compare_Rtrparams);
		for (k= Rtrparams.begin(); k != Rtrparams.end(); k++)   {
		    if ((k->inflectionpoint < 0) || (k->latency < 0))   {
			fprintf(stderr, "Invalid inflection point: index %d, len %" PRId64 "d, lat %" PRId64 "d\n",
			    k->index, k->inflectionpoint, k->latency);
			_abort(Routermodel, "Fix xml file!\n");
		    }
		}
	    }


	    /* Attach the handler to each port */
	    for (int i= 0; i < num_ports; i++)   {
		struct port_t new_port;
		char link_name[MAX_LINK_NAME];

		sprintf(link_name, "Link%dname", i);
		it= params.begin();
		while (it != params.end())   {
		    if (!it->first.compare(link_name))   {
			break;
		    }
		    it++;
		}

		if (it == params.end() || !it->second.compare("unused"))   {
		    /* Push a dummy port, so port numbering and order in list match */
		    strcpy(link_name, "Unused_port");
		    new_port.link= NULL;
		    _ROUTER_MODEL_DBG(2, "Recorded unused port %d, link \"%s\", on router %s\n",
			i, link_name, component_name.c_str());
		} else   {
		    strcpy(link_name, it->second.c_str());
		    new_port.link= configureLink(link_name, new Event::Handler<Routermodel,int>
					(this, &Routermodel::handle_port_events, i));
		    new_port.link->setDefaultTimeBase(tc);
		    _ROUTER_MODEL_DBG(2, "Added handler for port %d, link \"%s\", on router %s\n",
			i, link_name, component_name.c_str());
		}

		new_port.cnt_in= 0;
		new_port.cnt_out= 0;
		new_port.next_out= 0;
		new_port.next_in= 0;
		port.push_back(new_port);
	    }

	    if (!aggregator)   {
		_ROUTER_MODEL_DBG(1, "Router model component \"%s\" is on rank %d\n",
		    component_name.c_str(), _debug_rank);
	    } else   {
		_ROUTER_MODEL_DBG(1, "Aggregator component \"%s\" is on rank %d\n",
		    component_name.c_str(), _debug_rank);
	    }

	    if (!aggregator)   {
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

        }


	int
	Setup()
	{
#ifdef WITH_POWER
	    if (!aggregator)   {
		if (ifModelPower)   {
		    power = new Power(getId());

		    //set up floorplan and thermal tiles
		    power->setChip(params);

		    //set up architecture parameters
		    power->setTech(getId(), params, ROUTER, powerModel);

		    //reset all counts to zero
		    power->resetCounts(&mycounts);

		    registerIntrospector(pushIntrospector);
		    registerMonitor("router_delay", new MonitorPointer<SimTime_t>(&router_totaldelay));
		    registerMonitor("local_message", new MonitorPointer<uint64_t>(&num_local_message));
		    registerMonitor("current_power", new MonitorPointer<I>(&pdata.currentPower));
		    registerMonitor("leakage_power", new MonitorPointer<I>(&pdata.leakagePower));
		    registerMonitor("runtime_power", new MonitorPointer<I>(&pdata.runtimeDynamicPower));
		    registerMonitor("total_power", new MonitorPointer<I>(&pdata.totalEnergy));
		    registerMonitor("peak_power", new MonitorPointer<I>(&pdata.peak));
		}
	    }
#endif
	    return 0;
	}


	int
	Finish()
	{
#ifdef WITH_POWER
	    if (!aggregator)   {
		//power->printFloorplanAreaInfo();
		//std::cout << "area return from McPAT = " << power->estimateAreaMcPAT() << " mm^2" << std::endl;
		//power->printFloorplanPowerInfo();
		//power->printFloorplanThermalInfo();
	    }
#endif
	    return 0;
	}


	

    private:


        Routermodel(); // For serialization only
        Routermodel(const Routermodel &c);
	int64_t get_Rtrparams(std::list<Rtrparams_t> params, int64_t msg_len);

        Params_t params;
	void handle_port_events(Event *, int in_port);
	void handle_self_events(Event *);
	Link *initPort(int port, char *link_name);
	std::string frequency;
	TimeConverter *tc;

	SimTime_t hop_delay;
	std::string component_name;

#ifdef WITH_POWER
	// For power & introspection
	std::string pushIntrospector;
  	Pdissipation_t pdata, pstats;
  	Power *power;
	// Over-specified struct that holds usage counts of its sub-components
  	usagecounts_t mycounts;

	pmodel powerModel;
	bool ifModelPower;
	bool pushData(Cycle_t);
#endif

	// totaldelay = congestion delay + generic router delay
  	SimTime_t router_totaldelay;

	//number of intra-core messages
	uint64_t num_local_message;


	struct port_t   {
	    Link *link;
	    long long int cnt_in;
	    long long int cnt_out;
	    SimTime_t next_out;
	    SimTime_t next_in;

	    friend class boost::serialization::access;                                                        
	    template<class Archive>
	    void
	    serialize(Archive & ar, const unsigned int version)
	    {
		ar & BOOST_SERIALIZATION_NVP(link);
		ar & BOOST_SERIALIZATION_NVP(cnt_in);
		ar & BOOST_SERIALIZATION_NVP(cnt_out);
		ar & BOOST_SERIALIZATION_NVP(next_out);
		ar & BOOST_SERIALIZATION_NVP(next_in);
	    }
	};
	std::vector<struct port_t> port;

	Link *self_link;
	int num_ports;
	int router_model_debug;
	uint64_t router_bw;
	bool aggregator;
	std::list<Rtrparams_t> Rtrparams;
	bool new_model;
	SimTime_t congestion_out;
	long long int congestion_out_cnt;
	SimTime_t congestion_in;
	long long int congestion_in_cnt;


        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Component);
	    ar & BOOST_SERIALIZATION_NVP(params);
	    ar & BOOST_SERIALIZATION_NVP(frequency);
	    ar & BOOST_SERIALIZATION_NVP(tc);
	    ar & BOOST_SERIALIZATION_NVP(hop_delay);
	    ar & BOOST_SERIALIZATION_NVP(component_name);
#ifdef NEEDS_MORE_WORK
#ifdef WITH_POWER
	    ar & BOOST_SERIALIZATION_NVP(pushIntrospector);
	    ar & BOOST_SERIALIZATION_NVP(pdata);
	    ar & BOOST_SERIALIZATION_NVP(pstats);
	    ar & BOOST_SERIALIZATION_NVP(power);
	    ar & BOOST_SERIALIZATION_NVP(mycounts);
	    ar & BOOST_SERIALIZATION_NVP(powerModel);
	    ar & BOOST_SERIALIZATION_NVP(ifModelPower);
#endif // WITH_POWER
#endif // NEEDS_MORE_WORK
	    ar & BOOST_SERIALIZATION_NVP(router_totaldelay);
	    ar & BOOST_SERIALIZATION_NVP(num_local_message);
	    ar & BOOST_SERIALIZATION_NVP(port);
	    ar & BOOST_SERIALIZATION_NVP(self_link);
	    ar & BOOST_SERIALIZATION_NVP(num_ports);
	    ar & BOOST_SERIALIZATION_NVP(router_model_debug);
	    ar & BOOST_SERIALIZATION_NVP(router_bw);
	    ar & BOOST_SERIALIZATION_NVP(aggregator);
	    ar & BOOST_SERIALIZATION_NVP(Rtrparams);
	    ar & BOOST_SERIALIZATION_NVP(new_model);
	    ar & BOOST_SERIALIZATION_NVP(congestion_out);
	    ar & BOOST_SERIALIZATION_NVP(congestion_out_cnt);
	    ar & BOOST_SERIALIZATION_NVP(congestion_in);
	    ar & BOOST_SERIALIZATION_NVP(congestion_in_cnt);
        }

};

#endif
