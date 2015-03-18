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
** $Id: ranks.c,v 1.1 2010-10-22 22:14:54 rolf Exp $
**
** Rolf Riesen, Sandia National Laboratories, July 2010
** Functions to map MPI ranks to global (distributed) memory regions.
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>
#if STANDALONECPLUSPLUS
extern "C" {
#endif
#include "ghost.h"
#include "ranks.h"

long int lrint(double x);


/*
** Sort such that width >= height >= depth
*/
static void
sort_dim(int A, int B, int C, int *width, int *height, int *depth)
{

    if ((A >= B) && (A >= C) && (B >= C))   {
	*width= A;
	*height= B;
	*depth= C;
    } else if ((A >= B) && (A >= C) && (B < C))   {
	*width= A;
	*height= C;
	*depth= B;
    } else if ((B >= A) && (B >= C) && (A >= C))   {
	*width= B;
	*height= A;
	*depth= C;
    } else if ((B >= A) && (B >= C) && (A < C))   {
	*width= B;
	*height= C;
	*depth= A;
    } else if ((C >= A) && (C >= B) && (A >= B))   {
	*width= C;
	*height= A;
	*depth= B;
    } else if ((C >= A) && (C >= B) && (A < B))   {
	*width= C;
	*height= B;
	*depth= A;
    } else   {
	fprintf(stderr, "We should never get here: file \"%s\", line %d\n", __FILE__, __LINE__);
	MPI_Finalize();
	exit(-1);
    }

}  /* end of sort_dim() */



/*
** If we are doing 2-D (z_dim == 0), assign approximately sqrt(n) ranks
** to each row. Use cube root for 3-D. Try to find integers that are as
** close to these ideals as possible.
*/
void
map_ranks(int num_ranks, int TwoD, int *width, int *height, int *depth)
{

int guess_width;	/* x */
int guess_height;	/* y */
int guess_depth;	/* z */


    if (TwoD)   {
	guess_width= lrint(sqrt(num_ranks));
	guess_height= guess_width;
	guess_depth= 1;
	if (guess_width * guess_height == num_ranks)   {
	    /* This works, we're done */
	    sort_dim(guess_width, guess_height, guess_depth, width, height, depth);
	    return;
	}

	/* Start increasing the width until we have an integer rectangle */
	while (guess_width <= num_ranks)   {
	    guess_height= num_ranks / guess_width;
	    if (guess_width * guess_height == num_ranks)   {
		/* This works, we're done */
		sort_dim(guess_width, guess_height, guess_depth, width, height, depth);
		return;
	    }
	    guess_width++;
	}

	fprintf(stderr, "We should never get here: file \"%s\", line %d\n", __FILE__, __LINE__);
	MPI_Finalize();
	exit(-1);

    } else   {
	guess_width= lrint(pow(num_ranks, 1.0 / 3.0));
	if (guess_width * guess_width * guess_width > num_ranks)   {
	    guess_width--;
	}
	guess_height= guess_width;
	guess_depth= guess_width;
	if (guess_width * guess_height * guess_height == num_ranks)   {
	    /* This works, we're done */
	    sort_dim(guess_width, guess_height, guess_depth, width, height, depth);
	    return;
	}

	/* Start increasing the width until we have an integer rectangle */
	while (guess_width <= num_ranks)   {
	    guess_depth= lrint(sqrt(num_ranks / guess_width));
	    guess_height= num_ranks / guess_width / guess_depth;
	    if (guess_width * guess_height * guess_depth == num_ranks)   {
		/* This works, we're done */
		sort_dim(guess_width, guess_height, guess_depth, width, height, depth);
		return;
	    }

	    while (guess_height <= num_ranks)   {
		guess_depth= num_ranks / guess_width / guess_height;
		if (guess_width * guess_height * guess_depth == num_ranks)   {
		    /* This works, we're done */
		    sort_dim(guess_width, guess_height, guess_depth, width, height, depth);
		    return;
		}
		guess_height++;
	    }
	    guess_width++;
	}

	fprintf(stderr, "We should never get here: file \"%s\", line %d\n", __FILE__, __LINE__);
	MPI_Finalize();
	exit(-1);
    }

}  /* end of mem_ranks() */



/*
** After deciding in mem_ranks() how many ranks should be in each dimension, we now
** need to figure out how many elements (floats, doubles, etc.) each rank should own.
** We do that here and also make sure that mem_ranks() was able to find a valid
** assignment.
*/
void
check_element_assignment(int verbose, int decomposition_only, int num_ranks, int width, int height, int depth,
	int my_rank, int *TwoD, int *x_dim, int *y_dim, int *z_dim)
{

    if (depth <= 1)   {
	if (height <= 1)   {
	    if (my_rank == 0)   {
		fprintf(stderr, "ERROR   Rank allocation is %d x %d x %d. Please chose a number of ranks\n",
		    width, height, depth);
		fprintf(stderr, "ERROR   such that a 2-D decomposition becomes possible!\n");
	    }
	    MPI_Finalize();
	    exit (-1);
	} else   {
	    /* The ranks have been arranged in a 2-D grid */
	    *TwoD= TRUE;
	    if (*x_dim < 0)   {
		/* Use the default */
		*x_dim= DEFAULT_2D_X_DIM;
	    }
	    if (*y_dim < 0)   {
		/* Use the default */
		*y_dim= DEFAULT_2D_Y_DIM;
	    }
	    *z_dim= 0;
	}
    } else   {
	/* The ranks have been arranged in a 3-D cube */
	*TwoD= FALSE;
	if (*x_dim < 0)   {
	    /* Use the default */
	    *x_dim= DEFAULT_3D_X_DIM;
	}
	if (*y_dim < 0)   {
	    /* Use the default */
	    *y_dim= DEFAULT_3D_Y_DIM;
	}
	if (*z_dim < 0)   {
	    /* Use the default */
	    *z_dim= DEFAULT_3D_Z_DIM;
	}
    }

    if (((my_rank == 0) && (verbose)) || ((my_rank == 0) && (decomposition_only)))   {
	printf("We are running on %d MPI ranks\n", num_ranks);
	if (*TwoD)   {
	    printf("Decomposing data into a 2-D %d x %d grid = %d rectangles (one per rank)\n",
		width, height, width * height);
	} else   {
	    printf("Decomposing data into a 3-D %d x %d x %d block = %d blocks (one per rank)\n",
		width, height, depth, width * height * depth);
	}
    }

}  /* end of check_element_assignment() */
#if STANDALONECPLUSPLUS
}
#endif
