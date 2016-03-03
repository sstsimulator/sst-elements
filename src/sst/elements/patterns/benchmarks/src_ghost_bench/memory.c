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
** $Id: memory.c,v 1.4 2010-10-27 22:11:55 rolf Exp $
**
** Rolf Riesen, Sandia National Laboratories, July 2010
** Routines for memory management
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>	/* For memset() */
#include <mpi.h>
#if STANDALONECPLUSPLUS
extern "C" {
#endif
#include "ghost.h"
#include "memory.h"


/* Local functions */
static void mem_init(int TwoD, mem_ptr_t *m, int seed);



/*
** Estimate how much memory we will need
*/
int
mem_needed(int verbose, int decomposition_only, int num_ranks, int my_rank, int TwoD,
	int x_dim, int y_dim, int z_dim)
{

size_t mem_estimate;


    if (TwoD)   {
	mem_estimate= (6 * x_dim + 6 * y_dim) * sizeof(ELEMENT_TYPE);
	if (((my_rank == 0) && (verbose)) || ((my_rank == 0) && (decomposition_only)))   {
	    printf("Will Allocate memory for %d * %d virtual elements (of size %ld bytes) per rank\n",
		x_dim, y_dim, (long int)sizeof(ELEMENT_TYPE));
	    printf("Emulated memory per rank: %lld bytes (%.3f GB), %.3f GB total\n",
		x_dim * y_dim * (long long int)sizeof(ELEMENT_TYPE),
		(float)(x_dim * y_dim * (long long int)sizeof(ELEMENT_TYPE)) / 1024 / 1024 / 1024,
		(float)(x_dim * y_dim * (long long int)sizeof(ELEMENT_TYPE)) / 1024 / 1024 / 1024 * num_ranks);
	    printf("Memory needed per rank: %ld bytes (%.3f MB), %.1f MB total\n",
		(long int)mem_estimate, (float)mem_estimate / 1024 / 1024,
		(float)mem_estimate / 1024 / 1024 * num_ranks);
	}
    } else   {
	mem_estimate= (6 * x_dim * y_dim + 6 * y_dim * z_dim + 6 * x_dim * z_dim) * sizeof(ELEMENT_TYPE);
	if (((my_rank == 0) && (verbose)) || ((my_rank == 0) && (decomposition_only)))   {
	    printf("Will Allocate memory for %d * %d * %d virtual elements (of size %ld bytes) per rank\n",
		x_dim, y_dim, z_dim, (long int)sizeof(ELEMENT_TYPE));
	    printf("Emulated memory per rank: %ld bytes (%.3f GB), %.3f GB total\n",
		x_dim * y_dim * z_dim * (long int)sizeof(ELEMENT_TYPE),
		(float)(x_dim * y_dim * z_dim * (long int)sizeof(ELEMENT_TYPE)) / 1024 / 1024 / 1024,
		(float)(x_dim * y_dim * z_dim * (long int)sizeof(ELEMENT_TYPE)) / 1024 / 1024 / 1024 * num_ranks);
	    printf("Memory needed per rank: %ld bytes (%.3f MB), %.1f MB total\n",
		(long int)mem_estimate, (float)mem_estimate / 1024 / 1024,
		(float)mem_estimate / 1024 / 1024 * num_ranks);
	}
    }

    return mem_estimate;

}  /* end of mem_needed() */



/*
** Allocate the memory, check againts the estimate, make sure it worked
** on all ranks, and initialize it.
*/
void
do_mem_alloc(int my_rank, int TwoD, size_t mem_estimate, mem_ptr_t *memory, int x_dim, int y_dim, int z_dim)
{

int res;
size_t mem_total;


    res= mem_alloc(x_dim, y_dim, z_dim, memory, &mem_total);
    if (mem_total != mem_estimate)   {
        if (my_rank == 0)   {
	    fprintf(stderr, "Memory allocation %ld != estimate %ld!\n", (long int)mem_total,
		(long int)mem_estimate);
	}

	mem_free(memory);
	MPI_Finalize();
        exit (-1);
    }

    /* Make sure it worked on all ranks */
    MPI_Allreduce(MPI_IN_PLACE, &res, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    if (res > 0)   {
        if (my_rank == 0)   {
	    fprintf(stderr, "Memory allocation on some (all) ranks failed!\n");
	}

	mem_free(memory);
	MPI_Finalize();
        exit (-1);
    }

    /* Initialize it */
    mem_init(TwoD, memory, my_rank);

}  /* end of do_mem_alloc() */



/*
** Free all memory we have allocated so far
*/
void
mem_free(mem_ptr_t *m)
{

    if (m->x_top_ghost) free(m->x_top_ghost);
    if (m->x_top_interior1) free(m->x_top_interior1);
    if (m->x_top_interior2) free(m->x_top_interior2);
    if (m->x_bottom_ghost) free(m->x_bottom_ghost);
    if (m->x_bottom_interior1) free(m->x_bottom_interior1);
    if (m->x_bottom_interior2) free(m->x_bottom_interior2);

    if (m->y_top_ghost) free(m->y_top_ghost);
    if (m->y_top_interior1) free(m->y_top_interior1);
    if (m->y_top_interior2) free(m->y_top_interior2);
    if (m->y_bottom_ghost) free(m->y_bottom_ghost);
    if (m->y_bottom_interior1) free(m->y_bottom_interior1);
    if (m->y_bottom_interior2) free(m->y_bottom_interior2);

    if (m->z_top_ghost) free(m->z_top_ghost);
    if (m->z_top_interior1) free(m->z_top_interior1);
    if (m->z_top_interior2) free(m->z_top_interior2);
    if (m->z_bottom_ghost) free(m->z_bottom_ghost);
    if (m->z_bottom_interior1) free(m->z_bottom_interior1);
    if (m->z_bottom_interior2) free(m->z_bottom_interior2);

}  /* end of mem_free() */



/*
** Local macros
*/
#define CHECK_PTR(ptr)   { \
    if (ptr == NULL)   { \
	return 1; \
    } \
}

#define ALLOC_ROW(ptr, n)   { \
    ptr= (ELEMENT_TYPE *)malloc(n * sizeof(ELEMENT_TYPE)); CHECK_PTR(ptr); \
}

