/*
** Copyright 2009-2011 Sandia Corporation. Under the terms
** of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
** Government retains certain rights in this software.
**
** Copyright (c) 2009-2011, Sandia Corporation
** All rights reserved.
**
** This file is part of the SST software package. For license
** information, see the LICENSE file in the top level directory of the
** distribution.
*/

#include <sst_config.h>
#include "sst/core/serialization/element.h"

/*
** This file contains common routines used by all pattern generators.
*/
#include <stdio.h>
#include <sst/core/cpunicEvent.h>

#include "pattern_common.h"


using namespace SST;


/* Local functions */
static void gen_route(CPUNicEvent *e, int src, int dest, int width, int height);



/*
** Must be called before any other function in this file.
** Sets up some things based on stuff we learn when we read the
** SST xml file.
*/
int
Patterns::init(SST::Component::Params_t& params, Link *net_link, Link *self_link,
	SST::Link *NoC_link, SST::Link *nvram_link, SST::Link *storage_link)
{

    SST::Component::Params_t::iterator it= params.begin();
    while (it != params.end())   {
	if (!it->first.compare("rank"))   {
	    sscanf(it->second.c_str(), "%d", &my_rank);
	}

	if (!it->first.compare("x_dim"))   {
	    sscanf(it->second.c_str(), "%d", &mesh_width);
	}

	if (!it->first.compare("y_dim"))   {
	    sscanf(it->second.c_str(), "%d", &mesh_height);
	}

	if (!it->first.compare("NoC_x_dim"))   {
	    sscanf(it->second.c_str(), "%d", &NoC_width);
	}

	if (!it->first.compare("NoC_y_dim"))   {
	    sscanf(it->second.c_str(), "%d", &NoC_height);
	}

	if (!it->first.compare("cores"))   {
	    sscanf(it->second.c_str(), "%d", &cores_per_NoC_router);
	}

	if (!it->first.compare("nodes"))   {
	    sscanf(it->second.c_str(), "%d", &num_router_nodes);
	}

	if (!it->first.compare("envelope_size"))   {
	    sscanf(it->second.c_str(), "%d", &envelope_size);
	}

	it++;
    }

    // FIXME: Do some more thorough input checking here; e.g., cores and nodes must be > 0
    total_cores= mesh_width * mesh_height * num_router_nodes * NoC_width * NoC_height * cores_per_NoC_router;
    cores_per_node= NoC_width * NoC_height * cores_per_NoC_router;
    cores_per_Net_router= cores_per_node * num_router_nodes;

    if ((my_rank < 0) || (my_rank >= total_cores))   {
	fprintf(stderr, "My rank not 0 <= %d < %d (cores)\n", my_rank, total_cores);
	return FALSE;
    }

    my_net_link= net_link;
    my_self_link= self_link;
    my_NoC_link= NoC_link;
    my_nvram_link= nvram_link;
    my_storage_link= storage_link;


    if (my_rank == 0)   {
	printf("#  |||  mesh x %d, y %d, with %d nodes each = %d nodes\n",
	    mesh_width, mesh_height, num_router_nodes, mesh_width * mesh_height * num_router_nodes);
	printf("#  |||  NoC x %d, y %d, with %d cores each\n",
	    NoC_width, NoC_height, cores_per_NoC_router);
	printf("#  |||  Cores per node %d. Total %d cores in system\n", cores_per_node, total_cores);
    }

    return TRUE; /* success */

}  /* end of init() */



// Save the lower 27 bits of the event ID for the rank number
#define RANK_FIELD		(27)


// There is a lot of hardcoded information about how the network is wired.
// the program genPatterns generates XML files with routers that have
// the following port connections:
// Port 0: East/Left
// Port 1: South/Down
// Port 2: West/Right
// Port 3: North/Up
// Port 4...: local links to cores or nodes

#define EAST_PORT		(0)
#define SOUTH_PORT		(1)
#define WEST_PORT		(2)
#define NORTH_PORT		(3)
#define FIRST_LOCAL_PORT	(4)

