#include <stdio.h>
#include <inttypes.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#if 0
#include <unistd.h>
#include <sys/syscall.h>
#define syscall( a, x ) (uint32_t) x
#endif

#include "base.h"

#define DEBUG 1
#define dbgPrint(fmt, ARGS...) \
        do { if (DEBUG) fprintf(stdout, "%s():%d: " fmt, __func__,__LINE__, ##ARGS); } while (0) 


static NicQueueInfo s_nicQueueInfo;
static HostQueueInfo s_hostQueueInfo;
static int s_reqQueueHeadIndex;
static volatile uint32_t s_reqQtailIndex;

static uint32_t s_nicBaseAddr = 0x80000000;

static uint32_t* getNicWindow();
static void writeSetup( HostQueueInfo* setup );
static void readNicQueueInfo( volatile NicQueueInfo* info );

int base_n_pes() {
    return s_nicQueueInfo.numNodes * s_nicQueueInfo.numThreads;
}

int base_my_pe() {
    return s_nicQueueInfo.nodeId * s_nicQueueInfo.numThreads + s_nicQueueInfo.myThread;
}

void base_init( ) {

	dbgPrint("\n");
	s_reqQueueHeadIndex = 0;

	bzero( &s_nicQueueInfo, sizeof(s_nicQueueInfo) );

    s_hostQueueInfo.respAddress = (uint32_t) &s_nicQueueInfo;

    s_hostQueueInfo.reqQueueTailIndexAddress = (uint32_t) &s_reqQtailIndex;

	dbgPrint("req tail addr %#" PRIx32 "\n", s_hostQueueInfo.reqQueueTailIndexAddress);

	// make sure this structure does not cross page boundary
    writeSetup( &s_hostQueueInfo );

    readNicQueueInfo( &s_nicQueueInfo );

    --s_nicQueueInfo.nodeId;
    --s_nicQueueInfo.myThread;

    dbgPrint("req Q addr %#" PRIx32 "\n",s_nicQueueInfo.reqQueueAddress);
    dbgPrint("req head index %#" PRIx32 "\n",s_nicQueueInfo.reqQueueHeadIndexAddress);
    dbgPrint("req size %d\n",s_nicQueueInfo.reqQueueSize);
}

static inline int getReqQueueTailIndex() {
	return s_reqQtailIndex;
}

static inline uint32_t* getReqQueueHeadAddr() {
	return  (uint32_t*)((NicCmd*) s_nicQueueInfo.reqQueueAddress + s_reqQueueHeadIndex); 
}

void writeCmd( NicCmd* cmd ) {

	dbgPrint("cmd=%p\n",cmd);
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
	uint32_t* ptr = (uint32_t*) info;

	for ( int i = 0; i < sizeof( NicQueueInfo )/4;  i++ ) {
		while ( 0 == ptr[i] );
	}
}

static void writeSetup( HostQueueInfo* setup ) {

	dbgPrint("\n");

	uint32_t* tmp = (uint32_t*) setup;
	uint32_t* ptr = getNicWindow( );

	// the NIC is waiting on number of bytes writen so we don't need a memory fence
	// why can't this be a memcpy
	// for some reason this doesn't work with a memcpy 
	for ( int i = 0; i < sizeof( HostQueueInfo )/4;  i++ ) {
		ptr[i] = tmp[i];
	}
}

static uint32_t* getNicWindow( ) {
	return (uint32_t*) s_nicBaseAddr;
}
