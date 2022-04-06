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

#ifndef _RDMA_H
#define _RDMA_H

#include <assert.h>
#include <stdint.h>
#include "base.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t Node;
typedef uint32_t Pid;

void rdma_init(void); 
void rdma_fini(void); 
Node rdma_getMyNode();
uint32_t rdma_getNumNodes();

// create a Completion Queue
// returns:  CompQueueId 
int rdma_create_cq( );

// create a Receive Queue, with Key and a Completion Queue, 
// the Key is what a sender uses to target this queue
// returns:  RecvQueueId 
int rdma_create_rq( RecvQueueKey, CompQueueId );

// post a send to Node, Pid, RecvQueueKey, completions will be posted in the Completion Queue,
// Context is a void* that will be returned in the Completion Event
int rdma_send_post( void* buf, size_t len, Node, Pid, RecvQueueKey, CompQueueId, Context ); 

// post a buffer into receive queue, it can receive from any source
int rdma_recv_post( void* buf, size_t len, RecvQueueId, Context ); 

// read completion event, can be blocking or non-blocking
int rdma_read_comp( CompQueueId, RdmaCompletion*, int blocking );

// register a region of memory with a given key
int rdma_memory_reg( MemRgnKey key, const void* addr, size_t length );

// unregister a region of memory with a given key
int rdma_memory_unreg( MemRgnKey key );

// write to a memory region Node/Pid with key, the write happens at offset into the memory region
int rdma_memory_write( MemRgnKey key, Node, Pid, size_t offset, void* data, size_t length, CompQueueId id, Context context );

// read from a memory region Node/Pid with key, the read happens at offset into the memory region
int rdma_memory_read( MemRgnKey key, Node, Pid, size_t offset, void* data, size_t length, CompQueueId id, Context context );

// barrier for all nodes
int rdma_barrier();

#ifdef __cplusplus
}
#endif

#endif