/*
** This function computes a route, based on the assumption that we
** are on a x * y torus for our NoC, and a x * y torus for the network.
** This function then sends an event along that route.
** No actual data is sent.
*/
void
Patterns::event_send(int dest, int event, int32_t tag, uint32_t msg_len,
	const char *payload, int payload_len)
{

CPUNicEvent *e;
int my_node, dest_node;


    // Create an event and fill in the event info
    e= new CPUNicEvent();
    e->SetRoutine((int)event);
    e->router_delay= 0;
    e->hops= 0;
    e->msg_len= msg_len;
    e->tag= tag;
    e->dest= dest;
    e->msg_id= (msg_seq++ << RANK_FIELD) | my_rank;

    // If there is a payload, attach it
    if (payload)   {
	e->AttachPayload(payload, payload_len);
    }

    if (dest == my_rank)   {
	// No need to go through the network for this
	// FIXME: Shouldn't this involve some sort of delay?
	my_self_link->Send(e);
	return;
    }

    /* Is dest within our NoC? */
    my_node= my_rank / (NoC_width * NoC_height * cores_per_NoC_router);
    dest_node= dest / (NoC_width * NoC_height * cores_per_NoC_router);

    if (my_node == dest_node)   {
	/* Route locally */
	NoCsend(e, my_rank, dest);

    } else   {
	/* Route off chip */
	Netsend(e, my_node, dest_node, dest);
    }

}  /* end of event_send() */



/*
** Send a chunk of data to our stable storage device.
*/
void
Patterns::storage_write(int data_size, int return_event)
{

CPUNicEvent *e;
int my_node;
int my_core;
uint64_t delay;


    // Create an event and fill in the event info
    e= new CPUNicEvent();
    assert(0);  // FIXME: We need to SetRoutine()
    // e->SetRoutine(BIT_BUCKET_WRITE_START);
    e->router_delay= 0;
    e->hops= 0;
    e->msg_len= data_size;
    e->dest= -1;
    e->msg_id= (msg_seq++ << RANK_FIELD) | my_rank;
    e->return_event= return_event;

    // Storage request go out on port 0 of the aggregator
    e->route.push_back(0);

    // And then again on port 0 at the node aggregator
    e->route.push_back(0);

    // Coming back: First hop is to the right node
    my_node= my_rank / (NoC_width * NoC_height * cores_per_NoC_router);
    e->reverse_route.push_back(my_node + 1);	// Port 0 is for the SSD

    // From the aggregator, pick the right core
    my_core= my_rank % cores_per_node;
    e->reverse_route.push_back(my_core + 1);	// Port 0 goes off node

    // Send the write request
    delay= 0; // FIXME; Need to figure this out
    my_storage_link->Send(delay, e);

}  /* end of storage_write() */



/*
** Send a chunk of data to our local NVRAM
*/
void
Patterns::nvram_write(int data_size, int return_event)
{

CPUNicEvent *e;
int my_core;
uint64_t delay;


    // Create an event and fill in the event info
    e= new CPUNicEvent();
    // assert(0);  // FIXME: We need to SetRoutine()
    // e->SetRoutine(BIT_BUCKET_WRITE_START);
    e->router_delay= 0;
    e->hops= 0;
    e->msg_len= data_size;
    e->dest= -1;
    e->msg_id= (msg_seq++ << RANK_FIELD) | my_rank;
    e->return_event= return_event;

    // NVRAM requests go out on port 0 of the aggregator
    e->route.push_back(0);

    // Coming back: Only one hop back to us
    my_core= my_rank % cores_per_node;
    e->reverse_route.push_back(my_core + 1);	// Port 0 goes off node

    // Send the write request
    delay= 0; // FIXME; Need to figure this out
    my_nvram_link->Send(delay, e);

}  /* end of nvram_write() */



/*
** -----------------------------------------------------------------------------
** Some local functions
** -----------------------------------------------------------------------------
*/



/*
** Send a message into the Network on Chip with the appropriate
** delays.
*/
void
Patterns::NoCsend(CPUNicEvent *e, int my_rank, int dest_rank)
{

SimTime_t delay= 0;
int my_router, dest_router;


    /*
    ** For cores that share a router we'll assume that they
    ** can communicate with each other much faster; e.g., through
    ** a shared L2 cache. We mark that traffic so the router can
    ** deal with it seperatly.
    */
    my_router= my_rank / cores_per_NoC_router;
    dest_router= dest_rank / cores_per_NoC_router;
    if (my_router != dest_router)   {
	// This message travels through the NoC
	e->local_traffic= false;
    } else   {
	// This is a message among cores of the same router
	e->local_traffic= true;
    }

    gen_route(e, my_router, dest_router, NoC_width, NoC_height);

    // Exit to the local (NIC) pattern generator
    e->route.push_back(FIRST_LOCAL_PORT + (dest_rank % cores_per_NoC_router));

#define ROUTE_DEBUG 1
#undef ROUTE_DEBUG
#if ROUTE_DEBUG
{
    char route[128];
    char tmp[128];
    route[0]= 0;
    tmp[0]= 0;
    std::vector<int>::iterator itNum;

    for(itNum = e->route.begin(); itNum < e->route.end(); itNum++)   {
	sprintf(tmp, "%s.%d", route, *itNum);
	strcpy(route, tmp);
    }

    fprintf(stderr, "NoC Event %d from %d --> %d, delay %lu, route %s\n", event,
	 my_rank, dest_rank, (uint64_t)delay, route);
}
#endif  // ROUTE_DEBUG

    // Send it
    // FIXME: Multiple sends during processing of a single events should be
    // delayed, since they don't really ocur all at exactly the same nano second.
    // However, since they go out on the same link, the router at the other end
    // will delay them according to their length and link bandwidth.
    my_NoC_link->Send(delay, e);

}  // end of NoCsend()



