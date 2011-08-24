/*
** $Id: neighbors.c,v 1.1 2010-10-22 22:14:54 rolf Exp $
**
** Rolf Riesen, Sandia National Laboratories, July 2010
** Functions to map MPI ranks to global (distributed) memory regions.
*/

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include "ghost.h"
#include "neighbors.h"



static int
rank(int x, int y, int z, int width, int height)
{
    return z * width * height + y * width + x;
}  /* end of rank() */



void
neighbors(int verbose, int me, int width, int height, int depth, neighbors_t *n)
{

int x, y, z;


    /* What are my X, Y, Z coordinates? */
    z= me / (width * height);
    y= (me - z * (width * height)) / width;
    x= (me - z * (width * height)) - y * width;

    /* Do a sanity check */
    if (me != rank(x, y, z, width, height))   {
	fprintf(stderr, "me %d != rank %d!!!\b", me, rank(x, y, z, width, height));
	MPI_Finalize();
	exit(-1);
    }

    /* I have four or six neighbors */
    n->right= rank((x + 1) % width, y, z, width, height);
    n->left= rank((x + width - 1) % width, y, z, width, height);
    n->down= rank(x, (y + 1) % height, z, width, height);
    n->up= rank(x, (y + height - 1) % height, z, width, height);

    if (depth > 1)   {
	n->front= rank(x, y, (z + depth - 1) % depth, width, height);
	n->back= rank(x, y, (z + 1) % depth, width, height);
    } else   {
	n->front= -1;
	n->back= -1;
    }

    if (verbose > 2)   {
	if (me == 0)   {
	    printf("N   me:     X   Y   Z              Left Rght Up   Down Back Frnt\n");
	}
	printf("N %4d:   %3d %3d %3d, neightbors: %4d %4d %4d %4d %4d %4d\n",
	    me, x, y, z, n->left, n->right, n->up, n->down, n->back, n->front);
    }

}  /* end of neighbors() */
