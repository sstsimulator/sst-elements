// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <stddef.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#define PATH_NAME "fread_fwrite_test.txt"
#define NUM_INTS 4096

void writeData( const char *pathname );
void readData( const char* pathname );

static FILE* output;

int main( int argc, char* argv[] ) {
    output=stdout;
    writeData( PATH_NAME );
    readData( PATH_NAME );
    if ( unlink( PATH_NAME ) ) {
        printf("unlink '%s' failed, error %s \n",PATH_NAME,strerror(errno));
    }
    return 0;
}

void writeData( const char* pathname ) {

    FILE* fp = fopen( pathname, "w" );
    if ( NULL == fp ) {
        fprintf(output,"%s() open '%s' failed, error %s \n",__func__,pathname,strerror(errno));
        exit(-1);
    }

    int* buf = (int*) malloc( sizeof(int) * NUM_INTS );
    for ( int i = 0; i < NUM_INTS; i++ ) {
        buf[i] = i;
    }

    int ret = fwrite( buf, sizeof(int), NUM_INTS, fp );
    if ( ret != NUM_INTS ) {
        fprintf(output,"%s() fwrite failed, %s",__func__,strerror(errno));
        exit(-1);
    }

    if ( fclose( fp ) ) {
        fprintf(output,"%s() close failed, %s",__func__,strerror(errno));
        exit(-1);
    }
}

void readData( const char* pathname ) {

    FILE* fp = fopen( pathname, "r" );
    if ( NULL == fp ) {
        fprintf(output,"%s() open '%s' failed, error %s \n",__func__,pathname,strerror(errno));
        exit(-1);
    }

    int* buf = (int*) malloc( sizeof(int) * NUM_INTS );

    int ret = fread( buf, sizeof(int), NUM_INTS, fp );

    if ( ret != NUM_INTS ) {
        fprintf(output,"%s() fread failed, %s",__func__,strerror(errno));
        exit(-1);
    }

    for ( int i = 0; i < NUM_INTS; i++ ) {
        if ( buf[i] != i ) {
            printf("buf[%d] = %d\n", i,  buf[i]);
        }
    }
    printf("read data matched written data\n");

    if ( fclose( fp ) ) {
        fprintf(output,"%s() close failed, %s",__func__,strerror(errno));
        exit(-1);
    }
}
