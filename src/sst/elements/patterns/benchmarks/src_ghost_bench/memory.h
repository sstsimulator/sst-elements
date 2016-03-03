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
** $Id: memory.h,v 1.1 2010-10-22 22:14:54 rolf Exp $
**
** Rolf Riesen, Sandia National Laboratories, July 2010
** Routines for memory management
*/

#ifndef _MEMORY_H_
#define _MEMORY_H_

#include <string.h>	/* For size_t */
#include <math.h>



/*
** The following three defines have to match in type
*/
#define ELEMENT_TYPE		double
#define MPI_ELEMENT_TYPE	MPI_DOUBLE
#define TRUNC_ELEMENT(x)	trunc(x)

/* Keep track of local memory */
typedef struct mem_ptr_t   {
    int x_dim;
    ELEMENT_TYPE *x_top_ghost;
    ELEMENT_TYPE *x_top_interior1;
    ELEMENT_TYPE *x_top_interior2;
    ELEMENT_TYPE *x_bottom_ghost;
    ELEMENT_TYPE *x_bottom_interior1;
    ELEMENT_TYPE *x_bottom_interior2;

    int y_dim;
    ELEMENT_TYPE *y_top_ghost;
    ELEMENT_TYPE *y_top_interior1;
    ELEMENT_TYPE *y_top_interior2;
    ELEMENT_TYPE *y_bottom_ghost;
    ELEMENT_TYPE *y_bottom_interior1;
    ELEMENT_TYPE *y_bottom_interior2;

    int z_dim;
    ELEMENT_TYPE *z_top_ghost;
    ELEMENT_TYPE *z_top_interior1;
    ELEMENT_TYPE *z_top_interior2;
    ELEMENT_TYPE *z_bottom_ghost;
    ELEMENT_TYPE *z_bottom_interior1;
    ELEMENT_TYPE *z_bottom_interior2;
} mem_ptr_t;


int mem_needed(int verbose, int decomposition_only, int num_ranks, int my_rank, int TwoD,
	int x_dim, int y_dim, int z_dim);
void do_mem_alloc(int my_rank, int TwoD, size_t mem_estimate, mem_ptr_t *memory,
	int x_dim, int y_dim, int z_dim);
void mem_free(mem_ptr_t *m);
int mem_alloc(int x, int y, int z, mem_ptr_t *m, size_t *mem_total);


#endif /* _MEMORY_H_ */
