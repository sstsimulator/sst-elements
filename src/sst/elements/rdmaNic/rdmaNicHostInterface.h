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

typedef uint32_t Addr_t;

typedef Addr_t Context;
typedef int QueueIndex;
typedef enum { RdmaDone=0, RdmaSend=1, RdmaRecv, RdmaFini, RdmaCreateCQ, RdmaCreateRQ, RdmaMemRgnReg, RdmaMemRgnUnreg, RdmaMemWrite, RdmaMemRead, RdmaBarrier } RdmaCmd;

typedef int MemRgnKey;
typedef int RecvQueueKey;
typedef int RecvQueueId;
typedef int CompQueueId;


typedef struct __attribute__((aligned(64))) {
	RdmaCmd type;
	Addr_t  respAddr;
    union Data {
        struct {
			MemRgnKey key;
            Addr_t addr;
            uint32_t len;
		} memRgnReg;
        struct {
			MemRgnKey key;
		} memRgnUnreg;
        struct {
			MemRgnKey key;
			uint32_t offset;
			int node;
            int pe;
            Addr_t srcAddr;
            uint32_t len;
			CompQueueId cqId;
			Context context;
		} write;
        struct {
			MemRgnKey key;
			uint32_t offset;
			int node;
            int pe;
            Addr_t destAddr;
            uint32_t len;
			CompQueueId cqId;
			Context context;
		} read;
        struct {
            Addr_t addr;
            uint32_t len;
			int node;
            int pe;
			RecvQueueKey rqKey;
			CompQueueId cqId;
			Context context;
        } send;
        struct {
            Addr_t addr;
            uint32_t len;
			RecvQueueId rqId;
			Context context;
        } recv;
        struct {
			RecvQueueKey rqKey;
			CompQueueId cqId;
		} createRQ;
        struct {
			Addr_t headPtr;
			Addr_t dataPtr;
			int	   num;
		} createCQ;
    } data;
} NicCmd;

#define NUM_COMP_Q 16
#define RDMA_COMP_Q_SIZE 32

typedef struct __attribute__((aligned(64))) {
	Context context;
} RdmaCompletion;

typedef struct __attribute__((aligned(64))) {
	uint32_t numThreads;
	uint32_t reqQueueAddress;
	uint32_t reqQueueHeadIndexAddress;
	uint32_t reqQueueSize;

	uint32_t nodeId;
	uint32_t numNodes;
	uint32_t myThread;
} NicQueueInfo;

typedef struct __attribute__((aligned(64))) {
	uint32_t respAddress;
	uint32_t reqQueueTailIndexAddress;

} HostQueueInfo;

typedef struct __attribute__((aligned(64))) {
	union {
		struct {
			Addr_t tailIndexAddr;
		} createCQ;
	} data;
	// this has to be last as it is the flag to indicate the 
	// NIC has completed the write, the NIC calculates  the offset
	// of this member and write it to the host after it write data
    volatile uint32_t retval;
} NicResp;
