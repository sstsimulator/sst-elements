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
#include <pthread.h>
#include <assert.h>


static void* threadMain( void *args )
{
    return NULL;
}

extern char **environ;
int main( int argc, char* argv[] )
{
    int numThreads = atoi( argv[1] );
    int i;
    pthread_t* threads = malloc( sizeof( *threads ) * numThreads );
    assert( threads );

    printf("numThreads %d\n",numThreads);

    for ( i = 0; i < numThreads; ++i ) {
        pthread_create( &threads[i], NULL, threadMain, NULL );
    } 

    for ( i = 0; i < numThreads; ++i ) {
        pthread_join( threads[i], NULL );
    }

    printf("test done\n");
    return 0;
}
