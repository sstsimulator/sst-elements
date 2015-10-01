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

extern char **environ;
int main( int argc, char* argv[] )
{
    printf("hello mike pid=%d ppid=%d\n",getpid(),getppid());
    int i;
    for ( i = 0; i< argc; i++ ) {
        printf("argv[%d]=`%s`\n",i,argv[i]);
    }
    i = 0;
    while ( environ[i] ) {
        printf("envirn[%d]=`%s`\n",i,environ[i]);
        ++i;
    }
    printf("goodby mike\n");
    return 0;
}
