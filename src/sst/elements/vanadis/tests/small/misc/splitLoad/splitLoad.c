// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
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

int main( int argc, char* argv[] ) {

    char buf[4096];

    printf("&buf=%p\n",buf);
    void* alignedPtr = buf + (64*2);
    alignedPtr = (void*) ((unsigned long) alignedPtr & ~(64-1));
    printf("alignedPtr=%p\n",alignedPtr);

    void* bufPtr;
    bufPtr  = alignedPtr - sizeof(int);
    printf("bufPtr=%p\n",bufPtr);

    for ( int i = 1; i < sizeof(int); i++ ) {
        int* intPtr = (int*) (bufPtr+i);
        *intPtr = 0xdeadbeef;
        printf("i=%d intPtr=%p\n", i, intPtr);
        printf("buf+%d %x\n",i,*intPtr);
    }

    bufPtr  = alignedPtr - sizeof(unsigned long);
    printf("bufPtr=%p\n",bufPtr);

    for ( int i = 1; i < sizeof(unsigned long); i++ ) {
        unsigned long* longPtr = (unsigned long*) (bufPtr+i);
        *longPtr = (unsigned long) 0xdeadbeef12345678L;
        printf("i=%d longPtr=%p\n", i, longPtr);
        printf("buf+%d %lx\n",i,*longPtr);
    }

}
