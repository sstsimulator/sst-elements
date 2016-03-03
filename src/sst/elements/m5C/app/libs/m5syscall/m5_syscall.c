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

#include <sys/syscall.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>

#include "m5_syscall.h"

static uint64_t volatile * ptr = NULL;

static __attribute__ ((constructor)) void  init(void)
{
    char* tmp = getenv("SYSCALL_ADDR");
    unsigned long addr = 0;
    if ( tmp )
        addr = strtoul(tmp,NULL,16);

    ptr = (uint64_t*)syscall( SYS_mmap_dev, addr, 0x2000 );
}

long _m5_syscall( long number, long arg0, long arg1, long arg2 )
{
    assert( ptr );

    ptr[0] = arg0;
    ptr[1] = arg1;
    ptr[2] = arg2;

    //asm volatile("wmb");
    ptr[0xf] = number + 1;
    //asm volatile("wmb");

    while ( (signed) ptr[0xf] == number + 1 );

    long retval = ptr[0];

    if ( retval < 0 ) {
	    errno = -retval;
        retval = -1;
    }
    return retval;
}
