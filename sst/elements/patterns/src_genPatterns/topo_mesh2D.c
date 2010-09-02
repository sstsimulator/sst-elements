/*
** $Id: topo_mesh2D.c,v 1.5 2010/04/27 23:17:19 rolf Exp $
**
** Rolf Riesen, March 2010, Sandia National Laboratories
**
*/
#include <stdio.h>
#include "gen.h"
#include "topo_mesh2D.h"


#define EAST_PORT		(0)
#define SOUTH_PORT		(1)
#define WEST_PORT		(2)
#define NORTH_PORT		(3)
#define FIRST_LOCAL_PORT	(4)

#define GEN_DEBUG		1
void
GenMesh2D(int net_x_dim, int net_y_dim, int NoC_x_dim, int NoC_y_dim, int num_cores, int IO_nodes)
{

int x, y;
int R, r;
int me;
int gen;
int num_routers;
int first_NoC_router;
int all_cores;
int NoC_router;
int src, dest;
int rank;
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


    /* List the Net routers first; they each have 5 ports */
    if (net_x_dim * net_y_dim > 1)   {
	/* Only generate a network, if there are more than one node */
	for (R= 0; R < net_x_dim * net_y_dim; R++)   {
	    #if defined GEN_DEBUG
		fprintf(stderr, "gen_router(%3d, 5);\n", R);
	    #endif /* GEN_DEBUG */
	    gen_router(R, 5, wormhole);
	}
	if ((NoC_x_dim * NoC_y_dim > 1) || (num_cores > 1))   {
	    num_routers= net_x_dim * net_y_dim * NoC_x_dim * NoC_y_dim + (net_x_dim * net_y_dim);
	} else   {
	    num_routers= net_x_dim * net_y_dim;
	}
	first_NoC_router= net_x_dim * net_y_dim;
    } else   {
	num_routers= NoC_x_dim * NoC_y_dim;
	first_NoC_router= 0;
    }


    /* Now list the NoC routers; they have 4 + num_cores ports each */
    if ((NoC_x_dim * NoC_y_dim > 1) || (num_cores > 1))   {
	for (r= first_NoC_router; r < num_routers; r++)   {
	    #if defined GEN_DEBUG
		fprintf(stderr, "gen_router(%3d, %d);\n", r, 4 + num_cores);
	    #endif /* GEN_DEBUG */
	    gen_router(r, 4 + num_cores, flit_based);
	}
    }

    /* Generate an aggregator to route Net traffic to each core */
    if (net_x_dim * net_y_dim > 1)   {
	/* Only generate a network aggregator, if there are more than one node */
	net_aggregator= num_routers;
	for (R= 0; R < net_x_dim * net_y_dim; R++)   {
	    #if defined GEN_DEBUG
		fprintf(stderr, "gen_router(%3d, %2d); for net\n", net_aggregator, num_cores + 1);
	    #endif /* GEN_DEBUG */
	    gen_router(net_aggregator, num_cores + 1, wormhole);
	    net_aggregator++;
	}
    }

    /* Generate an aggregator to route data to a local NVRAM to and from each core */
    nvram_aggregator= num_routers + (net_x_dim * net_y_dim);
    for (R= 0; R < net_x_dim * net_y_dim; R++)   {
	#if defined GEN_DEBUG
	    fprintf(stderr, "gen_router(%3d, %2d); for nvram\n", nvram_aggregator, num_cores + 1);
	#endif /* GEN_DEBUG */
	gen_router(nvram_aggregator, num_cores + 1, flit_based);
	nvram_aggregator++;
    }

    /* Generate an aggregator to route data to stable storage to and from each core */
    ss_aggregator= num_routers + 2 * (net_x_dim * net_y_dim);
    for (R= 0; R < net_x_dim * net_y_dim; R++)   {
	#if defined GEN_DEBUG
	    fprintf(stderr, "gen_router(%3d, %2d); for stable storage\n", ss_aggregator,
		num_cores + 1);
	#endif /* GEN_DEBUG */
	gen_router(ss_aggregator, num_cores + 1, wormhole);
	ss_aggregator++;
    }


    /* Generate pattern generators and links between pattern generators and NoC routers */
    all_cores= num_routers * num_cores;
    #if defined GEN_DEBUG
	fprintf(stderr, "X %d, Y %d, x %d, y %d, routers %d, cores %d\n", NoC_x_dim, NoC_y_dim,
	    net_x_dim, net_y_dim, num_routers, all_cores);
    #endif /* GEN_DEBUG */
    net_aggregator= num_routers;
    nvram_aggregator= num_routers + (net_x_dim * net_y_dim);
    ss_aggregator= num_routers + 2 * (net_x_dim * net_y_dim);

    for (R= 0; R < net_x_dim * net_y_dim; R++)   {

	/* For each network router */
	net_aggregator_port= 1;
	nvram_aggregator_port= 1;
	ss_aggregator_port= 1;
	if ((NoC_x_dim * NoC_y_dim > 1) || (num_cores > 1))   {
	    for (r= 0; r < NoC_x_dim * NoC_y_dim; r++)   {
		/* For each NoC router */

		for (gen= 0; gen < num_cores; gen++)   {
		    /* For each core on a NoC router */
		    rank= (R * NoC_x_dim * NoC_y_dim * num_cores) +
			    (r * num_cores) + gen;
		    NoC_router= first_NoC_router + r + (R * NoC_x_dim * NoC_y_dim);
		    if (net_x_dim * net_y_dim > 1)   {
			/* Connect each core to its Net, NVRAM, and SS aggregator */
			#if defined GEN_DEBUG
			    fprintf(stderr, "gen_nic(rank %3d, router %2d, port %2d, agg %2d, "
				"agg port %2d); net\n", rank, NoC_router,
				FIRST_LOCAL_PORT + gen, net_aggregator, net_aggregator_port);
			#endif /* GEN_DEBUG */
			gen_nic(rank,
			    NoC_router, FIRST_LOCAL_PORT + gen,
			    net_aggregator, net_aggregator_port,
			    nvram_aggregator, nvram_aggregator_port,
			    ss_aggregator, ss_aggregator_port);
			net_aggregator_port++;
			nvram_aggregator_port++;
			ss_aggregator_port++;
		    } else   {
			/* Single node, no connection to a network */
			#if defined GEN_DEBUG
			    fprintf(stderr, "gen_nic(rank %3d, router %2d, port %2d, agg %2d, agg "
				"port %2d);\n", rank, NoC_router, FIRST_LOCAL_PORT + gen, -1, -1);
			#endif /* GEN_DEBUG */
			gen_nic(rank,
			    NoC_router, FIRST_LOCAL_PORT + gen,
			    -1, -1,
			    nvram_aggregator, nvram_aggregator_port,
			    ss_aggregator, ss_aggregator_port);
			nvram_aggregator_port++;
			ss_aggregator_port++;
		    }
		}
	    }

	} else   {

	    /* There is no NoC */
	    for (gen= 0; gen < num_cores; gen++)   {
		/* For each core on a NoC router */
		rank= (R * NoC_x_dim * NoC_y_dim * num_cores) + gen;
		NoC_router= (net_x_dim * net_y_dim) + (R * NoC_x_dim * NoC_y_dim);
		if (net_x_dim * net_y_dim > 1)   {
		    /* Connect each core to its Net aggregator */
		    gen_nic(rank,
			-1, -1,
			net_aggregator, net_aggregator_port,
			nvram_aggregator, nvram_aggregator_port,
			ss_aggregator, ss_aggregator_port);
		    #if defined GEN_DEBUG
			fprintf(stderr, "gen_nic(rank %3d, router %2d, port %2d, agg %2d, agg port "
			    "%2d);\n", rank, -1, -1, net_aggregator, net_aggregator_port);
		    #endif /* GEN_DEBUG */
		    net_aggregator_port++;
		    nvram_aggregator_port++;
		    ss_aggregator_port++;
		} else   {
		    /* No network and no NoC? */
		    #if defined GEN_DEBUG
			fprintf(stderr, "ERROR: Nothing to simulate with a single core!\n");
		    #endif /* GEN_DEBUG */
		    return;
		}
	    }
	}
	net_aggregator++;
	nvram_aggregator++;
	ss_aggregator++;
    }



    /* Generate links between network routers */
    if (net_x_dim * net_y_dim > 1)   {
	for (y= 0; y < net_y_dim; y++)   {
	    for (x= 0; x < net_x_dim; x++)   {
		me= y * net_x_dim + x;
		/* Go East */
		if ((me + 1) < (net_x_dim * (y + 1)))   {
		    gen_link(me, EAST_PORT, me + 1, WEST_PORT);
		} else   {
		    /* Wrap around */
		    gen_link(me, EAST_PORT, y * net_x_dim, WEST_PORT);
		}

		/* Go South */
		if ((me + net_x_dim) < (net_x_dim * net_y_dim))   {
		    gen_link(me, SOUTH_PORT, me + net_x_dim, NORTH_PORT);
		} else   {
		    /* Wrap around */
		    gen_link(me, SOUTH_PORT, x, NORTH_PORT);
		}
	    }
	}
    }


    /* Generate links between NoC routers */
    if ((NoC_x_dim * NoC_y_dim > 1) || (num_cores > 1))   {
	if (net_x_dim * net_y_dim > 1)   {
	    for (R= 0; R < net_x_dim * net_y_dim; R++)   {
		/* For each network router */

		for (y= 0; y < NoC_y_dim; y++)   {
		    for (x= 0; x < NoC_x_dim; x++)   {
			me= y * NoC_x_dim + x;
			src= (net_x_dim * net_y_dim) + me + (R * NoC_x_dim * NoC_y_dim);

			/* Go East */
			if ((me + 1) < (NoC_x_dim * (y + 1)))   {
			    dest= src + 1;
			    gen_link(src, EAST_PORT, dest, WEST_PORT);
			} else   {
			    /* Wrap around */
			    dest= (net_x_dim * net_y_dim) + (y * NoC_x_dim) + (R * NoC_x_dim * NoC_y_dim);
			    gen_link(src, EAST_PORT, dest, WEST_PORT);
			}

			/* Go South */
			if ((me + NoC_x_dim) < (NoC_x_dim * NoC_y_dim))   {
			    dest= src + NoC_x_dim;
			    gen_link(src, SOUTH_PORT, dest, NORTH_PORT);
			} else   {
			    /* Wrap around */
			    dest= (net_x_dim * net_y_dim) + x + (R * NoC_x_dim * NoC_y_dim);
			    gen_link(src, SOUTH_PORT, dest, NORTH_PORT);
			}
		    }
		}
	    }

	} else   {

	    /* No network, just a NoC */
	    for (y= 0; y < NoC_y_dim; y++)   {
		for (x= 0; x < NoC_x_dim; x++)   {
		    me= y * NoC_x_dim + x;
		    src= me;

		    /* Go East */
		    if ((me + 1) < (NoC_x_dim * (y + 1)))   {
			dest= src + 1;
			gen_link(src, EAST_PORT, dest, WEST_PORT);
		    } else   {
			/* Wrap around */
			dest= y * NoC_x_dim;
			gen_link(src, EAST_PORT, dest, WEST_PORT);
		    }

		    /* Go South */
		    if ((me + NoC_x_dim) < (NoC_x_dim * NoC_y_dim))   {
			dest= src + NoC_x_dim;
			gen_link(src, SOUTH_PORT, dest, NORTH_PORT);
		    } else   {
			/* Wrap around */
			dest= x;
			gen_link(src, SOUTH_PORT, dest, NORTH_PORT);
		    }
		}
	    }
	}
    }



    /* Generate the links between Network aggregators and the network routers */
    if (net_x_dim * net_y_dim > 1)   {
	/* Only generate a network aggregator, if there are more than one node */
	net_aggregator= num_routers;
	for (R= 0; R < net_x_dim * net_y_dim; R++)   {
	    #if defined GEN_DEBUG
		fprintf(stderr, "gen_link(From %2d, %2d, to %2d, %2d);\n", net_aggregator, 0,
		    R, FIRST_LOCAL_PORT);
	    #endif /* GEN_DEBUG */
	    gen_link(net_aggregator, 0, R, FIRST_LOCAL_PORT);
	    net_aggregator++;
	}
    }



    /*
    ** Generate one NVRAM component (a bit bucket) per node and connect the
    ** NVRAM aggregator to it.
    */
    nvram_aggregator= num_routers + (net_x_dim * net_y_dim);
    for (R= 0; R < net_x_dim * net_y_dim; R++)   {
	nvram_aggregator_port= 0;
	gen_nvram(R, nvram_aggregator++, nvram_aggregator_port, LOCAL_NVRAM);
    }



    /*
    ** Generate Stable Storage
    ** So far we have given each node an SS aggregator and connected all
    ** cores to it. Now we need to create a number of stable storage units
    ** (bit buckets) and connect the SS aggregators from a bunch of nodes
    ** to it.
    */

    /* Create N bit buckets attached to a router */
    IO_aggregator= num_routers + 3 * (net_x_dim * net_y_dim);
    for (R= 0; R < IO_nodes; R++)   {
	#if defined GEN_DEBUG
	    fprintf(stderr, "gen_router(%3d, %2d); for I/O nodes\n", IO_aggregator,
		(net_x_dim * net_y_dim / IO_nodes) + 1);
	#endif /* GEN_DEBUG */
	gen_router(IO_aggregator, (net_x_dim * net_y_dim / IO_nodes) + 1, wormhole);
	gen_nvram(R, IO_aggregator, 0, SSD); /* On port 0 */

	IO_aggregator++;
    }

    /*
    ** Attach (net_x_dim * net_y_dim / N) ss_aggregators to each of the N
    ** bit bucket routers
    */
    IO_aggregator= num_routers + 3 * (net_x_dim * net_y_dim);
    IO_aggregator_port= 1;
    ss_aggregator= num_routers + 2 * (net_x_dim * net_y_dim);

    for (R= 0; R < net_x_dim * net_y_dim; R++)   {
	/* For each node */
	#if defined GEN_DEBUG
	    fprintf(stderr, "gen_link(From %2d, %2d, to %2d, %2d); ss aggregator to SSD\n",
		ss_aggregator, 0, IO_aggregator, IO_aggregator_port);
	#endif /* GEN_DEBUG */
	gen_link(ss_aggregator, 0, IO_aggregator, IO_aggregator_port++);
	ss_aggregator++;
	if (((R + 1) % (net_x_dim * net_y_dim / IO_nodes)) == 0)    {
	    /* Switch to next I/I node */
	    IO_aggregator++;
	    IO_aggregator_port= 1;
	}
    }


}  /* end of GenMesh2D() */
