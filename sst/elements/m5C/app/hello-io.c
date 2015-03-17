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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/syscall.h>

#define STR "hello Mike!"

int main( )
{
    int fd;
    //char buf[1024];
    char*  buf = malloc(1024);

    printf("hello mike\n");
    
    if ( ( fd = open("/tmp/hello", O_CREAT|O_RDWR, S_IRWXU ) ) == -1 ) {
	fprintf(stderr,"open 1 failed `%s`\n",strerror(errno));
	abort();
    }
 
    printf("fd=%d\n",fd);
    if ( write( fd, STR, strlen(STR) ) != strlen(STR) ) {
	fprintf(stderr,"write failed `%s`\n",strerror(errno));
	abort();
    }

    if ( close( fd ) == -1 ) {
	fprintf(stderr,"close 2 failed `%s`\n",strerror(errno));
	abort();
    }

    if ( ( fd = open("/tmp/hello", O_RDONLY ) ) == -1 ) {
	fprintf(stderr,"open 2 failed `%s`\n",strerror(errno));
	abort();
    }
 
    if ( read( fd, buf, strlen(STR) ) != strlen(STR) ) {
	fprintf(stderr,"read failed `%s`\n",strerror(errno));
	abort();
    }
 
    buf[strlen(STR)] = 0;

    printf("hello mike `%s`\n",buf);

    if ( close( fd ) == -1 ) {
	fprintf(stderr,"close 2 failed `%s`\n",strerror(errno));
	abort();
    }

    printf("goodbye mike\n");

    return 0;
}
