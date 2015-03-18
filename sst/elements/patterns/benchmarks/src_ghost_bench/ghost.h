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
** $Id: ghost.h,v 1.3 2010-10-27 23:00:47 rolf Exp $
**
** Rolf Riesen, Sandia National Laboratories, July 2010
*/

#ifndef _GHOST_H_
#define _GHOST_H_


#undef TRUE
#define TRUE            	(1)
#undef FALSE
#define FALSE           	(0)

#define DEFAULT_TIME_STEPS	(1000)
#define DEFAULT_2D_X_DIM	(16384)
#define DEFAULT_2D_Y_DIM	(16384)
#define DEFAULT_3D_X_DIM	(400)
#define DEFAULT_3D_Y_DIM	(400)
#define DEFAULT_3D_Z_DIM	(400)
#define DEFAULT_LOOP		(16)
#define DEFAULT_REDUCE_STEPS	(20)

extern int my_rank;
extern int num_ranks;


int ghost_init(int argc, char *argv[]);
double ghost_work(void);
void ghost_print(void);


#endif  /* _GHOST_H_ */
