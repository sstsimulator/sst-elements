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

#ifndef __m5_syscall_h
#define __m5_syscall_h

#ifdef __x86_64__
#define SYS_virt2phys 273 
#define SYS_mmap_dev  274 
#define SYS_barrier   500
#else
    #error "What's your cpu?"
#endif

extern long _m5_syscall( long number, long arg0, long arg1, long arg2 );

#endif
