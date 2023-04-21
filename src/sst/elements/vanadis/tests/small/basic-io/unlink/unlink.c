// Copyright 2009-2023 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2023, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>

#define PATH_NAME "unlink-test-file"

int main( int argc, char* argv[] ) 
{
    if ( unlink( PATH_NAME ) ) {
        printf("unlink '%s' failed, error %s \n",PATH_NAME,strerror(errno));
    } else {
        printf("unlink '%s' succeeded\n",PATH_NAME);
    }   

    int fd = open( PATH_NAME, O_WRONLY|O_CREAT, S_IRWXU );
    if ( fd == -1) {
        printf("open '%s' failed, error %s \n",PATH_NAME,strerror(errno));
        exit(-1);
    }
    printf("open( '%s' ) succeeded\n",PATH_NAME);
    if ( unlink( PATH_NAME ) ) {
        printf("unlink '%s' failed, error %s \n",PATH_NAME,strerror(errno));
    } else {
        printf("unlink '%s' succeeded\n",PATH_NAME);
    }
}

