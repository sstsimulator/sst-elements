/*
** Copyright 2009-2010 Sandia Corporation. Under the terms
** of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
** Government retains certain rights in this software.
**
** Copyright (c) 2009-2010, Sandia Corporation
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
static int is_pow2(int num);
static void gen_route(CPUNicEvent *e, int src, int dest, int width, int height);



/*
** Must be called before any other function in this file.
** Sets up some things based on stuff we learn when we read the
** SST xml file.
*/
int
Patterns::init(int x, int y, int NoC_x_dim, int NoC_y_dim, int rank, int cores, Link *net_link,
	Link *self_link, SST::Link *NoC_link, SST::Link *nvram_link, SST::Link *storage_link,
	SimTime_t net_lat, SimTime_t net_bw, SimTime_t node_lat, SimTime_t node_bw,
	chckpt_t method, int chckpt_size, SimTime_t chckpt_interval,
	int envelope_size)
{

    /* Make sure number of ranks is a power of 2, and my_rank is within range */
    total_cores= x * y * NoC_x_dim * NoC_y_dim * cores;
    if (!is_pow2(total_cores))   {
	fprintf(stderr, "Total number of ranks (cores) %d is not a power of 2!\n", total_cores);
	return FALSE;
    }

    if ((rank < 0) || (rank >= total_cores))   {
	fprintf(stderr, "My rank not 0 <= %d < %d (cores)\n", rank, total_cores);
	return FALSE;
    }

    if (net_lat == 0)   {
	fprintf(stderr, "Network latency in xml file must be specified\n");
	return FALSE;
    }

    if (net_bw == 0)   {
	fprintf(stderr, "Network bandwidth in xml file must be specified\n");
	return FALSE;
    }

    if (node_lat == 0)   {
	fprintf(stderr, "Node latency in xml file must be specified\n");
	return FALSE;
    }

    if (node_bw == 0)   {
	fprintf(stderr, "Node bandwidth in xml file must be specified\n");
	return FALSE;
    }

    my_net_link= net_link;
    my_self_link= self_link;
    my_NoC_link= NoC_link;
    my_nvram_link= nvram_link;
    my_storage_link= storage_link;

    mesh_width= x;
    mesh_height= y;
    NoC_width= NoC_x_dim;
    NoC_height= NoC_y_dim;
    my_rank= rank;
    net_latency= net_lat;
    net_bandwidth= net_bw;
    node_latency= node_lat;
    node_bandwidth= node_bw;
    cores_per_router= cores;
    cores_per_node= NoC_width * NoC_height * cores;

    if (my_rank == 0)   {
	printf("||| mesh x %d, y %d, = %d nodes\n",
	    mesh_width, mesh_height, mesh_width * mesh_height);
	printf("||| NoC x %d, y %d, with %d cores each\n",
	    NoC_x_dim, NoC_y_dim, cores_per_router);
	printf("||| Cores per node %d. Total %d cores in system\n", cores_per_node, total_cores);
	printf("||| Network bandwidth %.3f GB/s, latency %.9f s\n",
	    (double)net_bandwidth / 1000000000.0, (double)net_latency / 1000000000.0);
	printf("||| Node bandwidth    %.3f GB/s, latency %.9f s\n",
	    (double)node_bandwidth / 1000000000.0, (double)node_latency / 1000000000.0);
	printf("||| Checkpoint method is ");
	switch (method)   {
	    case CHCKPT_NONE:
		printf("none\n");
		break;
	    case CHCKPT_COORD:
		printf("coordinated\n");
		printf("||| Checkpoint every %.9f s\n", (double)chckpt_interval / 1000000000.0);
		break;
	    case CHCKPT_UNCOORD:
		printf("uncoordinated with message logging\n");
		break;
	    case CHCKPT_RAID:
		printf("distributed\n");
		break;
	}
    }

    return TRUE; /* success */

}  /* end of init() */



/*
** Send "len" bytes to rank "dest"
*/
void
Patterns::send(int dest, int len)
{
    send(0, dest, len);
}  /* end of send() */


