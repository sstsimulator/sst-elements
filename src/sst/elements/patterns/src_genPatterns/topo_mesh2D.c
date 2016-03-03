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

/*
** $Id: topo_mesh2D.c,v 1.5 2010/04/27 23:17:19 rolf Exp $
**
** Rolf Riesen, March 2010, Sandia National Laboratories
**
*/
#include <stdio.h>
#include <assert.h>
#include "farlink.h"
#include "gen.h"
#include "machine.h"
#include "topo_mesh2D.h"


#define EAST_PORT		(0)
#define SOUTH_PORT		(1)
#define WEST_PORT		(2)
#define NORTH_PORT		(3)
#define BACK_PORT		(4)
#define FRONT_PORT		(5)
#define FIRST_LOCAL_PORT	(6)

#define GEN_DEBUG		1
#undef GEN_DEBUG
void
GenMesh3D(int IO_nodes, int partition)
{

int x, y, z;
int R, r;
int N;
int me;
int right;
int row_start;
int south;
int col_start;
int back;
int plane_start;
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
int mpi_rank;
#if defined GEN_DEBUG
int all_cores;
#endif


    if (num_net_routers() >= partition)   {
	/* put num_net_routers / partition  onto each MPI rank */
    } else   {
	printf("# >>> Only have %d net routers, but %d MPI ranks\n",
	    num_net_routers(), partition);
	printf("# >>> Will only use %d MPI ranks\n", num_net_routers());
	partition= num_net_routers();
    }

    /* List the Net routers first; they each have 6 + num_router_nodes ports */
    RouterID= 0;
    for (R= 0; R < num_net_routers(); R++)   {
	#if defined GEN_DEBUG
	    fprintf(stderr, "gen_router(ID %3d, %d ports);\n", RouterID, 6 + num_router_nodes());
	#endif /* GEN_DEBUG */
	mpi_rank= R / (num_net_routers() / partition);
	gen_router(RouterID, 6 + num_router_nodes(), Rnet, wormhole, mpi_rank);
	RouterID++;
    }

    /*
    ** NOTE: We always generate the wrap around links, even if they were not specified.
    ** The routing algorithm then will simply not use them. This makes things here a lot
    ** simpler.
    */


    /* Now list the NoC routers on each node; they have 6 + num_router_cores ports each */
    first_NoC_router= RouterID;
    for (R= 0; R < num_net_routers(); R++)   {
	for (N= 0 ; N < num_router_nodes(); N++)   {
	    for (r= 0; r < num_NoC_routers(); r++)   {
		#if defined GEN_DEBUG
		    fprintf(stderr, "gen_router(ID %3d, %d ports);\n", RouterID, 6 + num_router_cores());
		#endif /* GEN_DEBUG */
		mpi_rank= R / (num_net_routers() / partition);
		gen_router(RouterID, 6 + num_router_cores(), RNoC, flit_based, mpi_rank);
		RouterID++;
	    }
	}
    }

    /* Generate an aggregator on each node to route Net traffic to each core */
    first_net_aggregator= RouterID;
    for (R= 0; R < num_net_routers(); R++)   {
	for (N= 0 ; N < num_router_nodes(); N++)   {
	    int nodeID= R * num_router_nodes() + N;
	    #if defined GEN_DEBUG
		fprintf(stderr, "Node %3d gen_router(ID %3d, %2d ports); for net with %d far links\n",
		    nodeID, RouterID, num_cores() + 1 + numFarPorts(nodeID), numFarPorts(nodeID));
	    #endif /* GEN_DEBUG */
	    mpi_rank= R / (num_net_routers() / partition);
	    gen_router(RouterID, num_cores() + 1 + numFarPorts(nodeID), RnetPort, wormhole, mpi_rank);
	    RouterID++;
	}
    }

    /* Generate an aggregator on each node to route data from each core to a local NVRAM */
    first_nvram_aggregator= RouterID;
    for (R= 0; R < num_net_routers(); R++)   {
	for (N= 0 ; N < num_router_nodes(); N++)   {
	    #if defined GEN_DEBUG
		fprintf(stderr, "gen_router(ID %3d, %2d ports); for nvram\n", RouterID, num_cores() + 1);
	    #endif /* GEN_DEBUG */
	    mpi_rank= R / (num_net_routers() / partition);
	    gen_router(RouterID, num_cores() + 1, Rnvram, flit_based, mpi_rank);
	    RouterID++;
	}
    }

    /* Generate an aggregator on each node to route data to stable storage from each core */
    first_ss_aggregator= RouterID;
    for (R= 0; R < num_net_routers(); R++)   {
	for (N= 0 ; N < num_router_nodes(); N++)   {
	    #if defined GEN_DEBUG
		fprintf(stderr, "gen_router(ID %3d, %2d ports); for stable storage\n", RouterID,
		    num_cores() + 1);
	    #endif /* GEN_DEBUG */
	    mpi_rank= R / (num_net_routers() / partition);
	    gen_router(RouterID, num_cores() + 1, Rstorage, wormhole, mpi_rank);
	    RouterID++;
	}
    }

    /* The next router will be used as an I/O aggregator further down. */
    first_IO_aggregator= RouterID;


    /* Generate pattern generators and links between pattern generators and NoC routers */
    #if defined GEN_DEBUG
	all_cores= num_nodes() * num_cores();
	fprintf(stderr, "X %d, Y %d, Z %d, x %d, y %d, z %d, routers %d, nodes %d, cores %d\n",
	    Net_x_dim(), Net_y_dim(), Net_z_dim(),
	    NoC_x_dim(), NoC_y_dim(), NoC_z_dim(),
	    RouterID, num_nodes(), all_cores);
    #endif /* GEN_DEBUG */
    net_aggregator= first_net_aggregator;
    nvram_aggregator= first_nvram_aggregator;
    ss_aggregator= first_ss_aggregator;
    NoC_router= first_NoC_router;
    coreID= 0;

    /* For each network router */
    for (R= 0; R < num_net_routers(); R++)   {

	/* For each node */
	for (N= 0 ; N < num_router_nodes(); N++)   {
	    net_aggregator_port= 1;
	    nvram_aggregator_port= 1;
	    ss_aggregator_port= 1;

	    /* For each NoC router */
	    for (r= 0; r < num_NoC_routers(); r++)   {

		/* For each core */
		for (gen= 0; gen < num_router_cores(); gen++)   {
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
    for (z= 0; z < Net_z_dim(); z++)   {
	for (y= 0; y < Net_y_dim(); y++)   {
	    for (x= 0; x < Net_x_dim(); x++)   {
		me= y * Net_x_dim() + x + z * Net_x_dim() * Net_y_dim();
		right= me + 1;
		row_start= y * Net_x_dim() + z * Net_x_dim() * Net_y_dim();
		south= me + Net_x_dim();
		col_start= x + z * Net_x_dim() * Net_y_dim();
		back= me + Net_x_dim() * Net_y_dim();
		plane_start= x + y * Net_x_dim();

		/* Go East */
		if ((x + 1) < Net_x_dim())   {
		    gen_link(me, EAST_PORT, right, WEST_PORT, Lnet);
		} else   {
		    /* Wrap around */
		    gen_link(me, EAST_PORT, row_start, WEST_PORT, Lnet);
		}

		/* Go South */
		if ((y + 1) < Net_y_dim())   {
		    gen_link(me, SOUTH_PORT, south, NORTH_PORT, Lnet);
		} else   {
		    /* Wrap around */
		    gen_link(me, SOUTH_PORT, col_start, NORTH_PORT, Lnet);
		}

		/* Go Back */
		if ((z + 1) < Net_z_dim())   {
		    gen_link(me, BACK_PORT, back, FRONT_PORT, Lnet);
		} else   {
		    /* Wrap around */
		    gen_link(me, BACK_PORT, plane_start, FRONT_PORT, Lnet);
		}
	    }
	}
    }


    /* Generate links between NoC routers */
    /* For each network router */
    NoC_router= first_NoC_router;
    for (R= 0; R < num_net_routers(); R++)   {

	/* For each node on a network router */
	for (N= 0 ; N < num_router_nodes(); N++)   {

	    for (z= 0; z < NoC_z_dim(); z++)   {
		for (y= 0; y < NoC_y_dim(); y++)   {
		    for (x= 0; x < NoC_x_dim(); x++)   {
			me= y * NoC_x_dim() + x + NoC_router + z * NoC_x_dim() * NoC_y_dim();
			right= me + 1;
			row_start= y * NoC_x_dim() + NoC_router + z * NoC_x_dim() * NoC_y_dim();
			south= me + NoC_x_dim();
			col_start= x + NoC_router + z * NoC_x_dim() * NoC_y_dim();
			back= me + NoC_x_dim() * NoC_y_dim();
			plane_start= x + NoC_router + y * NoC_x_dim();

			/* Go East */
			if ((x + 1) < NoC_x_dim())   {
			    #if defined GEN_DEBUG
				fprintf(stderr, "gen_link(%2d, %2d - %2d, %2d) NoC EAST\n",
				    me, EAST_PORT, right, WEST_PORT);
			    #endif /* GEN_DEBUG */
			    gen_link(me, EAST_PORT, right, WEST_PORT, LNoC);
			} else   {
			    /* Wrap around */
			    #if defined GEN_DEBUG
				fprintf(stderr, "gen_link(%2d, %2d - %2d, %2d) NoC EAST wrap\n",
				    me, EAST_PORT, row_start, WEST_PORT);
			    #endif /* GEN_DEBUG */
			    gen_link(me, EAST_PORT, row_start, WEST_PORT, LNoC);
			}

			/* Go South */
			if ((y + 1) < NoC_y_dim())   {
			    #if defined GEN_DEBUG
				fprintf(stderr, "gen_link(%2d, %2d - %2d, %2d) NoC SOUTH\n",
				    me, SOUTH_PORT, south, NORTH_PORT);
			    #endif /* GEN_DEBUG */
			    gen_link(me, SOUTH_PORT, south, NORTH_PORT, LNoC);
			} else   {
			    /* Wrap around */
			    #if defined GEN_DEBUG
				fprintf(stderr, "gen_link(%2d, %2d - %2d, %2d) NoC SOUTH wrap\n",
				    me, SOUTH_PORT, col_start, NORTH_PORT);
			    #endif /* GEN_DEBUG */
			    gen_link(me, SOUTH_PORT, col_start, NORTH_PORT, LNoC);
			}

			/* Go Back */
			if ((z + 1) < NoC_z_dim())   {
			    #if defined GEN_DEBUG
				fprintf(stderr, "gen_link(%2d, %2d - %2d, %2d) NoC SOUTH\n",
				    me, BACK_PORT, south, FRONT_PORT);
			    #endif /* GEN_DEBUG */
			    gen_link(me, BACK_PORT, back, FRONT_PORT, LNoC);
			} else   {
			    /* Wrap around */
			    #if defined GEN_DEBUG
				fprintf(stderr, "gen_link(%2d, %2d - %2d, %2d) NoC BACK wrap\n",
				    me, BACK_PORT, col_start, FRONT_PORT);
			    #endif /* GEN_DEBUG */
			    gen_link(me, BACK_PORT, plane_start, FRONT_PORT, LNoC);
			}
		    }
		}
	    }
	    NoC_router= NoC_router + num_NoC_routers();
	}
    }



    /* Generate the links between Network aggregators and the network routers */
    net_aggregator= first_net_aggregator;
    for (R= 0; R < num_net_routers(); R++)   {
	for (N= 0 ; N < num_router_nodes(); N++)   {
	    #if defined GEN_DEBUG
		fprintf(stderr, "gen_link(From %2d, %2d, to %2d, %2d);\n", net_aggregator, 0,
		    R, FIRST_LOCAL_PORT + N);
	    #endif /* GEN_DEBUG */
	    gen_link(net_aggregator, 0, R, FIRST_LOCAL_PORT + N, LnetAccess);
	    net_aggregator++;
	}
    }



    /* Generate far links */
    if (farlink_in_use())   {
	net_aggregator= first_net_aggregator;
	for (R= 0; R < num_net_routers(); R++)   {
	    for (N= 0 ; N < num_router_nodes(); N++)   {
		int src_node= R * num_router_nodes() + N;
		int Rdest, Ndest;

		for (Rdest= 0; Rdest < num_net_routers(); Rdest++)   {
		    for (Ndest= 0 ; Ndest < num_router_nodes(); Ndest++)   {
			int dest_node= Rdest * num_router_nodes() + Ndest;
			int dest_aggregator= first_net_aggregator + dest_node;

			/* Is there a link between these two nodes? */
			if (farlink_exists(src_node, dest_node))   {
			    #if defined GEN_DEBUG
				fprintf(stderr, "gen_far_link(From node %4d, R %2d, p %2d, to node %4d, R %2d, p %2d);\n",
				    src_node, net_aggregator, num_cores() + 1 + farlink_src_port(src_node, dest_node),
				    dest_node, dest_aggregator, num_cores() + 1 + farlink_dest_port(src_node, dest_node));
			    #endif /* GEN_DEBUG */
			    gen_link(net_aggregator, num_cores() + 1 + farlink_src_port(src_node, dest_node),
				dest_aggregator, num_cores() + 1 + farlink_dest_port(src_node, dest_node),
				LnetAccess);
			}
		    }
		}
		net_aggregator++;
	    }
	}
    }



    /*
    ** Generate one NVRAM component (a bit bucket) per node and connect
    ** each NVRAM aggregator to it.
    */
    nvram_aggregator= first_nvram_aggregator;
    nvramID= 0;
    nvram_aggregator_port= 0;
    for (R= 0; R < num_net_routers(); R++)   {
	for (N= 0 ; N < num_router_nodes(); N++)   {
	    #if defined GEN_DEBUG
		fprintf(stderr, "gen_nvram(R %d, nvram %d,%d, local %d);\n",
		    nvramID, nvram_aggregator, nvram_aggregator_port, LOCAL_NVRAM);
	    #endif /* GEN_DEBUG */
	    mpi_rank= R / (num_net_routers() / partition);
	    gen_nvram(nvramID, nvram_aggregator++, nvram_aggregator_port, LOCAL_NVRAM, mpi_rank);
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
	    fprintf(stderr, "gen_router(ID %3d, %2d ports); for I/O nodes\n", IO_aggregator,
		(num_net_routers() * num_router_nodes() / IO_nodes) + 1);
	#endif /* GEN_DEBUG */
	if (IO_nodes >= partition)   {
	    mpi_rank= R / (IO_nodes / partition);
	} else   {
	    mpi_rank= R % partition;
	}
	gen_router(IO_aggregator, (num_net_routers() * num_router_nodes() / IO_nodes) + 1,
	    RstoreIO, wormhole, mpi_rank);
	gen_nvram(R, IO_aggregator, 0, SSD, mpi_rank); /* On port 0 */

	IO_aggregator++;
    }

    /*
    ** Attach (num_net_routers() / N) ss_aggregators to each of the N
    ** bit-bucket routers
    */
    IO_aggregator= first_IO_aggregator;
    IO_aggregator_port= 1;
    ss_aggregator= first_ss_aggregator;

    for (R= 0; R < num_net_routers(); R++)   {
	for (N= 0 ; N < num_router_nodes(); N++)   {
	    #if defined GEN_DEBUG
		fprintf(stderr, "gen_link(From %2d, %2d, to %2d, %2d); ss aggregator to SSD\n",
		    ss_aggregator, 0, IO_aggregator, IO_aggregator_port);
	    #endif /* GEN_DEBUG */
	    gen_link(ss_aggregator, 0, IO_aggregator, IO_aggregator_port++, LIO);
	    ss_aggregator++;
	    if (((R + 1) % (num_net_routers() * num_router_nodes() / IO_nodes)) == 0)    {
		/* Switch to next I/0 node */
		IO_aggregator++;
		IO_aggregator_port= 1;
	    }
	}
    }


}  /* end of GenMesh3D() */
