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


/*
** This file contains common routines used by all pattern generators.
*/
#include <stdio.h>
#include <sst_config.h>
#include <sst/core/cpunicEvent.h>

#include "pattern_common.h"


using namespace SST;


/* Local functions */
static int is_pow2(int num);



/*
** Must be called before any other function in this file.
** Sets up some things based on stuff we learn when we read the
** SST xml file.
*/
int
Patterns::init(int x, int y, int rank, Link *net_link, Link *self_link, SimTime_t lat, SimTime_t bw)
{
    /* Make sure x * y is a power of 2, and my_rank is within range */
    if (!is_pow2(x * y))   {
	fprintf(stderr, "x = %d * y = %d is not a power of 2!\n", x, y);
	return FALSE;
    }

    if ((rank < 0) || (rank >= x * y))   {
	fprintf(stderr, "My rank not 0 <= %d < %d * %d\n", rank, x, y);
	return FALSE;
    }

    if (lat == 0)   {
	fprintf(stderr, "Latency in xml file must be specified\n");
	return FALSE;
    }

    if (bw == 0)   {
	fprintf(stderr, "Bandwidth in xml file must be specified\n");
	return FALSE;
    }

    my_net_link= net_link;
    my_self_link= self_link;
    mesh_width= x;
    mesh_height= y;
    my_rank= rank;
    net_latency= lat;
    net_bandwidth= bw;

    return TRUE; /* success */

}  /* end of init() */



/*
** Send "len" bytes to rank "dest"
** This function computes a route, based on the assumtopn that we
** are on a power of 2 x * y 2-D torus, and sends and event along
** that route. No actual data is sent.
*/
void
Patterns::send(int dest, int len)
{

SimTime_t delay;


    // This is a crude approximation of how long this message would
    // take to transmit, if there were no routers in between. (Each
    // router adds its own delays as the event passes through.)
    //
    // net_latency is in nano seconds and net_bandwidth is in bytes per second
    delay= net_latency + ((SimTime_t)len * 1000000000) / net_bandwidth;
    event_send(dest, RECEIVE, delay, len);

}  /* end of send() */



#define LOCAL_PORT	(0)
#define EAST_PORT	(1)
#define SOUTH_PORT	(2)
#define WEST_PORT	(3)
#define NORTH_PORT	(4)

void
Patterns::event_send(int dest, pattern_event_t event, SimTime_t delay, uint32_t msg_len)
{

CPUNicEvent *e;
int my_X, my_Y;
int dest_X, dest_Y;
int x_delta, y_delta;
int c1, c2;


    // Create an event and fill in the event info
    e= new CPUNicEvent();
    e->SetRoutine((int)event);

    if (dest == my_rank)   {
	// No need to go through the network for this
	my_self_link->Send(delay, e);
	return;
    }

    // If this event goes to another node, attach a route to it
    e->router_delay= 0;
    e->hops= 0;
    e->msg_len= msg_len;

    // We are hardcoded for a x * y torus!
    // the program genPatterns generates XML files with routers that have
    // the following port connections:
    // Port 0: to local NIC (pattern generator)
    // Port 1: East/Left
    // Port 2: South/Down
    // Port 3: West/Right
    // Port 4: North/Up

    // First we need to calculate the X and Y delta from us to dest
    my_Y= my_rank / mesh_width;
    my_X= my_rank % mesh_width;
    dest_Y= dest / mesh_width;
    dest_X= dest % mesh_width;

    // Use the shortest distance to reach the destination
    if (my_X > dest_X)   {
	// How much does it cost to go left?
	c1= my_X - dest_X;
	// How much does it cost to go right?
	c2= (dest_X + mesh_width) - my_X;
    } else   {
	// How much does it cost to go left?
	c1= (my_X + mesh_width) - dest_X;
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
	c2= (dest_Y + mesh_height) - my_Y;
    } else   {
	// How much does it cost to go up?
	c1= (my_Y + mesh_height) - dest_Y;
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

    fprintf(stderr, "[%2d] my X %d, Y %d, dest [%2d] X %d, dest Y %d, offset %d,%d\n",
	my_rank, my_X, my_Y, dest, dest_X, dest_Y, x_delta, y_delta);

    if (x_delta > 0)   {
	// Go East first
	while (x_delta > 0)   {
	    e->route.push_back(EAST_PORT);
	    x_delta--;
	}
    } else if (x_delta < 0)   {
	// Go West first
	while (x_delta < 0)   {
	    e->route.push_back(WEST_PORT);
	    x_delta++;
	}
    }

    if (y_delta > 0)   {
	// Go SOUTH first
	while (y_delta > 0)   {
	    e->route.push_back(SOUTH_PORT);
	    y_delta--;
	}
    } else if (y_delta < 0)   {
	// Go NORTH first
	while (y_delta < 0)   {
	    e->route.push_back(NORTH_PORT);
	    y_delta++;
	}
    }

    // Exit to the local (NIC) pattern generator
    e->route.push_back(LOCAL_PORT);

    // Send it
    // fprintf(stderr, "--> Sending event %d from %d to %d with a delay of %lu\n", event,
    //     my_rank, dest, (uint64_t)delay);
    {
    char route[128];
    std::vector<uint8_t>::iterator itNum;
    int i= 0;

    for(itNum = e->route.begin(); itNum < e->route.end(); itNum++)   {
	route[i++]= '0' + *itNum;
    }
    route[i++]= 0;

    fprintf(stderr, "Event %d from %d --> %d, delay %lu, route %s\n", event,
         my_rank, dest, (uint64_t)delay, route);
    }

    my_net_link->Send(delay, e);

}  /* end of event_send() */



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