void
Patterns::send(SST::SimTime_t start_delay, int dest, int len)
{

SimTime_t delay;


    // This is a crude approximation of how long this message would
    // take to transmit. We let SST handle latency by specyfing it
    // on the links in the xml file
    //
    // net_bandwidth is in bytes per second

    if ((my_rank / total_cores) != (dest / total_cores))   {
	// This message goes off node
	delay= ((SimTime_t)len * 1000000000) / net_bandwidth + start_delay;
    } else   {
	// This is an intra-node message
	delay= ((SimTime_t)len * 1000000000) / node_bandwidth + start_delay;
    }
    event_send(dest, RECEIVE, delay, len);

}  /* end of send() */



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
** are on a x * y torus for our NoC, and a x * y mesh for the network.
** This function then sends an event along that route.
** No actual data is sent.
*/
void
Patterns::event_send(int dest, pattern_event_t event, SimTime_t delay, uint32_t msg_len,
	const char *payload, int payload_len)
{

CPUNicEvent *e;
int my_node, dest_node;
int my_router, dest_router;


    // Create an event and fill in the event info
    e= new CPUNicEvent();
    e->SetRoutine((int)event);
    e->router_delay= 0;
    e->hops= 0;
    e->msg_len= msg_len;
    e->dest= dest;
    e->msg_id= (msg_seq++ << RANK_FIELD) | my_rank;

    // If there is a payload, attach it
    if (payload)   {
	e->AttachPayload(payload, payload_len);
    }

    if (dest == my_rank)   {
	// No need to go through the network for this
	my_self_link->Send(delay, e);
	return;
    }

    /* Is dest within our NoC? */
    my_node= my_rank / (NoC_width * NoC_height * cores_per_router);
    dest_node= dest / (NoC_width * NoC_height * cores_per_router);

    if (my_node == dest_node)   {
	/* Route locally */

	/*
	** For cores that share a router we'll assume that they
	** can communicate with each other much faster; e.g., through
	** a shared L2 cache. We mark that traffic so the router can
	** deal with it seperatly.
	*/
	my_router= my_rank / cores_per_router;
	dest_router= dest / cores_per_router;
	if (my_router != dest_router)   {
	    // This message travels through the NoC
	    e->local_traffic= false;
	} else   {
	    // This is a message among cores of the same router
	    e->local_traffic= true;
	}

	gen_route(e, my_router, dest_router, NoC_width, NoC_height);

	// Exit to the local (NIC) pattern generator
	e->route.push_back(FIRST_LOCAL_PORT + (dest % cores_per_router));

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
	     my_rank, dest, (uint64_t)delay, route);
    }
#endif  // ROUTE_DEBUG

	// Send it
	// FIXME: Multile sends during processing of a single events should be
	// delayed, since they don't really ocur all at exactly the same nano second.
	// However, since they go out on the same link, the router at the other end
	// will delay them according to their length and link bandwidth.
	my_NoC_link->Send(delay, e);

    } else   {
	/* Route off chip */
	e->local_traffic= false;

	/* On the network aggregator we'll have to go out port 0 to reach the mesh */
	e->route.push_back(0);

	// We assume one node per mesh router
	my_router= my_node;
	dest_router= dest_node;
	gen_route(e, my_router, dest_router, mesh_width, mesh_height);


	// Exit to the local node (aggregator on that node
	e->route.push_back(FIRST_LOCAL_PORT);

	/*
	** We come out at the aggregator on the destination node.
	** Figure out which port to go out to reach the destination core
	** Port 0 is where we came in
	*/
	e->route.push_back(1 + (dest % cores_per_node));

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
	     my_rank, dest, (uint64_t)delay, route);
    }
#endif  // ROUTE_DEBUG

	my_net_link->Send(delay, e);
    }

}  /* end of event_send() */



/*
** Send an out-of-band message
*/
void
Patterns::sendOB(int dest, pattern_event_t event, int value)
{

char *payload;
int payload_len;


    payload= (char *)&value;
    payload_len= sizeof(value);
    event_send(dest, event, 0, 0, payload, payload_len);

} /* end of sendOB() */



/*
** Send a chunk of data to our stable storage device.
*/
void
Patterns::storage_write(int data_size, pattern_event_t return_event)
{

CPUNicEvent *e;
int my_node;
int my_core;
uint64_t delay;


    // Create an event and fill in the event info
    e= new CPUNicEvent();
    e->SetRoutine(BIT_BUCKET_WRITE_START);
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
    my_node= my_rank / (NoC_width * NoC_height * cores_per_router);
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
Patterns::nvram_write(int data_size, pattern_event_t return_event)
{

CPUNicEvent *e;
int my_core;
uint64_t delay;


    // Create an event and fill in the event info
    e= new CPUNicEvent();
    e->SetRoutine(BIT_BUCKET_WRITE_START);
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

static int
is_pow2(int num)
{
    if (num < 1)   {
	return FALSE;
    }

    if ((num & (num - 1)) == 0)   {
	return TRUE;
    }

    return FALSE;

}  /* end of is_pow2() */



/*
** Generate a route through a X 8 Y torus
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