#define ALLOC_SURFACE(ptr, x, y)   { \
    ptr= (ELEMENT_TYPE *)malloc(x * y * sizeof(ELEMENT_TYPE)); CHECK_PTR(ptr); \
}



int
mem_alloc(int x, int y, int z, mem_ptr_t *m, size_t *mem_total)
{

    /* Clear the memory pointer structure */
    if (mem_total != NULL)   {
	*mem_total= 0;
    }
    memset(m, 0, sizeof(mem_ptr_t));

    if (z <= 1)   {
	/* 2-D */
	/*
	** We allocate 4 ghost rows and 4 * 2 interior rows
	*/
	m->z_dim= 0;
	m->x_dim= x;
	if (mem_total != NULL)   {
	    ALLOC_ROW(m->x_top_ghost, x);
	    ALLOC_ROW(m->x_top_interior1, x);
	    ALLOC_ROW(m->x_top_interior2, x);
	    ALLOC_ROW(m->x_bottom_ghost, x);
	    ALLOC_ROW(m->x_bottom_interior1, x);
	    ALLOC_ROW(m->x_bottom_interior2, x);
	    *mem_total += 6 * x * sizeof(ELEMENT_TYPE);
	}

	m->y_dim= y;
	if (mem_total != NULL)   {
	    ALLOC_ROW(m->y_top_ghost, y);
	    ALLOC_ROW(m->y_top_interior1, y);
	    ALLOC_ROW(m->y_top_interior2, y);
	    ALLOC_ROW(m->y_bottom_ghost, y);
	    ALLOC_ROW(m->y_bottom_interior1, y);
	    ALLOC_ROW(m->y_bottom_interior2, y);
	    *mem_total += 6 * y * sizeof(ELEMENT_TYPE);
	}

    } else   {

	/* 3-D */
	/*
	** We allocate 6 ghost surfaces and 6 * 2 interior surfaces
	** This benchmarks places the elements of a ghost surface into contigous
	** memory, so a single send or receive can be used to transfer the data
	** contained on one surface.
	*/
	m->x_dim= x;
	if (mem_total != NULL)   {
	    ALLOC_SURFACE(m->x_top_ghost, x, y);
	    ALLOC_SURFACE(m->x_top_interior1, x, y);
	    ALLOC_SURFACE(m->x_top_interior2, x, y);
	    ALLOC_SURFACE(m->x_bottom_ghost, x, y);
	    ALLOC_SURFACE(m->x_bottom_interior1, x, y);
	    ALLOC_SURFACE(m->x_bottom_interior2, x, y);
	    *mem_total += 6 * x * y * sizeof(ELEMENT_TYPE);
	}

	m->y_dim= y;
	if (mem_total != NULL)   {
	    ALLOC_SURFACE(m->y_top_ghost, y, z);
	    ALLOC_SURFACE(m->y_top_interior1, y, z);
	    ALLOC_SURFACE(m->y_top_interior2, y, z);
	    ALLOC_SURFACE(m->y_bottom_ghost, y, z);
	    ALLOC_SURFACE(m->y_bottom_interior1, y, z);
	    ALLOC_SURFACE(m->y_bottom_interior2, y, z);
	    *mem_total += 6 * y * z * sizeof(ELEMENT_TYPE);
	}

	m->z_dim= z;
	if (mem_total != NULL)   {
	    ALLOC_SURFACE(m->z_top_ghost, x, z);
	    ALLOC_SURFACE(m->z_top_interior1, x, z);
	    ALLOC_SURFACE(m->z_top_interior2, x, z);
	    ALLOC_SURFACE(m->z_bottom_ghost, x, z);
	    ALLOC_SURFACE(m->z_bottom_interior1, x, z);
	    ALLOC_SURFACE(m->z_bottom_interior2, x, z);
	    *mem_total += 6 * x * z * sizeof(ELEMENT_TYPE);
	}
    }

    return 0;

}  /* end of mem_alloc() */



