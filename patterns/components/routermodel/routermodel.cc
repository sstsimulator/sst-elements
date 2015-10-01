// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <sst_config.h>
#include <sst/core/serialization.h>
#include "routermodel.h"

#include <assert.h>

#include "sst/core/element.h"
#include <cpunicEvent.h>

// The lower 27 bits of the event ID are the rank number, high order bits are sequence
#define RANK_FIELD              (27)



void
Routermodel::handle_port_events(Event *event, int in_port)
{

SimTime_t current_time;
SimTime_t delay = 0;
SimTime_t link_time;
SimTime_t blocked;
int out_port;


    // Check for routing algorithm problems
    assert((in_port >= 0) && (in_port < num_ports));

    current_time= getCurrentSimTime();
    CPUNicEvent *e= static_cast<CPUNicEvent *>(event);

    _ROUTER_MODEL_DBG(4, "%s in port %d, time %" PRIu64 ", src %" PRId64 ", seq %" PRId64 "\n",
	component_name.c_str(), in_port, (uint64_t)current_time,
	e->msg_id & ((1 << RANK_FIELD) - 1), e->msg_id >> RANK_FIELD);


#if DBG_ROUTER_MODEL > 1
    /* Diagnostic: print the route this event is taking */
    if (router_model_debug >= 5)   {
	std::vector<int>::iterator itNum;
	char str[132];
	char tmp[132];
	int i= 0;

	sprintf(str, "%s event route: ", component_name.c_str());
	for(itNum = e->route.begin(); itNum < e->route.end(); itNum++)   {
	    if (i == e->hops)   {
		sprintf(tmp, "%s%c", str, '[');
		strcpy(str, tmp);
	    }
	    // str= str + boost::lexical_cast<std::string>(*itNum);
	    sprintf(tmp, "%s%d", str, *itNum);
	    strcpy(str, tmp);
	    if (i == e->hops)   {
		sprintf(tmp, "%s%c", str, ']');
		strcpy(str, tmp);
	    }
	    sprintf(tmp, "%s%c", str, ' ');
	    strcpy(str, tmp);
	    i++;
	}
	_ROUTER_MODEL_DBG(5, "%s\n", str);
    }
#endif  // DBG_ROUTER_MODEL

    // Where are we going?
    out_port= e->route[e->hops];
    assert((out_port >= 0) && (out_port < num_ports));

    if (aggregator)   {
	// We're not really a router, rather we are being used as an aggregator
	// Just send the event on: no delays, no queuing
	e->hops++;
	assert(port[out_port].link); // Trying to use an unused port. This is a routing error
	port[out_port].link->send(0, e); 
	return;
    }

    if (port[in_port].next_in > current_time)   {
	SimTime_t arrival_delay;

	// If the input port is in use right now, then this message actually
	// wont come in until later
	arrival_delay= port[in_port].next_in - current_time;
	port[in_port].next_in= current_time + arrival_delay;

	// FIXME: I am not sure these are meaningful statistics
	congestion_in_cnt++;
	congestion_in += arrival_delay;
	e->congestion_cnt++;
	e->congestion_delay += arrival_delay;

	e->entry_port= in_port;
	assert(self_link); // Trying to use an unused port. This is a routing error
	self_link->send(arrival_delay, e); 

	return;
    }

    // We'll take it now
    port[in_port].cnt_in++;


    // Update total usage counts of all ports for power
    if (!e->local_traffic)   {
#ifdef WITH_POWER
	mycounts.router_access++;
#endif
    } else   {
	num_local_message++;
    }


    // What is the current delay to send on this output port?
    if (!new_model)   {
	if (port[out_port].next_out <= current_time)   {
	    // No output port delays.  We can send as soon as the message arrives;
	    delay= 0;
	    blocked= 0;

	} else   {
	    // Busy right now
	    blocked= port[out_port].next_out - current_time;
	    congestion_out_cnt++;
	    congestion_out += blocked;
	    e->congestion_cnt++;
	    e->congestion_delay += blocked;
	}

	// Add in the generic router delay
	delay += hop_delay;

	// For introspection (router_delay)
	router_totaldelay= router_totaldelay + e->congestion_delay + delay;

	// How long will this message occupy the input and output port?
	// FIXME: The constant 1000000000 should be replaced with our time base
	link_time= ((uint64_t)e->msg_len * 1000000000) / router_bw;

    } else   {

	// Calculate when this message can leave the router port
	delay= get_Rtrparams(Rtrparams, e->msg_len);
	link_time= get_Rtrparams(NICparams, e->msg_len);
	if (current_time >= port[out_port].next_out)   {
	    // Port is not busy
	    blocked= 0.0;

	} else   {
	    // Port is busy
	    blocked= port[out_port].next_out - current_time ;

	    congestion_out_cnt++;
	    congestion_out += blocked;
	    e->congestion_cnt++;
	    e->congestion_delay += blocked;
	}
    }

    // When can these ports be used again?
    port[out_port].next_out= current_time + blocked + delay + link_time;
    port[in_port].next_in= current_time + blocked + link_time;

    e->hops++;
    assert(port[out_port].link); // Trying to use an unused port. This is a routing error
    port[out_port].link->send(delay + blocked, e); 
    port[out_port].cnt_out++;
    msg_cnt++;

}  // end of handle_port_events()



