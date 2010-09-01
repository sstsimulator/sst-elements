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


void
GenMesh2D(int net_x_dim, int net_y_dim, int NoC_x_dim, int NoC_y_dim, int num_cores)
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
int aggregator;
int aggregator_port;


    /* List the Net routers first; they each have 5 ports */
    if (net_x_dim * net_y_dim > 1)   {
	/* Only generate a network, if there are more than one node */
	for (R= 0; R < net_x_dim * net_y_dim; R++)   {
fprintf(stderr, "gen_router(%3d, 5);\n", R);
	    gen_router(R, 5);
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
fprintf(stderr, "gen_router(%3d, %d);\n", r, 4 + num_cores);
	    gen_router(r, 4 + num_cores);
	}
    }

    /* Generate an aggregator to route Net traffic to each core */
    if (net_x_dim * net_y_dim > 1)   {
	/* Only generate a network aggregator, if there are more than one node */
	aggregator= num_routers;
	for (R= 0; R < net_x_dim * net_y_dim; R++)   {
fprintf(stderr, "gen_router(%3d, %2d);\n", aggregator, num_cores + 1);
	    gen_router(aggregator, num_cores + 1);
	    aggregator++;
	}
    }


    /* Generate pattern generators and links between pattern generators and NoC routers */
    all_cores= num_routers * num_cores;
fprintf(stderr, "X %d, Y %d, x %d, y %d, routers %d, cores %d\n", NoC_x_dim, NoC_y_dim, net_x_dim, net_y_dim, num_routers, all_cores);
    aggregator= num_routers;
    for (R= 0; R < net_x_dim * net_y_dim; R++)   {
	/* For each network router */

	aggregator_port= 1;
	if ((NoC_x_dim * NoC_y_dim > 1) || (num_cores > 1))   {
	    for (r= 0; r < NoC_x_dim * NoC_y_dim; r++)   {
		/* For each NoC router */

		for (gen= 0; gen < num_cores; gen++)   {
		    /* For each core on a NoC router */
		    rank= (R * NoC_x_dim * NoC_y_dim * num_cores) +
			    (r * num_cores) + gen;
		    NoC_router= first_NoC_router + r + (R * NoC_x_dim * NoC_y_dim);
		    if (net_x_dim * net_y_dim > 1)   {
			/* Connect each core to its Net aggregator */
fprintf(stderr, "gen_nic(rank %3d, router %2d, port %2d, agg %2d, agg port %2d);\n", rank, NoC_router, FIRST_LOCAL_PORT + gen, aggregator, aggregator_port);
			gen_nic(rank, NoC_router, FIRST_LOCAL_PORT + gen, aggregator, aggregator_port);
			aggregator_port++;
		    } else   {
			/* Single node, no connection ot a network */
fprintf(stderr, "gen_nic(rank %3d, router %2d, port %2d, agg %2d, agg port %2d);\n", rank, NoC_router, FIRST_LOCAL_PORT + gen, -1, -1);
			gen_nic(rank, NoC_router, FIRST_LOCAL_PORT + gen, -1, -1);
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
		    gen_nic(rank, -1, -1, aggregator, aggregator_port);
fprintf(stderr, "gen_nic(rank %3d, router %2d, port %2d, agg %2d, agg port %2d);\n", rank, -1, -1, aggregator, aggregator_port);
		    aggregator_port++;
		} else   {
		    /* No network and no NoC? */
		    fprintf(stderr, "ERROR: Nothing to simulate with a single core!\n");
		    return;
		}
	    }
	}
	aggregator++;
    }
fprintf(stderr, "Done with pattern generators\n");




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
fprintf(stderr, "Done with network\n");


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
fprintf(stderr, "Done with GenMesh2D()\n");


}  /* end of GenMesh2D() */
