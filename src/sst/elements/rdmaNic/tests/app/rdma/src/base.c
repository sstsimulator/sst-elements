// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sys/mman.h>
#include <stdio.h>
#include <inttypes.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/syscall.h>

#include "base.h"

#define DEBUG 0
#define dbgPrint(fmt, ARGS...) \
        do { if (DEBUG) fprintf(stdout, "%s():%d: " fmt, __func__,__LINE__, ##ARGS); } while (0)


static NicQueueInfo s_nicQueueInfo;
static HostQueueInfo s_hostQueueInfo;
static int s_reqQueueHeadIndex;
static volatile Addr_t s_reqQtailIndex;

static Addr_t* s_nicBaseAddr;

static Addr_t* getNicWindow();
static void writeSetup( HostQueueInfo* setup );
static void readNicQueueInfo( volatile NicQueueInfo* info );

int base_n_pes() {
    return s_nicQueueInfo.numNodes * s_nicQueueInfo.numThreads;
}

int base_my_pe() {
    return s_nicQueueInfo.nodeId * s_nicQueueInfo.numThreads + s_nicQueueInfo.myThread;
}

Addr_t getCompQueueInfoAddress() {
    return (Addr_t) s_nicQueueInfo.compQueuesAddress;
}

void base_init( ) {

	dbgPrint("\n");

    // We should do something like open("/dev/rdmaNic", ) but devices are not currently implemented in Vanadis
    // so we use a magic file descriptor
#ifdef SYS_mmap2
    s_nicBaseAddr = (Addr_t*) syscall(SYS_mmap2, 0, 0, PROT_WRITE, 0, -1000, 0);
#else
    s_nicBaseAddr = (Addr_t*) syscall(SYS_mmap, 0, 0, PROT_WRITE, 0, -1000, 0);
#endif

    dbgPrint("nicBaseAddr=%p\n",s_nicBaseAddr);
	s_reqQueueHeadIndex = 0;

	bzero( &s_nicQueueInfo, sizeof(s_nicQueueInfo) );

    s_hostQueueInfo.respAddress = (Addr_t) &s_nicQueueInfo;

    s_hostQueueInfo.reqQueueTailIndexAddress = (Addr_t) &s_reqQtailIndex;

	dbgPrint("respAddress %#" PRIxBITS ", req tail addr %#" PRIxBITS "\n", s_hostQueueInfo.respAddress, s_hostQueueInfo.reqQueueTailIndexAddress);

	// make sure this structure does not cross page boundary
    writeSetup( &s_hostQueueInfo );

    readNicQueueInfo( &s_nicQueueInfo );

    // the NIC adds 1 to this addres to make sure it's not 0 as part of the handshake
    s_nicQueueInfo.reqQueueAddress -= 1;
    // the NIC returns addresses that are relative to the start of it's address space
    // which is mapped into a virtual address space the process uses to talk to the NIC
    // add the start of the virtual address space
    s_nicQueueInfo.reqQueueAddress += (Addr_t) s_nicBaseAddr;
    s_nicQueueInfo.compQueuesAddress += (Addr_t) s_nicBaseAddr;

    --s_nicQueueInfo.nodeId;
    --s_nicQueueInfo.myThread;

    dbgPrint("req Q addr %#" PRIxBITS " comp Qs addr %#" PRIxBITS "\n",s_nicQueueInfo.reqQueueAddress,s_nicQueueInfo.compQueuesAddress);
    dbgPrint("req size %d\n",s_nicQueueInfo.reqQueueSize);
}

static inline int getReqQueueTailIndex() {
	return s_reqQtailIndex;
}

static inline Addr_t* getReqQueueHeadAddr() {
	return  (Addr_t*) ( (NicCmd*) s_nicQueueInfo.reqQueueAddress + s_reqQueueHeadIndex);
}

void writeCmd( NicCmd* cmd ) {

	dbgPrint("cmd=%p\n",cmd);
    // we need a memory fence here to get MVAPICH working, It's not clear why it has to be hear
	__sync_synchronize();
	while ( ( s_reqQueueHeadIndex + 1 ) % s_nicQueueInfo.reqQueueSize == getReqQueueTailIndex() );

	memcpy( getReqQueueHeadAddr(), cmd, sizeof(*cmd) );

	// the NIC is waiting for sizeof(NicCmd) bytes to be written

	++s_reqQueueHeadIndex;
	s_reqQueueHeadIndex %= s_nicQueueInfo.reqQueueSize;
	dbgPrint("done\n");
}

static void readNicQueueInfo( volatile NicQueueInfo* info )
{
	dbgPrint("wait for response from NIC, addr %p\n",info);
	volatile Addr_t* ptr = (Addr_t*) info;

	for ( int i = 0; i < sizeof( NicQueueInfo )/sizeof(Addr_t);  i++ ) {
		while ( 0 == ptr[i] );
	}
}

static void writeSetup( HostQueueInfo* setup ) {

	dbgPrint("enter\n");

	Addr_t* tmp = (Addr_t*) setup;
	Addr_t* ptr = getNicWindow( );

	// the NIC is waiting on number of bytes writen so we don't need a memory fence
	// why can't this be a memcpy
	// for some reason this doesn't work with a memcpy
	for ( int i = 0; i < sizeof( HostQueueInfo )/sizeof(Addr_t);  i++ ) {
		ptr[i] = tmp[i];
	}

	dbgPrint("return\n");
}

static Addr_t* getNicWindow( ) {
	return (Addr_t*) s_nicBaseAddr;
}