// When we send to ourselves, we come here.
// Just pass it on to the main handler above
void
Routermodel::handle_self_events(Event *event)
{

    CPUNicEvent *e= static_cast<CPUNicEvent *>(event);

    if (e->entry_port < 0)   {
	out.fatal(CALL_INFO, -1, "Internal error: entry port not defined!\n");
    }

    handle_port_events(e, e->entry_port);

}  /* end of handle_self_events() */



#ifdef WITH_POWER
// Get and push power at a frequency determinedby the push_introspector
bool
Routermodel::pushData(Cycle_t current)
{
    if (isTimeToPush(current, pushIntrospector.c_str()))   {
       // Here you can push power statistics by 1) set up values in the mycounts structure
       // and 2) call the gerPower function. See cpu_PowerAndData for example
	
       // set up counts
       // set up router-related counts (in this case, this is done in handle_port_event)
       // mycounts.router_access=1;
       // std::cout << " It is time (" <<current << ") to push power, router_delay = " << router_totaldelay << " and router_access = " << mycounts.router_access << std::endl;
       pdata= power->getPower(this, ROUTER, mycounts);
       power->compute_temperature(getId());
       regPowerStats(pdata);

       //reset all counts to zero for next power query
       power->resetCounts(&mycounts);
	
#if 0
	using namespace io_interval; std::cout <<"ID " << getId() <<": current total power = " << pdata.currentPower << " W" << std::endl;
	using namespace io_interval; std::cout <<"ID " << getId() <<": leakage power = " << pdata.leakagePower << " W" << std::endl;
	using namespace io_interval; std::cout <<"ID " << getId() <<": runtime power = " << pdata.runtimeDynamicPower << " W" << std::endl;
	using namespace io_interval; std::cout <<"ID " << getId() <<": total energy = " << pdata.totalEnergy << " J" << std::endl;
	using namespace io_interval; std::cout <<"ID " << getId() <<": peak power = " << pdata.peak << " W" << std::endl;
#endif
		
    }

    return false;

} // end of pushData()
#endif



// Based on message length and the router parameter list, extract the
// delay this message will experience on the output port.
int64_t
Routermodel::get_Rtrparams(std::list<Rtrparams_t> params, int64_t msg_len)
{

std::list<Rtrparams_t>::iterator k;
std::list<Rtrparams_t>::iterator previous;
double T, B;
double byte_cost;
int64_t latency;


    previous= params.begin();
    k= previous;
    k++;
    for (; k != params.end(); k++)   {
	if (k->inflectionpoint > msg_len)   {
	    T= k->latency - previous->latency;
	    B= k->inflectionpoint - previous->inflectionpoint;
	    byte_cost= T / B;
	    if (byte_cost < 0)   {
		// FIXME: Not sure this is a good decision
		byte_cost= 0;
	    }
	    // We want the values from the previous point
	    if (previous->latency < 0)   {
		latency= 0;
	    } else   {
		latency= previous->latency;
	    }
	    return (int64_t)((double)(msg_len - previous->inflectionpoint) * byte_cost) + latency;
	}
	previous++;
    }

    // We're beyond the list. Use the last two values in the list and
    // extrapolite.
    previous--;
    T= params.back().latency - previous->latency;
    B= params.back().inflectionpoint - previous->inflectionpoint;
    byte_cost= T / B;
    if (byte_cost < 0)   {
	// FIXME: Not sure this is a good decision
	byte_cost= 0;
    }
    if (previous->latency < 0)   {
	latency= 0;
    } else   {
	latency= previous->latency;
    }
    return (int64_t)((double)(msg_len - previous->inflectionpoint) * byte_cost) + latency;

}  // end of getRtrparams()



/*
// THIS SECTION MOVED TO patterns.cc FOR RELEASE 3.x OF SST - ALEVINE

static Component*
create_routermodel(SST::ComponentId_t id, 
                  SST::Params& params)
{
    return new Routermodel( id, params );
}

static const ElementInfoComponent components1[] = {
    { "routermodel",
      "router model without power",
      NULL,
      create_routermodel
    },
    { NULL, NULL, NULL, NULL }
};

extern "C" {
    ElementLibraryInfo routermodel_eli = {
        "routermodel",
        "router model without power",
        components1,
    };
}



static Component*
create_routermodel_power(SST::ComponentId_t id, 
                  SST::Params& params)
{
    return new Routermodel( id, params );
}

static const ElementInfoComponent components2[] = {
    { "routermodel_power",
      "router model with power",
      NULL,
      create_routermodel_power
    },
    { NULL, NULL, NULL, NULL }
};

extern "C" {
    ElementLibraryInfo routermodel_power_eli = {
        "routermodel_power",
        "router model with power",
        components2,
    };
}
*/

BOOST_CLASS_EXPORT(Routermodel)
