// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2013, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <mpi.h>
#include <assert.h>
#include <stdio.h>


struct Buf {
    unsigned char data[10];
};
int main( int argc, char **argv )
{
    int rc;
    int dest;
    MPI_Request request;
    struct Buf sBuf,rBuf;
    
    printf("hello world\n");

    rc = MPI_Init( &argc, &argv ); 
    assert( rc == MPI_SUCCESS );

    printf("%s():%d\n",__func__,__LINE__);
    int rank,size;
    rc = MPI_Comm_rank( MPI_COMM_WORLD, &rank );
    assert( rc == MPI_SUCCESS );

    dest = (rank + 1 ) % 2;

    printf("%s():%d\n",__func__,__LINE__);
    rc = MPI_Comm_size( MPI_COMM_WORLD, &size );
    assert( rc == MPI_SUCCESS );

    printf("size=%d rank=%d\n",size,rank);
#if 1 
    MPI_Irecv( &sBuf, sizeof(sBuf), MPI_CHAR, dest, 0xdead, 
                        MPI_COMM_WORLD, &request );
    assert( rc == MPI_SUCCESS );
#endif

    printf("%s():%d\n",__func__,__LINE__);
    rc = MPI_Barrier(MPI_COMM_WORLD);
    assert( rc == MPI_SUCCESS );
    
#if 1 
    printf("%s():%d\n",__func__,__LINE__);
    MPI_Send( &rBuf, sizeof(rBuf), MPI_CHAR, dest, 0xdead, 
                        MPI_COMM_WORLD );
    assert( rc == MPI_SUCCESS );
#endif

    printf("%s():%d\n",__func__,__LINE__);
    rc = MPI_Finalize(); 
    assert( rc == MPI_SUCCESS );

    printf("goodbye world\n");

    return  0;
}
