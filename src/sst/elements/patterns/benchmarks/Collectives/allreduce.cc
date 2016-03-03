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

#include <stdio.h>
#include <stdlib.h>		// For malloc()
#include <string.h>		// For memcpy()
#include <mpi.h>
#include "allreduce.h"

// Instead of using the MPI provided allreduce, we use this algorithm.
// It is the same one used in the communication pattern and allows for
// more accurate comparisons, since the number of messages as well as
// sources and destinations are the same.


#define MY_TAG_UP		(55)
#define MY_TAG_DOWN		(77)



void
my_allreduce(double *in, double *result, int msg_len, Collective_topology *ctopo)
{

double *tmp;
std::list<int>::iterator it;


    if (ctopo->is_leaf() && ctopo->num_nodes() > 1)   {
	// Send data to parent and wait for result
	MPI_Send(in, msg_len, MPI_DOUBLE, ctopo->parent_rank(), MY_TAG_UP, MPI_COMM_WORLD);
	MPI_Recv(result, msg_len, MPI_DOUBLE, ctopo->parent_rank(), MY_TAG_DOWN,
		MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    } else   {
	// Wait for the children
	memcpy(result, in, msg_len * sizeof(double));

	// We could do this outside of the loop, but that would probably not be fair
	// to MPI_Allreduce()
	tmp= (double *)malloc(msg_len * sizeof(double));
	if (tmp == NULL)   {
	    fprintf(stderr, "Out of memory!\n");
	    exit(-1);
	}

	for (it= ctopo->children.begin(); it != ctopo->children.end(); it++)   {
	    MPI_Recv(tmp, msg_len, MPI_DOUBLE, *it, MY_TAG_UP, MPI_COMM_WORLD,
		    MPI_STATUS_IGNORE);
	    for (int i= 0; i < msg_len; i++)   {
		// Only doing sum for now
		result[i]= result[i] + tmp[i];
	    }
	}

	if (!ctopo->is_root())   {
	    // Send it to the parent and wait for answer
	    MPI_Send(result, msg_len, MPI_DOUBLE, ctopo->parent_rank(), MY_TAG_UP,
		    MPI_COMM_WORLD);
	    MPI_Recv(result, msg_len, MPI_DOUBLE, ctopo->parent_rank(),
		    MY_TAG_DOWN, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	}

	// Send it back out
	for (it= ctopo->children.begin(); it != ctopo->children.end(); it++)   {
	    MPI_Send(result, msg_len, MPI_DOUBLE, *it, MY_TAG_DOWN,
		    MPI_COMM_WORLD);
	}

	free(tmp);
    }

}  /* end of my_allreduce() */