/*
** Send a message into the global Network with the
** appropriate delays.
*/
void
Patterns::Netsend(CPUNicEvent *e, int my_node, int dest_node, int dest_rank)
{

SimTime_t delay= 0;
int my_router, dest_router;


	e->local_traffic= false;

	/* On the network aggregator we'll have to go out port 0 to reach the mesh */
	e->route.push_back(0);

	my_router= my_node / num_router_nodes;
	dest_router= dest_node / num_router_nodes;
	gen_route(e, my_router, dest_router, mesh_width, mesh_height);


	// Exit to the local node (aggregator on that node
	e->route.push_back(FIRST_LOCAL_PORT +
		((dest_rank % (num_router_nodes * cores_per_node)) / cores_per_node));

	/*
	** We come out at the aggregator on the destination node.
	** Figure out which port to go out to reach the destination core
	** Port 0 is where we came in
	*/
	e->route.push_back(1 + (dest_rank % cores_per_node));

#if ROUTE_DEBUG
    {
	char route[128];
	char tmp[128];
	route[0]= 0;
	tmp[0]= 0;
	std::vector<int>::iterator itNum;

	for(itNum = e->route.begin(); itNum < e->route.end(); itNum++)   {
	    sprintf(tmp, "%s.%d", route, *itNum);
	    strcpy(route, tmp);
	}

	fprintf(stderr, "Net Event %d from %d --> %d, delay %lu, route %s\n", event,
	     my_rank, dest_rank, (uint64_t)delay, route);
    }
#endif  // ROUTE_DEBUG

	my_net_link->Send(delay, e);

}  // end of Netsend()



/*
** Generate a route through a X * Y torus
** We use it to traverse the mesh as well as the NoC
*/
static void
gen_route(CPUNicEvent *e, int src, int dest, int width, int height)
{

int c1, c2;
int my_X, my_Y;
int dest_X, dest_Y;
int x_delta, y_delta;


    // Calculate x and y coordinates within the Network
    my_Y= src / width;
    my_X= src % width;
    dest_Y= dest / width;
    dest_X= dest % width;

    // Use the shortest distance to reach the destination
    if (my_X > dest_X)   {
	// How much does it cost to go left?
	c1= my_X - dest_X;
	// How much does it cost to go right?
	c2= (dest_X + width) - my_X;
    } else   {
	// How much does it cost to go left?
	c1= (my_X + width) - dest_X;
	// How much does it cost to go right?
	c2= dest_X - my_X;
    }
    if (c1 > c2)   {
	// go right
	x_delta= c2;
    } else   {
	// go left
	x_delta= -c1;
    }

    if (my_Y > dest_Y)   {
	// How much does it cost to go up?
	c1= my_Y - dest_Y;
	// How much does it cost to go down?
	c2= (dest_Y + height) - my_Y;
    } else   {
	// How much does it cost to go up?
	c1= (my_Y + height) - dest_Y;
	// How much does it cost to go down?
	c2= dest_Y - my_Y;
    }
    if (c1 > c2)   {
	// go down
	y_delta= c2;
    } else   {
	// go up
	y_delta= -c1;
    }

    if (x_delta > 0)   {
	// Go East
	while (x_delta > 0)   {
	    e->route.push_back(EAST_PORT);
	    x_delta--;
	}
    } else if (x_delta < 0)   {
	// Go West
	while (x_delta < 0)   {
	    e->route.push_back(WEST_PORT);
	    x_delta++;
	}
    }

    if (y_delta > 0)   {
	// Go SOUTH
	while (y_delta > 0)   {
	    e->route.push_back(SOUTH_PORT);
	    y_delta--;
	}
    } else if (y_delta < 0)   {
	// Go NORTH
	while (y_delta < 0)   {
	    e->route.push_back(NORTH_PORT);
	    y_delta++;
	}
    }

}  /* end of gen_route() */