/*
** Local functions
*/
static void
mem_init(int TwoD, mem_ptr_t *m, int seed)
{

int i;
ELEMENT_TYPE value;


    /* Ghost cells get initialized to 0 */
    if (m->x_top_ghost)    memset(m->x_top_ghost,    0, sizeof(ELEMENT_TYPE) * m->x_dim);
    if (m->x_bottom_ghost) memset(m->x_bottom_ghost, 0, sizeof(ELEMENT_TYPE) * m->x_dim);
    if (m->y_top_ghost)    memset(m->y_top_ghost,    0, sizeof(ELEMENT_TYPE) * m->y_dim);
    if (m->y_bottom_ghost) memset(m->y_bottom_ghost, 0, sizeof(ELEMENT_TYPE) * m->y_dim);
    if (m->z_top_ghost)    memset(m->z_top_ghost,    0, sizeof(ELEMENT_TYPE) * m->z_dim);
    if (m->z_bottom_ghost) memset(m->z_bottom_ghost, 0, sizeof(ELEMENT_TYPE) * m->z_dim);

    if (TwoD)   {
	value= 0.000001 * seed;
	for (i= 0; i < m->x_dim; i++)   {
	    m->x_top_interior1[i] = value + 0.000000;
	    m->x_top_interior2[i]= value + 0.000003;
	    m->x_bottom_interior1[i]= value + 0.000005;
	    m->x_bottom_interior2[i]= value + 0.000007;
	    value= value + 0.000001;
	}

	value= 0.000001 * seed;
	for (i= 0; i < m->y_dim; i++)   {
	    m->y_top_interior1[i]= value + 0.000000;
	    m->y_top_interior2[i]= value + 0.000003;
	    m->y_bottom_interior1[i]= value + 0.000005;
	    m->y_bottom_interior2[i]= value + 0.000007;
	    value= value + 0.000001;
	}

    } else   {

	value= 0.000001 * seed;
	for (i= 0; i < m->x_dim * m->y_dim; i++)   {
	    m->x_top_interior1[i] = value + 0.000000;
	    m->x_top_interior2[i]= value + 0.000003;
	    m->x_bottom_interior1[i]= value + 0.000005;
	    m->x_bottom_interior2[i]= value + 0.000007;
	    value= value + 0.000001;
	}

	value= 0.000001 * seed;
	for (i= 0; i < m->y_dim * m->z_dim; i++)   {
	    m->y_top_interior1[i]= value + 0.000000;
	    m->y_top_interior2[i]= value + 0.000003;
	    m->y_bottom_interior1[i]= value + 0.000005;
	    m->y_bottom_interior2[i]= value + 0.000007;
	    value= value + 0.000001;
	}

	value= 0.000001 * seed;
	for (i= 0; i < m->z_dim * m->x_dim; i++)   {
	    m->z_top_interior1[i]= value + 0.000000;
	    m->z_top_interior2[i]= value + 0.000003;
	    m->z_bottom_interior1[i]= value + 0.000005;
	    m->z_bottom_interior2[i]= value + 0.000007;
	    value= value + 0.000001;
	}
    }

}  /* end of mem_init() */
#if STANDALONECPLUSPLUS
}
#endif
