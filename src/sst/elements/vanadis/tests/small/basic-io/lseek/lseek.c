// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#define PATH_NAME "lseek_test.txt"
#define BUF_LEN 33

char* data =
"The Structural Simulation Toolkit (SST) was developed to explore innovations in highly concurrent systems \
where the ISA, microarchitecture, and memory interact with the programming model and communications system. \
The package provides two novel capabilities. The first is a fully modular design that enables extensive \
exploration of an individual system parameter without the need for intrusive changes to the simulator. \
The second is a parallel simulation environment based on MPI. This provides a high level of performance \
and the ability to look at large systems. The framework has been successfully used to model concepts ranging \
from processing in memory to conventional processors connected by conventional network interfaces and running MPI.";


int main( int argc, char* argv[] ) {

    int fd = open( PATH_NAME, O_RDWR|O_CREAT, S_IRWXU );
    if ( fd == -1) {
        fprintf(stderr,"%s() open '%s' failed, error %s \n",__func__,PATH_NAME,strerror(errno));
        exit(-1);
    }
    fprintf(stderr,"%s() open( '%s' ) succeeded\n",__func__,PATH_NAME);

    off_t off = lseek( fd, 0, SEEK_CUR );

    assert( 0 == off ); 

    size_t len = strlen(data);

    size_t half = len/2;

    assert( half == write( fd, data, half ) );

    off = lseek( fd, 0, SEEK_CUR );
    printf("off=%zu\n",off);
    assert( off == half );

    assert( (len-half) == write( fd, data+half, len-half ) );

    off = lseek( fd, 0, SEEK_CUR );
    printf("off=%zu\n",off);
    assert( off == len );

    off = lseek( fd, 0, SEEK_SET );
    assert( off == 0 );
    assert( 0 == lseek( fd, 0, SEEK_CUR ) );

    
    char buf[BUF_LEN + 1];
    bzero( buf, BUF_LEN);

    assert( BUF_LEN == read( fd, buf, BUF_LEN ) );

    printf("%s\n",buf);

    assert( 0 == strncmp(data,buf,BUF_LEN) );

    assert( len == lseek( fd, 0, SEEK_END ) );

    assert( len-BUF_LEN == lseek( fd, -BUF_LEN, SEEK_CUR ) );

    bzero( buf, BUF_LEN);

    assert( BUF_LEN  == read( fd, buf, BUF_LEN ) );

    assert( 0 == strncmp(data+(len-BUF_LEN),buf,BUF_LEN) );

    printf("%s\n",buf);

    if ( close( fd ) ) {
        fprintf(stderr,"%s() close failed, %s",__func__,strerror(errno));
        exit(-1);
    }

    fprintf(stderr,"%s() close '%s' succeeded\n",__func__,PATH_NAME);

    if ( unlink( PATH_NAME ) ) {
        printf("unlink '%s' failed, error %s \n",PATH_NAME,strerror(errno));
    }
    fprintf(stderr,"unlink '%s' succeeded\n",PATH_NAME);
    return 0;
}
