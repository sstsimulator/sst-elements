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

#include <unistd.h>
#include <m5_syscall.h>
#include <syscall.h> // for SYS_exit

void exit( int status )
{
    _m5_syscall( SYS_exit, status, 0, 0 );
    while(1);
}

