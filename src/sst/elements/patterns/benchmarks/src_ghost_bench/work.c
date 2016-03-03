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
** $Id: work.c,v 1.5 2010-10-26 16:33:36 rolf Exp $
**
** Rolf Riesen, Sandia National Laboratories, July 2010
** Routines to do the work and the ghost cell exchanges.
*/

#include <stdio.h>
#include <mpi.h>
#include <math.h>	/* For trunk(), log(), lrint */
#include <unistd.h>	/* For usleep() */
#include <stdlib.h>	/* For drand48() */
#if STANDALONECPLUSPLUS
extern "C" {
#endif
#include "ghost.h"
#include "work.h"


double trunc(double x);
long int lrint(double x);


/* Local functions */
static void do_delay(double compute_time, double delay, int imbalance);


/* Use these MPI tags */
#define TAG_UP		(200)
#define TAG_DOWN	(201)
#define TAG_LEFT	(202)
#define TAG_RIGHT	(203)
#define TAG_FRONT	(204)
#define TAG_BACK	(205)



void
exchange_ghosts(int TwoD, mem_ptr_t *m, neighbors_t *n, long long int *bytes_sent, long long int *num_sends)
{

MPI_Request req[12];


    if (TwoD)   {
	/* Post the receives */
	MPI_Irecv(m->x_top_ghost,    m->x_dim, MPI_ELEMENT_TYPE, n->up,    TAG_DOWN, MPI_COMM_WORLD, &(req[0]));
	MPI_Irecv(m->x_bottom_ghost, m->x_dim, MPI_ELEMENT_TYPE, n->down,  TAG_UP, MPI_COMM_WORLD, &(req[1]));

	MPI_Irecv(m->y_top_ghost,    m->y_dim, MPI_ELEMENT_TYPE, n->left,  TAG_RIGHT, MPI_COMM_WORLD, &(req[2]));
	MPI_Irecv(m->y_bottom_ghost, m->y_dim, MPI_ELEMENT_TYPE, n->right, TAG_LEFT, MPI_COMM_WORLD, &(req[3]));

	req[4]= MPI_REQUEST_NULL;
	req[5]= MPI_REQUEST_NULL;

	/* Do the sends */
	MPI_Isend(m->x_top_interior1,    m->x_dim, MPI_ELEMENT_TYPE, n->up,    TAG_UP, MPI_COMM_WORLD, &(req[6]));
	MPI_Isend(m->x_bottom_interior1, m->x_dim, MPI_ELEMENT_TYPE, n->down,  TAG_DOWN, MPI_COMM_WORLD, &(req[7]));

	MPI_Isend(m->y_top_interior1,    m->y_dim, MPI_ELEMENT_TYPE, n->left,  TAG_LEFT, MPI_COMM_WORLD, &(req[8]));
	MPI_Isend(m->y_bottom_interior1, m->y_dim, MPI_ELEMENT_TYPE, n->right, TAG_RIGHT, MPI_COMM_WORLD, &(req[9]));

	req[10]= MPI_REQUEST_NULL;
	req[11]= MPI_REQUEST_NULL;

	*bytes_sent += 2 * m->x_dim * sizeof(ELEMENT_TYPE) + 2 * m->y_dim * sizeof(ELEMENT_TYPE);
	*num_sends += 4;

    } else   {

	/* Post the receives */
	MPI_Irecv(m->x_top_ghost,    m->x_dim * m->y_dim, MPI_ELEMENT_TYPE, n->up,    TAG_DOWN, MPI_COMM_WORLD, &(req[0]));
	MPI_Irecv(m->x_bottom_ghost, m->x_dim * m->y_dim, MPI_ELEMENT_TYPE, n->down,  TAG_UP, MPI_COMM_WORLD, &(req[1]));

	MPI_Irecv(m->y_top_ghost,    m->y_dim * m->z_dim, MPI_ELEMENT_TYPE, n->left,  TAG_RIGHT, MPI_COMM_WORLD, &(req[2]));
	MPI_Irecv(m->y_bottom_ghost, m->y_dim * m->z_dim, MPI_ELEMENT_TYPE, n->right, TAG_LEFT, MPI_COMM_WORLD, &(req[3]));

	MPI_Irecv(m->z_top_ghost,    m->x_dim * m->z_dim, MPI_ELEMENT_TYPE, n->back,  TAG_FRONT, MPI_COMM_WORLD, &(req[4]));
	MPI_Irecv(m->z_bottom_ghost, m->x_dim * m->z_dim, MPI_ELEMENT_TYPE, n->front, TAG_BACK, MPI_COMM_WORLD, &(req[5]));

	/* Do the sends */
	MPI_Isend(m->x_top_interior1,    m->x_dim * m->y_dim, MPI_ELEMENT_TYPE, n->up,    TAG_UP, MPI_COMM_WORLD, &(req[6]));
	MPI_Isend(m->x_bottom_interior1, m->x_dim * m->y_dim, MPI_ELEMENT_TYPE, n->down,  TAG_DOWN, MPI_COMM_WORLD, &(req[7]));

	MPI_Isend(m->y_top_interior1,    m->y_dim * m->z_dim, MPI_ELEMENT_TYPE, n->left,  TAG_LEFT, MPI_COMM_WORLD, &(req[8]));
	MPI_Isend(m->y_bottom_interior1, m->y_dim * m->z_dim, MPI_ELEMENT_TYPE, n->right, TAG_RIGHT, MPI_COMM_WORLD, &(req[9]));

	MPI_Isend(m->z_top_interior1,    m->x_dim * m->z_dim, MPI_ELEMENT_TYPE, n->back,  TAG_BACK, MPI_COMM_WORLD, &(req[10]));
	MPI_Isend(m->z_bottom_interior1, m->x_dim * m->z_dim, MPI_ELEMENT_TYPE, n->front, TAG_FRONT, MPI_COMM_WORLD, &(req[11]));
	*bytes_sent += 2 * m->x_dim * m->y_dim * sizeof(ELEMENT_TYPE) +
	               2 * m->y_dim * m->z_dim * sizeof(ELEMENT_TYPE) +
	               2 * m->x_dim * m->z_dim * sizeof(ELEMENT_TYPE);
	*num_sends += 6;
    }

    /* Wait for all exchanges to complete */
    MPI_Waitall(12, req, MPI_STATUSES_IGNORE);

}  /* end of exchange_ghosts() */



void
compute(int TwoD, mem_ptr_t *m, long long int *fop_cnt, int max_loop, double delay, int imbalance)
{

int i, j;
int loop;
double start, end;


    start= MPI_Wtime();
    if (TwoD)   {
	/* Do some computation accessing the ghost cells in a strided manner, read-only, and only once */
	for (i= 1; i < m->x_dim - 1; i++)   {
	    m->x_top_interior1[i]= 4.0 * m->x_top_interior1[i] - (m->x_top_ghost[i] +
		m->x_top_interior2[i] + m->x_top_interior1[i - 1] + m->x_top_interior1[i + 1]);
	    m->x_bottom_interior1[i]= 4.0 * m->x_bottom_interior1[i] - (m->x_bottom_ghost[i] +
		m->x_bottom_interior2[i] + m->x_bottom_interior1[i - 1] + m->x_bottom_interior1[i + 1]);
	    *fop_cnt += (2 * 5);
	}

	for (i= 1; i < m->y_dim - 1; i++)   {
	    m->y_top_interior1[i]= 4.0 * m->y_top_interior1[i] - (m->y_top_ghost[i] +
		m->y_top_interior2[i] + m->y_top_interior1[i - 1] + m->y_top_interior1[i + 1]);
	    m->y_bottom_interior1[i]= 4.0 * m->y_bottom_interior1[i] - (m->y_bottom_ghost[i] +
		m->y_bottom_interior2[i] + m->y_bottom_interior1[i - 1] + m->y_bottom_interior1[i + 1]);
	    *fop_cnt += (2 * 5);
	}


	/* Now do some more computations to spend time */
	for (loop= 0; loop < max_loop; loop++)   {
	    for (i= 1; i < m->x_dim - 1; i++)   {
		m->x_top_interior2[i]= 3.0 * m->x_top_interior2[i] - (m->x_top_interior1[i] +
		    m->x_top_interior2[i - 1] + m->x_top_interior2[i + 1]);
		m->x_bottom_interior2[i]= 3.0 * m->x_bottom_interior2[i] - (m->x_bottom_interior1[i] +
		    m->x_bottom_interior2[i - 1] + m->x_bottom_interior2[i + 1]);
		*fop_cnt += (2 * 4);
	    }

	    for (i= 1; i < m->y_dim - 1; i++)   {
		m->y_top_interior2[i]= 3.0 * m->y_top_interior2[i] - (m->y_top_interior1[i] +
		    m->y_top_interior2[i - 1] + m->y_top_interior2[i + 1]);
		m->y_bottom_interior2[i]= 3.0 * m->y_bottom_interior2[i] - (m->y_bottom_interior1[i] +
		    m->y_bottom_interior2[i - 1] + m->y_bottom_interior2[i + 1]);
		*fop_cnt += (2 * 4);
	    }
	}


	end= MPI_Wtime();
	do_delay(end - start, delay, imbalance);


	/* Update the ghost cells */
	for (i= 0; i < m->x_dim; i++)   {
	    m->x_top_ghost[i]= TRUNC_ELEMENT(m->x_top_interior1[i]);
	    m->x_bottom_ghost[i]= TRUNC_ELEMENT(m->x_bottom_interior1[i]);
	}

	for (i= 0; i < m->y_dim; i++)   {
	    m->y_top_ghost[i]= TRUNC_ELEMENT(m->y_top_interior1[i]);
	    m->y_bottom_ghost[i]= TRUNC_ELEMENT(m->y_bottom_interior1[i]);
	}

	if (m->z_dim > 0)   {
	    for (i= 0; i < m->z_dim; i++)   {
		m->z_top_ghost[i]= TRUNC_ELEMENT(m->z_top_interior1[i]);
		m->z_bottom_ghost[i]= TRUNC_ELEMENT(m->z_bottom_interior1[i]);
	    }
	}

    } else   {

	/* Do some computation accessing the ghost cells in a strided manner, read-only, and only once */
	for (i= 1; i < m->x_dim - 1; i++)   {
	    for (j= 1; j < m->y_dim - 1; j++)   {
		m->x_top_interior1[i + j * m->x_dim]= 6.0 * m->x_top_interior1[i + j * m->x_dim] -
		    (m->x_top_ghost[i + j * m->x_dim] +
		     m->x_top_interior2[i + j * m->x_dim] +
		     m->x_top_interior1[i + j * m->x_dim - 1] +
		     m->x_top_interior1[i + j * m->x_dim + 1] +
		     m->x_top_interior1[i + j * m->x_dim - m->x_dim] +
		     m->x_top_interior1[i + j * m->x_dim + m->x_dim]);
		m->x_bottom_interior1[i + j * m->x_dim]= 6.0 * m->x_bottom_interior1[i + j * m->x_dim] -
		    (m->x_bottom_ghost[i + j * m->x_dim] +
		     m->x_bottom_interior2[i] +
		     m->x_bottom_interior1[i + j * m->x_dim - 1] +
		     m->x_bottom_interior1[i + j * m->x_dim + 1] +
		     m->x_bottom_interior1[i + j * m->x_dim - m->x_dim] +
		     m->x_bottom_interior1[i + j * m->x_dim + m->x_dim]);
		*fop_cnt += (2 * 7);
	    }
	}

	for (i= 1; i < m->y_dim - 1; i++)   {
	    for (j= 1; j < m->z_dim - 1; j++)   {
		m->y_top_interior1[i + j * m->y_dim]= 6.0 * m->y_top_interior1[i + j * m->y_dim] -
		    (m->y_top_ghost[i + j * m->y_dim] +
		     m->y_top_interior2[i + j * m->y_dim] +
		     m->y_top_interior1[i + j * m->y_dim - 1] +
		     m->y_top_interior1[i + j * m->y_dim + 1] +
		     m->y_top_interior1[i + j * m->y_dim - m->y_dim] +
		     m->y_top_interior1[i + j * m->y_dim + m->y_dim]);
		m->y_bottom_interior1[i + j * m->y_dim]= 6.0 * m->y_bottom_interior1[i + j * m->y_dim] -
		    (m->y_bottom_ghost[i + j * m->y_dim] +
		     m->y_bottom_interior2[i + j * m->y_dim] +
		     m->y_bottom_interior1[i + j * m->y_dim - 1] +
		     m->y_bottom_interior1[i + j * m->y_dim + 1] +
		     m->y_bottom_interior1[i + j * m->y_dim - m->y_dim] +
		     m->y_bottom_interior1[i + j * m->y_dim + m->y_dim]);
		*fop_cnt += (2 * 7);
	    }
	}

	for (i= 1; i < m->x_dim - 1; i++)   {
	    for (j= 1; j < m->z_dim - 1; j++)   {
		m->z_top_interior1[i + j * m->x_dim]= 6.0 * m->z_top_interior1[i + j * m->x_dim] -
		    (m->z_top_ghost[i + j * m->x_dim] +
		     m->z_top_interior2[i + j * m->x_dim] +
		     m->z_top_interior1[i + j * m->x_dim - 1] +
		     m->z_top_interior1[i + j * m->x_dim + 1] +
		     m->z_top_interior1[i + j * m->x_dim - m->x_dim] +
		     m->z_top_interior1[i + j * m->x_dim + m->x_dim]);
		m->z_bottom_interior1[i + j * m->x_dim]= 6.0 * m->z_bottom_interior1[i + j * m->x_dim] -
		    (m->z_bottom_ghost[i + j * m->x_dim] +
		     m->z_bottom_interior2[i + j * m->x_dim] +
		     m->z_bottom_interior1[i + j * m->x_dim - 1] +
		     m->z_bottom_interior1[i + j * m->x_dim + 1] +
		     m->y_bottom_interior1[i + j * m->x_dim - m->x_dim] +
		     m->y_bottom_interior1[i + j * m->x_dim + m->x_dim]);
		*fop_cnt += (2 * 7);
	    }
	}


	end= MPI_Wtime();
	do_delay(end - start, delay, imbalance);


	/* Now do some more computations to spend time */
	for (loop= 0; loop < max_loop; loop++)   {
	    for (i= 1; i < m->x_dim - 1; i++)   {
		for (j= 1; j < m->y_dim - 1; j++)   {
		    m->x_top_interior2[i + j * m->x_dim]= 5.0 * m->x_top_interior2[i + j * m->x_dim] -
			(m->x_top_interior1[i + j * m->x_dim] +
			 m->x_top_interior2[i + j * m->x_dim - 1] +
			 m->x_top_interior2[i + j * m->x_dim + 1] +
			 m->z_top_interior2[i + j * m->x_dim - m->x_dim] +
			 m->z_top_interior2[i + j * m->x_dim + m->x_dim]);
		    m->x_bottom_interior2[i + j * m->x_dim]= 5.0 * m->x_bottom_interior2[i + j * m->x_dim] -
			(m->x_bottom_interior1[i + j * m->x_dim] +
			 m->x_bottom_interior2[i + j * m->x_dim - 1] +
			 m->x_bottom_interior2[i + j * m->x_dim + 1] +
			 m->z_bottom_interior2[i + j * m->x_dim - m->x_dim] +
			 m->z_bottom_interior2[i + j * m->x_dim + m->x_dim]);
		    *fop_cnt += (2 * 6);
		}
	    }

	    for (i= 1; i < m->y_dim - 1; i++)   {
		for (j= 1; j < m->z_dim - 1; j++)   {
		    m->y_top_interior2[i + j * m->y_dim]= 5.0 * m->y_top_interior2[i + j * m->y_dim] -
			(m->y_top_interior1[i + j * m->y_dim] +
			 m->y_top_interior2[i + j * m->y_dim - 1] +
			 m->y_top_interior2[i + j * m->y_dim + 1] +
			 m->z_top_interior2[i + j * m->y_dim - m->y_dim] +
			 m->z_top_interior2[i + j * m->y_dim + m->y_dim]);
		    m->y_bottom_interior2[i + j * m->y_dim]= 5.0 * m->y_bottom_interior2[i + j * m->y_dim] -
			(m->y_bottom_interior1[i + j * m->y_dim] +
			 m->y_bottom_interior2[i + j * m->y_dim - 1] +
			 m->y_bottom_interior2[i + j * m->y_dim + 1] +
			 m->z_bottom_interior2[i + j * m->y_dim - m->y_dim] +
			 m->z_bottom_interior2[i + j * m->y_dim + m->y_dim]);
		    *fop_cnt += (2 * 6);
		}
	    }

	    for (i= 1; i < m->x_dim - 1; i++)   {
		for (j= 1; j < m->z_dim - 1; j++)   {
		    m->z_top_interior2[i + j * m->x_dim]= 5.0 * m->z_top_interior2[i + j * m->x_dim] -
			(m->z_top_interior1[i + j * m->x_dim] +
			 m->z_top_interior2[i + j * m->x_dim - 1] +
			 m->z_top_interior2[i + j * m->x_dim + 1] +
			 m->z_top_interior2[i + j * m->x_dim - m->x_dim] +
			 m->z_top_interior2[i + j * m->x_dim + m->x_dim]);
		    m->z_bottom_interior2[i + j * m->x_dim]= 5.0 * m->z_bottom_interior2[i + j * m->x_dim] -
			(m->z_bottom_interior1[i + j * m->x_dim] +
			 m->z_bottom_interior2[i + j * m->x_dim - 1] +
			 m->z_bottom_interior2[i + j * m->x_dim + 1] +
			 m->z_bottom_interior2[i + j * m->x_dim - m->x_dim] +
			 m->z_bottom_interior2[i + j * m->x_dim + m->x_dim]);
		    *fop_cnt += (2 * 6);
		}
	    }
	}


	/* Update the ghost cells */
	for (i= 0; i < m->x_dim * m->y_dim; i++)   {
	    m->x_top_ghost[i]= TRUNC_ELEMENT(m->x_top_interior1[i]);
	    m->x_bottom_ghost[i]= TRUNC_ELEMENT(m->x_bottom_interior1[i]);
	}

	for (i= 0; i < m->y_dim * m->z_dim; i++)   {
	    m->y_top_ghost[i]= TRUNC_ELEMENT(m->y_top_interior1[i]);
	    m->y_bottom_ghost[i]= TRUNC_ELEMENT(m->y_bottom_interior1[i]);
	}

	for (i= 0; i < m->x_dim * m->z_dim; i++)   {
	    m->z_top_ghost[i]= TRUNC_ELEMENT(m->z_top_interior1[i]);
	    m->z_bottom_ghost[i]= TRUNC_ELEMENT(m->z_bottom_interior1[i]);
	}
    }

}  /* end of compute() */



static void
do_delay(double compute_time, double delay, int imbalance)
{

double w;
unsigned int usecs;
double r;


    if (delay > 0.0)   {
	/*
	** Extend compute time a little bit, by adding a wait of
	** delay percent of compute time.
	*/
	if (!imbalance)   {
	    /* All ranks are equally delayed by delay % */
	    w= compute_time * delay / 100.0;
	    usecs= lrint(w * 1000000.0);
	    usleep(usecs);

	} else   {
	    /* Randomly delay ranks by an average of delay % */
	    r= -1.0 * log(drand48()) * delay;
	    if (r < 0.0)   {
		r= 0.0;
	    }
	    w= compute_time * r / 100.0;
	    usecs= lrint(w * 1000000.0);
	    usleep(usecs);
	}
    }

}  /* end of do_delay() */
#if STANDALONECPLUSPLUS
}
#endif
