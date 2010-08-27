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


#include <sst_config.h>
#include <sst/core/serialization/element.h>
#include <sst/core/cpunicEvent.h>
#include <assert.h>
#include "routermodel.h"



void
Routermodel::handle_port_events(Event *event, int in_port)
{

SimTime_t current_time;
SimTime_t delay;
SimTime_t link_time;
uint8_t out_port;


    // Check for routing algorithm problems
    assert((in_port >= 0) && (in_port < num_ports));

    current_time= getCurrentSimTime();
    _ROUTER_MODEL_DBG(3, "Router %s got an event from port %d at time %lu\n",
	component_name.c_str(), in_port, (uint64_t)current_time);
    CPUNicEvent *e= static_cast<CPUNicEvent *>(event);
    port[in_port].cnt_in++;

#if DBG_ROUTER_MODEL > 1
    /* Diagnostic: print the route this event is taking */
    if (router_model_debug >= 4)   {
	std::vector<uint8_t>::iterator itNum;
	char str[32];
	int i= 0;

	sprintf(str, "Event route: ");
	for(itNum = e->route.begin(); itNum < e->route.end(); itNum++)   {
	    if (i == e->hops)   {
		sprintf(str, "%s%c", str, '[');
	    }
	    // str= str + boost::lexical_cast<std::string>(*itNum);
	    sprintf(str, "%s%d", str, *itNum);
	    if (i == e->hops)   {
		sprintf(str, "%s%c", str, ']');
	    }
	    sprintf(str, "%s%c", str, ' ');
	    i++;
	}
	_ROUTER_MODEL_DBG(4, "%s\n", str);
    }
#endif  // DBG_ROUTER_MODEL

    // Where are we going?
    out_port= e->route[e->hops];
    assert((out_port >= 0) && (out_port < num_ports));


    // How long will this message occupy the input and output port?
    // FIXME: The constant 1000000000 should be replaced with our time base
    link_time= ((uint64_t)e->msg_len * 1000000000) / router_bw;

    if (port[in_port].next_in > current_time)   {
	SimTime_t arrival_delay;

	// If the input port is in use right now, then this message actually
	// wont come in until later
	arrival_delay= port[in_port].next_in - current_time;

	// FIXME: I am not sure these are meaningful statistics
	congestion_in_cnt++;
	congestion_in += arrival_delay;
	e->congestion_cnt++;
	e->congestion_delay += arrival_delay;

	e->entry_port= in_port;
	self_link->Send(arrival_delay, e);

	return;
    }


    // What is the current delay to send on this output port?
    if (port[out_port].next_out <= current_time)   {
	// No output port delays.  We can send as soon as the message arrives;
	delay= 0;

    } else   {
	// Busy right now
	delay= port[out_port].next_out - current_time;
	congestion_out_cnt++;
	congestion_out += delay;
	e->congestion_cnt++;
	e->congestion_delay += delay;
    }

    // Add in the generic router delay
    delay += hop_delay;

    port[in_port].next_in= current_time + delay + link_time;

    // Once this message is going out, the port will be busy for that
    // much longer
    port[out_port].next_out= current_time + delay + link_time;

    e->hops++;

    _ROUTER_MODEL_DBG(3, "Sending message out on port %d at time %lu with delay %lu\n",
	out_port, (uint64_t)current_time, (uint64_t)delay);

    port[out_port].link->Send(delay, e);

}  /* end of handle_port_events() */



// When we send to ourselves, we come here.
// Just pass it on to the main handler above
void
Routermodel::handle_self_events(Event *event)
{

    CPUNicEvent *e= static_cast<CPUNicEvent *>(event);

    if (e->entry_port < 0)   {
	_abort(Routermodel, "Internal error: entry port not defined!\n");
    }

    handle_port_events(e, e->entry_port);

}  /* end of handle_self_events() */



extern "C" {
Routermodel *
routermodelAllocComponent(SST::ComponentId_t id,
                          SST::Component::Params_t& params)
{
    return new Routermodel(id, params);
}
}

BOOST_CLASS_EXPORT(Routermodel)
