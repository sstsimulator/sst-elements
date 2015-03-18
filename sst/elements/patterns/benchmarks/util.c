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
** Some commonly used functions by the benchmarks
*/

#include <stdio.h>
#if STANDALONECPLUSPLUS
extern "C" {
#endif
#include "util.h"


void
disp_cmd_line(int argc, char **argv)
{

int i;


    printf("# Command line \"");
    for (i= 0; i < argc; i++)   {
	printf("%s", argv[i]);
	if (i < (argc - 1))   {
	    printf(" ");
	}
    }
    printf("\"\n");

}  /* end of disp_cmd_line() */
#if STANDALONECPLUSPLUS
}
#endif
