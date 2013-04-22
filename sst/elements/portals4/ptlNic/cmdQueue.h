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

#ifndef _cmdQueue_h
#define _cmdQueue_h

#include "cmdQueueEntry.h"

#ifndef __NOT_SST__
namespace SST {
namespace Portals4 {
#endif

#define CMD_QUEUE_SIZE 16
typedef struct {
    volatile int head; 
    volatile int tail;
    cmdQueueEntry_t  queue[CMD_QUEUE_SIZE];
} cmdQueue_t ;    

#define ME_UNLINKED_SIZE 64 

#ifndef __NOT_SST__
}
}
#endif

#endif
