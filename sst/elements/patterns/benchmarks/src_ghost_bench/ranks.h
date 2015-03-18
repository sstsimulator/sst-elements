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
** $Id: ranks.h,v 1.1 2010-10-22 22:14:54 rolf Exp $
**
** Rolf Riesen, Sandia National Laboratories, July 2010
** Functions to map MPI ranks to global (distributed) memory regions.
*/

#ifndef _RANKS_H_
#define _RANKS_H_


void map_ranks(int num_ranks, int TwoD, int *width, int *height, int *depth);
void check_element_assignment(int verbose, int decomposition_only, int num_ranks, int width, int height,
	int depth, int my_rank, int *TwoD, int *x_dim, int *y_dim, int *z_dim);

#endif /* _RANKS_H_ */
