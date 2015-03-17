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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include "runtime.h"


static int _readFd;
static int _writeFd;

static __attribute__ ((constructor)) void  init(void)
{
    char buf[100];
    
    sprintf( buf, "/tmp/sst-barrier.%d", getppid() );
    
    _writeFd = open( buf, O_WRONLY ); 
    assert( _writeFd > -1 );

    sprintf( buf, "/tmp/sst-barrier-app-%d.%d", getpid() - 100, getppid() );
    
    _readFd = open( buf, O_RDONLY | O_NONBLOCK ); 
    assert( _readFd > -1 );
}


int cnos_get_rank( void )
{
    char* tmp = getenv("RT_RANK");
    int rank = -1;
    if ( tmp ) {
         rank = atoi(tmp);
    }

    return rank;
}

int cnos_get_size( void )
{
    char* tmp = getenv("RT_SIZE");
    int size = 0;
    if ( tmp ) {
         size = atoi(tmp);
    }

    return size;
}

int cnos_get_nidpid_map( cnos_nidpid_map_t** map )
{
    int size = cnos_get_size();
    if ( size > 0) {
        *map = (cnos_nidpid_map_t*) malloc( sizeof( cnos_nidpid_map_t ) * size );
        int i;
        for ( i = 0; i < size; i++ ) {
            (*map)[i].nid = i;  
            (*map)[i].pid = getpid();
        }
    }
    return size;
}

int cnos_barrier( void )
{
    int buf = getpid();
    int rc;
    rc = write( _writeFd, &buf, sizeof(buf) );
    assert( rc == sizeof(buf) ); 

    while ( ( rc = read( _readFd, &buf, sizeof(buf) ) ) == 0 );
    assert( rc == sizeof(buf) );
    return 0; 
}
