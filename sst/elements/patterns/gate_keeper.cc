// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


/*


*/
#include <sst_config.h>
#include "sst/core/serialization/element.h"
#include <sst/core/cpunicEvent.h>
#include <assert.h>
#include "gate_keeper.h"
#include "pattern_common.h"



void
Gate_keeper::init(Component *me, ComponentId_t id, Params& params)
{
    Params::iterator it= params.begin();

    my_component= me;
    my_component->registerExit();

    // Process parameters common to all patterns
    while (it != params.end())   {
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
    tc= my_component->registerTimeBase("1ns", true);

    // Create a handler for events from the Network
    net= my_component->configureLink("NETWORK", new Event::Handler<Gate_keeper>
	    (this, &Gate_keeper::handle_net_events));
    if (net == NULL)   {
	fprintf(stderr, "There is no network...\n");
    } else   {
	net->setDefaultTimeBase(tc);
    }

    // Create a handler for events from the network on chip (NoC)
    NoC= my_component->configureLink("NoC", new Event::Handler<Gate_keeper>
	    (this, &Gate_keeper::handle_NoC_events));
    if (NoC == NULL)   {
	fprintf(stderr, "There is no NoC...\n");
    } else   {
	NoC->setDefaultTimeBase(tc);
    }

    // Create a channel for "out of band" events sent to ourselves
    self_link= my_component->configureSelfLink("Me", new Event::Handler<Gate_keeper>
	    (this, &Gate_keeper::handle_self_events));
    if (self_link == NULL)   {
	_ABORT(Gate_keeper, "That was no good!\n");
    }

    // Create a handler for events from local NVRAM
    nvram= my_component->configureLink("NVRAM", new Event::Handler<Gate_keeper>
	    (this, &Gate_keeper::handle_nvram_events));
    if (nvram == NULL)   {
	fprintf(stderr, "The msgrate pattern generator expects a link named \"NVRAM\"\n");
	_ABORT(Gate_keeper, "Check the input XML file!\n");
    }

    // Create a handler for events from the storage network
    storage= my_component->configureLink("STORAGE", new Event::Handler<Gate_keeper>
	    (this, &Gate_keeper::handle_storage_events));
    if (storage == NULL)   {
	fprintf(stderr, "The msgrate pattern generator expects a link named \"STORAGE\"\n");
	_ABORT(Gate_keeper, "Check the input XML file!\n");
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


}  /* end of init() */



void
Gate_keeper::start(void)
{

    // Send a start event to ourselves without a delay
    common->event_send(my_rank, START);
    fprintf(stderr, "Gatekeeper[%2d]: %s event sent\n", my_rank, __FUNCTION__);

}  /* end of start() */



void
Gate_keeper::handle_events(CPUNicEvent *e)
{

pattern_event_t event;


    // Extract the pattern event type from the SST event
    // (We are "misusing" the routine filed in CPUNicEvent to xmit the event type
    fprintf(stderr, "Gatekeeper[%2d]: %s got an event\n", my_rank, __FUNCTION__);
    event= (pattern_event_t)e->GetRoutine();
    fprintf(stderr, "Gatekeeper[%2d]: %s event is %d\n", my_rank, __FUNCTION__, (int)event);

    // We need to figure out which of the state machines is currently active
    // If this even is of the active machine, we send it down. Otherwise we
    // queue it.


    // We're done
    delete(e);

    my_component->unregisterExit();
    fprintf(stderr, "Gatekeeper[%2d]: %s returning\n", my_rank, __FUNCTION__);
    return;

}  /* end of handle_events() */



// Messages from the global network
void
Gate_keeper::handle_net_events(Event *sst_event)
{

CPUNicEvent *e;


    e= static_cast<CPUNicEvent *>(sst_event);

    if (e->dest != my_rank)   {
	_abort(gate_keeper, "NETWORK dest %d != my rank %d\n", e->dest, my_rank);
    }

    handle_events(e);

}  /* end of handle_net_events() */



// Messages from the local chip network
void
Gate_keeper::handle_NoC_events(Event *sst_event)
{

CPUNicEvent *e;


    e= static_cast<CPUNicEvent *>(sst_event);

    if (e->dest != my_rank)   {
	_abort(gate_keeper, "NoC dest %d != my rank %d\n", e->dest, my_rank);
    }

    handle_events(e);

}  /* end of handle_NoC_events() */



// When we send to ourselves, we come here.
// Just pass it on to the main handler above
void
Gate_keeper::handle_self_events(Event *sst_event)
{

CPUNicEvent *e;


    e= static_cast<CPUNicEvent *>(sst_event);
    handle_events(e);

}  /* end of handle_self_events() */



// Events from the local NVRAM
void
Gate_keeper::handle_nvram_events(Event *sst_event)
{

CPUNicEvent *e;


    // _abort(gate_keeper, "We should not get NVRAM events in this pattern!\n");
    e= static_cast<CPUNicEvent *>(sst_event);
    handle_events(e);

}  /* end of handle_nvram_events() */



// Events from stable storage
void
Gate_keeper::handle_storage_events(Event *sst_event)
{

CPUNicEvent *e;


    // _abort(gate_keeper, "We should not get stable storage events in this pattern!\n");
    e= static_cast<CPUNicEvent *>(sst_event);
    handle_events(e);

}  /* end of handle_storage_events() */


//
// -----------------------------------------------------------------------------
// Implementation of the gate keeper API
