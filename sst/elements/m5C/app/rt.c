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
#include <stdlib.h>
#include <runtime.h>
#include <assert.h>

extern char **environ;
int main( int argc, char* argv[] )
{
    printf("hello mike\n");

    int rank = cnos_get_rank();
    int size = cnos_get_size();

    printf("rank=%d size=%d\n",rank,size);

    cnos_nidpid_map_t* map;

    assert( cnos_get_nidpid_map( &map ) == size );

    int i;
    for ( i = 0; i < size; i++) {
	printf("%d: rank=%d nid=%d pid=%d\n", rank, i, map[i].nid, map[i].pid );
    }

    printf("call cnos_barrier()\n");
    cnos_barrier();
    printf("cnos_barrier() return\n");
    printf("goodby mike\n");

    return 0;
}
