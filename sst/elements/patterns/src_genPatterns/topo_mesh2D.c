/*
** $Id: topo_mesh2D.c,v 1.5 2010/04/27 23:17:19 rolf Exp $
**
** Rolf Riesen, March 2010, Sandia National Laboratories
**
*/
#include <stdio.h>
#include <assert.h>
#include "gen.h"
#include "topo_mesh2D.h"


#define EAST_PORT		(0)
#define SOUTH_PORT		(1)
#define WEST_PORT		(2)
#define NORTH_PORT		(3)
#define FIRST_LOCAL_PORT	(4)

#define GEN_DEBUG		1
#undef GEN_DEBUG
void
GenMesh2D(int net_x_dim, int net_y_dim, int NoC_x_dim, int NoC_y_dim,
    int num_cores, int IO_nodes, int num_router_nodes)
{

int x, y;
int R, r;
int N;
int me;
int right;
int row_start;
int down;
int col_start;
int gen;
int RouterID;
int nvramID;
int first_NoC_router;
int first_net_aggregator;
int first_nvram_aggregator;
int first_ss_aggregator;
int first_IO_aggregator;
int NoC_router;
int coreID;
int net_aggregator;
int net_aggregator_port;
int nvram_aggregator;
int nvram_aggregator_port;
int ss_aggregator;
int ss_aggregator_port;
int IO_aggregator;
int IO_aggregator_port;
int flit_based= 0;
int wormhole= 1;
int cores_per_node;
#if defined GEN_DEBUG
int all_cores;
#endif


    assert((net_x_dim * net_y_dim) > 0);
    assert((NoC_x_dim * NoC_y_dim) > 0);
    assert(num_cores >= 1);
    assert(num_router_nodes >= 1);
    cores_per_node= num_cores * NoC_x_dim * NoC_y_dim;

    /* List the Net routers first; they each have 4 + num_router_nodes ports */
    RouterID= 0;
    for (R= 0; R < net_x_dim * net_y_dim; R++)   {
	#if defined GEN_DEBUG
	    fprintf(stderr, "gen_router(%3d, %d);\n", RouterID, 4 + num_router_nodes);
	#endif /* GEN_DEBUG */
	gen_router(RouterID, 4 + num_router_nodes, Rnet, wormhole);
	RouterID++;
    }


    /* Now list the NoC routers; they have 4 + num_cores ports each */
    first_NoC_router= RouterID;
    for (R= 0; R < net_x_dim * net_y_dim; R++)   {
	for (N= 0 ; N < num_router_nodes; N++)   {
	    for (r= 0; r < NoC_x_dim * NoC_y_dim; r++)   {
		#if defined GEN_DEBUG
		    fprintf(stderr, "gen_router(%3d, %d);\n", RouterID, 4 + num_cores);
		#endif /* GEN_DEBUG */
		gen_router(RouterID, 4 + num_cores, RNoC, flit_based);
		RouterID++;
	    }
	}
    }

    /* Generate an aggregator on each node to route Net traffic to each core */
    first_net_aggregator= RouterID;
    for (R= 0; R < net_x_dim * net_y_dim; R++)   {
	for (N= 0 ; N < num_router_nodes; N++)   {
	    #if defined GEN_DEBUG
		fprintf(stderr, "gen_router(%3d, %2d); for net\n", RouterID, num_cores + 1);
	    #endif /* GEN_DEBUG */
	    gen_router(RouterID, cores_per_node + 1, RnetPort, wormhole);
	    RouterID++;
	}
    }

    /* Generate an aggregator on each node to route data from each core to a local NVRAM */
    first_nvram_aggregator= RouterID;
    for (R= 0; R < net_x_dim * net_y_dim; R++)   {
	for (N= 0 ; N < num_router_nodes; N++)   {
	    #if defined GEN_DEBUG
		fprintf(stderr, "gen_router(%3d, %2d); for nvram\n", RouterID, num_cores + 1);
	    #endif /* GEN_DEBUG */
	    gen_router(RouterID, cores_per_node + 1, Rnvram, flit_based);
	    RouterID++;
	}
    }

    /* Generate an aggregator on each node to route data to stable storage from each core */
    first_ss_aggregator= RouterID;
    for (R= 0; R < net_x_dim * net_y_dim; R++)   {
	for (N= 0 ; N < num_router_nodes; N++)   {
	    #if defined GEN_DEBUG
		fprintf(stderr, "gen_router(%3d, %2d); for stable storage\n", RouterID,
		    num_cores + 1);
	    #endif /* GEN_DEBUG */
	    gen_router(RouterID, cores_per_node + 1, Rstorage, wormhole);
	    RouterID++;
	}
    }

    /* The next router will be used as an I/O aggregator further down. */
    first_IO_aggregator= RouterID;


    /* Generate pattern generators and links between pattern generators and NoC routers */
    #if defined GEN_DEBUG
	all_cores= net_x_dim * net_y_dim * num_router_nodes * NoC_x_dim * NoC_y_dim * num_cores;
	fprintf(stderr, "X %d, Y %d, x %d, y %d, routers %d, nodes %d, cores %d\n", NoC_x_dim, NoC_y_dim,
	    net_x_dim, net_y_dim, RouterID, net_x_dim * net_y_dim * num_router_nodes, all_cores);
    #endif /* GEN_DEBUG */
    net_aggregator= first_net_aggregator;
    nvram_aggregator= first_nvram_aggregator;
    ss_aggregator= first_ss_aggregator;
    NoC_router= first_NoC_router;
    coreID= 0;

    /* For each network router */
    for (R= 0; R < net_x_dim * net_y_dim; R++)   {

	/* For each node */
	for (N= 0 ; N < num_router_nodes; N++)   {
	    net_aggregator_port= 1;
	    nvram_aggregator_port= 1;
	    ss_aggregator_port= 1;

	    /* For each NoC router */
	    for (r= 0; r < NoC_x_dim * NoC_y_dim; r++)   {

		/* For each core */
		for (gen= 0; gen < num_cores; gen++)   {
		    /* Connect each core to its Net, NVRAM, and SS aggregator */
		    #if defined GEN_DEBUG
			fprintf(stderr, "gen_nic(coreID %3d, router %2d, port %2d, agg %2d, "
			    "agg port %2d); net\n", coreID, NoC_router,
			    FIRST_LOCAL_PORT + gen, net_aggregator, net_aggregator_port);
		    #endif /* GEN_DEBUG */
		    gen_nic(coreID,
			NoC_router, FIRST_LOCAL_PORT + gen,
			net_aggregator, net_aggregator_port,
			nvram_aggregator, nvram_aggregator_port,
			ss_aggregator, ss_aggregator_port);
		    net_aggregator_port++;
		    nvram_aggregator_port++;
		    ss_aggregator_port++;
		    coreID++;
		}
		NoC_router++;
	    }
	    net_aggregator++;
	    nvram_aggregator++;
	    ss_aggregator++;
	}
    }



    /* Generate links between network routers */
    for (y= 0; y < net_y_dim; y++)   {
	for (x= 0; x < net_x_dim; x++)   {
	    me= y * net_x_dim + x;
	    right= me + 1;
	    row_start= y * net_x_dim;
	    down= me + net_x_dim;
	    col_start= x;

	    /* Go East */
	    if ((x + 1) < net_x_dim)   {
		gen_link(me, EAST_PORT, right, WEST_PORT);
	    } else   {
		/* Wrap around */
		gen_link(me, EAST_PORT, row_start, WEST_PORT);
	    }

	    /* Go South */
	    if ((y + 1) < net_y_dim)   {
		gen_link(me, SOUTH_PORT, down, NORTH_PORT);
	    } else   {
		/* Wrap around */
		gen_link(me, SOUTH_PORT, col_start, NORTH_PORT);
	    }
	}
    }


    /* Generate links between NoC routers */
    /* For each network router */
    NoC_router= first_NoC_router;
    for (R= 0; R < net_x_dim * net_y_dim; R++)   {

	/* For each node on a network router */
	for (N= 0 ; N < num_router_nodes; N++)   {

	    for (y= 0; y < NoC_y_dim; y++)   {
		for (x= 0; x < NoC_x_dim; x++)   {
		    me= y * NoC_x_dim + x + NoC_router;
		    right= me + 1;
		    row_start= y * NoC_x_dim + NoC_router;
		    down= me + NoC_x_dim;
		    col_start= x + NoC_router;

		    /* Go East */
		    if ((x + 1) < NoC_x_dim)   {
			#if defined GEN_DEBUG
			    fprintf(stderr, "gen_link(%2d, %2d - %2d, %2d) NoC EAST\n",
				me, EAST_PORT, right, WEST_PORT);
			#endif /* GEN_DEBUG */
			gen_link(me, EAST_PORT, right, WEST_PORT);
		    } else   {
			/* Wrap around */
			#if defined GEN_DEBUG
			    fprintf(stderr, "gen_link(%2d, %2d - %2d, %2d) NoC EAST wrap\n",
				me, EAST_PORT, row_start, WEST_PORT);
			#endif /* GEN_DEBUG */
			gen_link(me, EAST_PORT, row_start, WEST_PORT);
		    }

		    /* Go South */
		    if ((y + 1) < NoC_y_dim)   {
			#if defined GEN_DEBUG
			    fprintf(stderr, "gen_link(%2d, %2d - %2d, %2d) NoC SOUTH\n",
				me, SOUTH_PORT, down, NORTH_PORT);
			#endif /* GEN_DEBUG */
			gen_link(me, SOUTH_PORT, down, NORTH_PORT);
		    } else   {
			/* Wrap around */
			#if defined GEN_DEBUG
			    fprintf(stderr, "gen_link(%2d, %2d - %2d, %2d) NoC SOUTH wrap\n",
				me, SOUTH_PORT, col_start, NORTH_PORT);
			#endif /* GEN_DEBUG */
			gen_link(me, SOUTH_PORT, col_start, NORTH_PORT);
		    }

		}
	    }
	    NoC_router= NoC_router + NoC_x_dim * NoC_y_dim;
	}
    }



    /* Generate the links between Network aggregators and the network routers */
    net_aggregator= first_net_aggregator;
    for (R= 0; R < net_x_dim * net_y_dim; R++)   {
	for (N= 0 ; N < num_router_nodes; N++)   {
	    #if defined GEN_DEBUG
		fprintf(stderr, "gen_link(From %2d, %2d, to %2d, %2d);\n", net_aggregator, 0,
		    R, FIRST_LOCAL_PORT + N);
	    #endif /* GEN_DEBUG */
	    gen_link(net_aggregator, 0, R, FIRST_LOCAL_PORT + N);
	    net_aggregator++;
	}
    }



    /*
    ** Generate one NVRAM component (a bit bucket) per node and connect
    ** each NVRAM aggregator to it.
    */
    nvram_aggregator= first_nvram_aggregator;
    nvramID= 0;
    nvram_aggregator_port= 0;
    for (R= 0; R < net_x_dim * net_y_dim; R++)   {
	for (N= 0 ; N < num_router_nodes; N++)   {
	    #if defined GEN_DEBUG
		fprintf(stderr, "gen_nvram(R %d, nvram %d,%d, local %d);\n",
		    nvramID, nvram_aggregator, nvram_aggregator_port, LOCAL_NVRAM);
	    #endif /* GEN_DEBUG */
	    gen_nvram(nvramID, nvram_aggregator++, nvram_aggregator_port, LOCAL_NVRAM);
	    nvramID++;
	}
    }



    /*
    ** Generate Stable Storage
    ** So far we have given each node an SS aggregator and connected all
    ** cores to it. Now we need to create a number of stable storage units
    ** (bit buckets) and connect the SS aggregators from a bunch of nodes
    ** to it.
    */

    /* Create N bit buckets attached to a router */
    IO_aggregator= first_IO_aggregator;
    for (R= 0; R < IO_nodes; R++)   {
	#if defined GEN_DEBUG
	    fprintf(stderr, "gen_router(%3d, %2d); for I/O nodes\n", IO_aggregator,
		(net_x_dim * net_y_dim * num_router_nodes / IO_nodes) + 1);
	#endif /* GEN_DEBUG */
	gen_router(IO_aggregator, (net_x_dim * net_y_dim * num_router_nodes / IO_nodes) + 1,
	    RstoreIO, wormhole);
	gen_nvram(R, IO_aggregator, 0, SSD); /* On port 0 */

	IO_aggregator++;
    }

    /*
    ** Attach (net_x_dim * net_y_dim / N) ss_aggregators to each of the N
    ** bit-bucket routers
    */
    IO_aggregator= first_IO_aggregator;
    IO_aggregator_port= 1;
    ss_aggregator= first_ss_aggregator;

    for (R= 0; R < net_x_dim * net_y_dim; R++)   {
	for (N= 0 ; N < num_router_nodes; N++)   {
	    #if defined GEN_DEBUG
		fprintf(stderr, "gen_link(From %2d, %2d, to %2d, %2d); ss aggregator to SSD\n",
		    ss_aggregator, 0, IO_aggregator, IO_aggregator_port);
	    #endif /* GEN_DEBUG */
	    gen_link(ss_aggregator, 0, IO_aggregator, IO_aggregator_port++);
	    ss_aggregator++;
	    if (((R + 1) % (net_x_dim * net_y_dim * num_router_nodes / IO_nodes)) == 0)    {
		/* Switch to next I/0 node */
		IO_aggregator++;
		IO_aggregator_port= 1;
	    }
	}
    }


}  /* end of GenMesh2D() */
