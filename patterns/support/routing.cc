//
// Copyright (c) 2011, IBM Corporation
// All rights reserved.
//
// Rolf Riesen, October 2011
//
#include <sst_config.h>
#include "routing.h"

#define ROUTE_DEBUG	(1)
#undef ROUTE_DEBUG



// There is a lot of hardcoded information about how the network is wired.
// the program genPatterns generates XML files with routers that have
// the following port connections:
// Port 0: East/Left
// Port 1: South/Down
// Port 2: West/Right
// Port 3: North/Up
// Port 4: Back
// Port 5: Front
// Port 6...: local links to cores or nodes

#define EAST_PORT		(0)
#define SOUTH_PORT		(1)
#define WEST_PORT		(2)
#define NORTH_PORT		(3)
#define BACK_PORT		(4)
#define FRONT_PORT		(5)
#define FIRST_LOCAL_PORT	(6)



// Create a source route from me (my core = my rank) and embed it into the event
// There a three cases:
//     On-node traffic that travels through the NoC
//     Off-node traffic that travels through the machine network
//     Off-node traffic that travels through one of the short cut links
//
void
Router::attach_route(CPUNicEvent *e, int dest_core)
{

int my_router, dest_router;


    if (_m->myNode() == _m->destNode(dest_core))   {
	// We're on the same node, go through the NoC
	my_router= _m->myCore() / _m->get_cores_per_NoC_router();
	dest_router= dest_core / _m->get_cores_per_NoC_router();


	gen_route(e, my_router, dest_router, _m->get_NoC_width(), _m->get_NoC_height(),
		_m->get_NoC_depth(), _m->get_NoCXwrap(), _m->get_NoCYwrap(), _m->get_NoCZwrap());

	// Exit to the detination local (NIC) pattern generator
	e->route.push_back(FIRST_LOCAL_PORT + (dest_core % _m->get_cores_per_NoC_router()));
	show_route("NoC", e, _m->myCore(), dest_core);

	// Right now this is only used by the power model in the router
	if (my_router != dest_router)   {
	    // This message travels through the NoC
	    e->local_traffic= false;
	} else   {
	    // This is a message among cores of the same router
	    e->local_traffic= true;
	}

    } else   {

	// Go through the global network, unless we have a far link
	if (_m->FarLinkExists(_m->destNode(dest_core)))   {
	    // On the network aggregator we'll have to go out the specified port to
	    // reach the appropriate far link
	    e->route.push_back(_m->FarLinkExitPort(_m->destNode(dest_core)));

	    // We come out at the aggregator on the destination node.
	    // Figure out which port to go out to reach the destination core
	    // Port 0 is where we came in
	    e->route.push_back(1 + (dest_core % _m->get_cores_per_node()));
	    show_route("Far", e, _m->myCore(), dest_core);

	} else   {

	    // On the network aggregator we'll have to go out port 0 to reach the mesh
	    e->route.push_back(0);

	    gen_route(e, _m->myNetRouter(), _m->destNetRouter(_m->destNode(dest_core)),
		_m->get_Net_width(), _m->get_Net_height(), _m->get_Net_depth(),
		_m->get_NetXwrap(), _m->get_NetYwrap(), _m->get_NetZwrap());

	    // Exit to the local node (aggregator on that node)
	    e->route.push_back(FIRST_LOCAL_PORT +
		    ((dest_core % (_m->get_num_router_nodes() * _m->get_cores_per_node())) /
		     _m->get_cores_per_node()));

	    // We come out at the aggregator on the destination node.
	    // Figure out which port to go out to reach the destination core
	    // Port 0 is where we came in
	    e->route.push_back(1 + (dest_core % _m->get_cores_per_node()));
	    show_route("Net", e, _m->myCore(), dest_core);
	}

	e->local_traffic= false;
    }

}  // end of attach_route()



void
Router::show_route(std::string net_name, CPUNicEvent *e, int src_rank, int dest_rank)
{

#if ROUTE_DEBUG
char route[128];
char tmp[128];
route[0]= 0;
tmp[0]= 0;
std::vector<int>::iterator itNum;

    for(itNum = e->route.begin(); itNum < e->route.end(); itNum++)   {
	sprintf(tmp, "%s.%d", route, *itNum);
	strcpy(route, tmp);
    }

    fprintf(stderr, "%s from %d --> %d, route %s\n", net_name.c_str(),
	 src_rank, dest_rank, route);
#endif

}  // end of show_route()



//
// Generate a route through a X * Y * Z torus
// We use it to traverse the machine network as well as the NoC
//
void
Router::gen_route(CPUNicEvent *e, int src, int dest, int width, int height, int depth,
	int x_wrap, int y_wrap, int z_wrap)
{

int c1, c2;
int my_X, my_Y, my_Z;
int dest_X, dest_Y, dest_Z;
int x_delta, y_delta, z_delta;


    // Calculate x, y and z coordinates within the Network or NoC
    my_Y= (src % (width * height)) / width;
    my_X= (src % (width * height)) % width;
    my_Z= src / (width * height);

    dest_Y= (dest % (width * height)) / width;
    dest_X= (dest % (width * height)) % width;
    dest_Z= dest / (width * height);

    // Use the shortest distance to reach the destination
    if (x_wrap)   {
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
    } else   {
	x_delta= dest_X - my_X;
    }

    if (y_wrap)   {
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
    } else   {
	y_delta= dest_Y - my_Y;
    }

    if (z_wrap)   {
	if (my_Z > dest_Z)   {
	    // How much does it cost to go back?
	    c1= my_Z - dest_Z;
	    // How much does it cost to go forward?
	    c2= (dest_Z + depth) - my_Z;
	} else   {
	    // How much does it cost to go back?
	    c1= (my_Z + depth) - dest_Z;
	    // How much does it cost to go forward?
	    c2= dest_Z - my_Z;
	}
	if (c1 > c2)   {
	    // go back
	    z_delta= c2;
	} else   {
	    // go forward
	    z_delta= -c1;
	}
    } else   {
	z_delta= dest_Z - my_Z;
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

    if (z_delta > 0)   {
	// Go BACK
	while (z_delta > 0)   {
	    e->route.push_back(BACK_PORT);
	    z_delta--;
	}
    } else if (z_delta < 0)   {
	// Go NORTH
	while (z_delta < 0)   {
	    e->route.push_back(FRONT_PORT);
	    z_delta++;
	}
    }

}  // end of gen_route()



