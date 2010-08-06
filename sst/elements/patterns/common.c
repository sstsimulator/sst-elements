/*
** Copyright 2009-2010 Sandia Corporation. Under the terms
** of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
** Government retains certain rights in this software.
** 
** Copyright (c) 2009-2010, Sandia Corporation
** All rights reserved.
** 
** This file is part of the SST software package. For license
** information, see the LICENSE file in the top level directory of the
** distribution.
*/


/*
** This file contains common routines used by all pattern generators.
*/
#include <stdio.h>
#include "common.h"



/* Local functions */
static int is_pow2(int num);



/*
** Must be called before any other function in this file.
** Sets up some things based on stuff we learn when we read the
** SST xml file.
*/
int
pattern_init(int x, int y, int my_rank)
{
    /* Make sure x * y is a power of 2, and my_rank is within range */
    if (!is_pow2(x * y))   {
	fprintf(stderr, "x = %d * y = %d is not a power of 2!\n", x, y);
	return FALSE;
    }

    if ((my_rank < 0) || (my_rank >= x * y))   {
	fprintf(stderr, "My rank not 0 <= %d < %d * %d\n", my_rank, x, y);
	return FALSE;
    }

    return TRUE; /* success */

}  /* end of pattern_init() */



/*
** Send "len" bytes to rank "dest"
** This function computes a route, based on the assumtopn that we
** are on a power of 2 x * y 2-D torus, and sends and event along
** that route. No actual data is sent.
*/
void
pattern_send(int dest, int len)
{
}  /* end of pattern_send() */



/*
** -----------------------------------------------------------------------------
** Some local functions
** -----------------------------------------------------------------------------
*/

static int
is_pow2(int num)
{
    if (num < 1)   {
	return FALSE;
    }
    
    if ((num & (num - 1)) == 0)   {
	return TRUE;
    }

    return FALSE;

}  /* end of is_pow2() */
