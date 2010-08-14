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
GenMesh2D(int dimX, int dimY, int doXwrap, int doYwrap, int num_cores)
{

int x, y;
int r;
int me;
int gen;


    /* List the routers */
    for (r= 0; r < dimX * dimY; r++)   {
	gen_router(r, 5);
    }

    /* Generate pattern generators and links between pattern generators and routers */
    for (r= 0; r < dimX * dimY; r++)   {
	for (gen= 0; gen < num_cores; gen++)   {
	    gen_nic(r * num_cores + gen, r, FIRST_LOCAL_PORT + gen);
	}
    }


    /* Generate links between routers */
    for (y= 0; y < dimY; y++)   {
	for (x= 0; x < dimX; x++)   {
	    me= y * dimX + x;
	    /* Go East */
	    if ((me + 1) < (dimX * (y + 1)))   {
		gen_link(me, EAST_PORT, me + 1, WEST_PORT);
	    } else   {
		/* Wrap around if specified */
		if (doXwrap)   {
		    gen_link(me, EAST_PORT, y * dimX, WEST_PORT);
		}
	    }

	    /* Go South */
	    if ((me + dimX) < (dimX * dimY))   {
		gen_link(me, SOUTH_PORT, me + dimX, NORTH_PORT);
	    } else   {
		/* Wrap around if specified */
		if (doYwrap)   {
		    gen_link(me, SOUTH_PORT, x, NORTH_PORT);
		}
	    }
	}
    }


}  /* end of GenMesh2D() */
