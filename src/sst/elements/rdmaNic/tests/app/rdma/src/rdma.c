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
#include <inttypes.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "rdma.h"

#define DEBUG 0
#define dbgPrint(fmt, ARGS...) \
        do { if (DEBUG) fprintf(stdout, "%s():%d: " fmt, __func__,__LINE__, ##ARGS); } while (0) 

typedef uint32_t* IndexPtr;

typedef struct {
	RdmaCompletion data[RDMA_COMP_Q_SIZE];
	volatile int headIndex;
	volatile int tailIndex;
	IndexPtr tailIndexAddr;
	int num;
} CompletionQ;

static CompletionQ s_compQ[NUM_COMP_Q];
static int s_curCompIndex = 0;

static int waitResp( NicResp* );
static NicResp* getResp(NicCmd* cmd );

#define USE_STATIC_CMDS 16 
#if USE_STATIC_CMDS

NicCmd _cmds[USE_STATIC_CMDS];
NicResp _resp[USE_STATIC_CMDS];
#endif

void rdma_init( void ) {

	setvbuf(stdout, NULL, _IONBF, 0);

    dbgPrint("\n");

  	base_init();

	for ( int i = 0; i < NUM_COMP_Q; i++) {
		s_compQ[i].num = RDMA_COMP_Q_SIZE;
		s_compQ[i].headIndex = 0;
		s_compQ[i].tailIndex = 0;
		s_compQ[i].tailIndexAddr = NULL;
	}
#if USE_STATIC_CMDS
	for ( int i = 0; i< USE_STATIC_CMDS; i++ ) {
		_cmds[i].respAddr = (Addr_t) NULL;
	}
#endif

    dbgPrint("return\n");
}

NicCmd* allocCmd() {
	NicCmd* cmd = NULL;
	NicResp* resp;
#if USE_STATIC_CMDS
	for ( int i = 0; i< USE_STATIC_CMDS; i++ ) {
		if ( 0 == _cmds[i].respAddr ) {
			resp = _resp + i;
			cmd = _cmds + i;
			break;
		}
	}
	assert( cmd );
#else
	NicCmd* cmd = malloc(sizeof(NicCmd)); 
	NicResp* resp = malloc(sizeof(NicResp));
#endif

	dbgPrint("cmdbuf=%p resp=%p\n",cmd,resp);
	// set retval to all ones as it is the flag will will watch for a change to none -1
	resp->retval = -INT_MAX;
	cmd->respAddr = (Addr_t) resp;
	
	return cmd;
}

void freeCmd( NicCmd* cmd ) {

#if USE_STATIC_CMDS
	cmd->respAddr = 0;
#else
	free( (NicResp*) cmd->respAddr );
	free( cmd );
#endif
}

void rdma_fini( void ) {

	NicCmd* cmd = allocCmd();

	cmd->type = RdmaFini;

	writeCmd( cmd );
	
	waitResp( getResp(cmd) );

	dbgPrint("retval=%d\n",((NicResp*)cmd->respAddr)->retval);


	freeCmd( cmd );
}

int rdma_barrier( void ) {

	NicCmd* cmd = allocCmd();

	cmd->type = RdmaBarrier;

	writeCmd( cmd );
	
	waitResp( getResp(cmd) );

	int retval = getResp(cmd)->retval;
	dbgPrint("retval=%d\n",retval);

	freeCmd( cmd );
	return retval;
}

Node rdma_getMyNode() { 
	return base_my_pe();
}
uint32_t rdma_getNumNodes(){
	return base_n_pes();
}

int rdma_create_cq( ) {
	NicCmd* cmd = allocCmd();
	
	dbgPrint("cmdbuf=%p headIndexAddr=%p dataAddr=%p\n", cmd, &s_compQ->headIndex, s_compQ->data);

	CompletionQ* compQ = s_compQ + s_curCompIndex++; 
	cmd->type = RdmaCreateCQ;

	cmd->data.createCQ.dataPtr = (Addr_t) compQ->data; 
	cmd->data.createCQ.headPtr = (Addr_t) &compQ->headIndex; 
	cmd->data.createCQ.num = compQ->num; 

	writeCmd( cmd );

	NicResp* resp = getResp(cmd);
	waitResp( resp );

	// the NIC returns addresses that are relative to the start of it's address space
	// which is mapped into a virtual address space the process uses to talk to the NIC
	// add the start of the virtual address space
	resp->data.createCQ.tailIndexAddr += getNicBase();

	int retval = resp->retval; 
	dbgPrint("retval=%d tailIndexPtr=%#x\n",retval,resp->data.createCQ.tailIndexAddr);
	assert( retval < NUM_COMP_Q ); 
	assert( s_compQ[retval].tailIndexAddr == (IndexPtr) NULL );
	s_compQ[retval].tailIndexAddr = (IndexPtr) resp->data.createCQ.tailIndexAddr; 

	freeCmd(cmd);

	return retval;
}

int rdma_destroy_cq( CompQueueId id ) {
    NicCmd* cmd = allocCmd();
	
    dbgPrint("cmdbuf=%p\n", cmd);

    cmd->type = RdmaDestroyCQ;
    cmd->data.destroyCQ.cqId = id; 

    writeCmd( cmd );

    NicResp* resp = getResp(cmd);

    waitResp( resp );
    int retval = resp->retval;
    dbgPrint("retval=%d\n",retval);

    freeCmd(cmd);
    return retval;
}

int rdma_create_rq( RecvQueueKey rqKey, CompQueueId cqId ) {
	NicCmd* cmd = allocCmd();
	
	dbgPrint("cmdbuf=%p\n", cmd);

	cmd->type = RdmaCreateRQ;
	cmd->data.createRQ.cqId = cqId; 
	cmd->data.createRQ.rqKey = rqKey; 

	writeCmd( cmd );

	NicResp* resp = getResp(cmd);

	waitResp( resp );
	int retval = resp->retval;
	dbgPrint("retval=%d\n",retval);

	freeCmd(cmd);
	return retval;
}

int rdma_destroy_rq( RecvQueueId id ) {
    NicCmd* cmd = allocCmd();
	
    dbgPrint("cmdbuf=%p\n", cmd);

    cmd->type = RdmaDestroyRQ;
    cmd->data.destroyRQ.rqId = id; 

    writeCmd( cmd );

    NicResp* resp = getResp(cmd);

    waitResp( resp );
    int retval = resp->retval;
    dbgPrint("retval=%d\n",retval);

    freeCmd(cmd);
    return retval;
}


int rdma_send_post( void* buf, size_t len, Node destNode, Pid destPid, RecvQueueKey rqKey, CompQueueId cqId, Context context )
{

	NicCmd* cmd = allocCmd();
	
	dbgPrint("ptr=%p len=%zu destNode=%d destPid=%d cmdbuf=%p context=%#x\n", buf,len,destNode,destPid,cmd,context);

	cmd->type = RdmaSend;
	cmd->data.send.pe = destPid;
	cmd->data.send.node = destNode;
	cmd->data.send.addr = (Addr_t)buf;
	cmd->data.send.len = len;
	cmd->data.send.rqKey = rqKey;
	cmd->data.send.cqId = cqId;
	cmd->data.send.context = context;
	
	writeCmd( cmd );

	NicResp* resp = getResp(cmd);

	waitResp( resp );
	int retval = resp->retval;
	dbgPrint("retval=%d\n",retval);

	freeCmd(cmd);
	return retval;
}

int rdma_recv_post( void* buf, size_t len, RecvQueueId rqId, Context context )
{
	NicCmd* cmd = allocCmd();
	
	dbgPrint("buf=%p len=%zu rqId=%d context=%#x\n", buf,len,rqId,context);

	cmd->type = RdmaRecv;
	cmd->data.recv.addr = (Addr_t)buf;
	cmd->data.recv.len = len;
	cmd->data.recv.rqId = rqId;
	cmd->data.recv.context = context;

	writeCmd( cmd );

	NicResp* resp = getResp(cmd);

	waitResp( resp );
	int retval = resp->retval;
	dbgPrint("retval=%d\n",retval);

	freeCmd(cmd);

	return 0;
}

int rdma_read_comp( CompQueueId cqId, RdmaCompletion* buf, int blocking ) {

	dbgPrint("cqId=%d head=%d tail=%d headAddr=%p tailAddr=%p\n",cqId, s_compQ[cqId].headIndex,s_compQ[cqId].tailIndex, &s_compQ[cqId].headIndex,&s_compQ[cqId].tailIndex);
	//dbgPrint("cqId=%d head=%d tail=%d respAddr=%p\n",cqId, s_compQ[cqId].headIndex,s_compQ[cqId].tailIndex, &s_compQ[cqId].data[s_compQ[cqId].tailIndex]);
	while ( s_compQ[cqId].headIndex == s_compQ[cqId].tailIndex );

	dbgPrint("got completion cqId=%d head=%d tail=%d \n",cqId, s_compQ[cqId].headIndex,s_compQ[cqId].tailIndex );
	//RdmaCompletion* comp = &s_compQ[cqId].data[s_compQ[cqId].tailIndex];
	memcpy( buf , &s_compQ[cqId].data[s_compQ[cqId].tailIndex], sizeof(*buf) );
	dbgPrint("context=%x\n",buf->context);

	++s_compQ[cqId].tailIndex;
	s_compQ[cqId].tailIndex %= s_compQ[cqId].num;
 	*s_compQ[cqId].tailIndexAddr = s_compQ[cqId].tailIndex;

	return 0;
}


int rdma_memory_reg( MemRgnKey key, const void* addr, size_t length ) 
{
	NicCmd* cmd = allocCmd();
	
	dbgPrint("key=%#x addr=%p length=%zu \n", key, addr, length );

	cmd->type = RdmaMemRgnReg;
	cmd->data.memRgnReg.addr = (Addr_t)addr;
	cmd->data.memRgnReg.len = length;
	cmd->data.memRgnReg.key = key;

	writeCmd( cmd );

	NicResp* resp = getResp(cmd);

	waitResp( resp );
	int retval = resp->retval;
	dbgPrint("retval=%d\n",retval);

	freeCmd(cmd);
	return 0;
}

int rdma_memory_unreg( MemRgnKey key )
{
	NicCmd* cmd = allocCmd();
	
	dbgPrint("key=%#x\n", key );

	cmd->type = RdmaMemRgnUnreg;
	cmd->data.memRgnUnreg.key = key;

	writeCmd( cmd );

	NicResp* resp = getResp(cmd);

	waitResp( resp );
	int retval = resp->retval;
	dbgPrint("retval=%d\n",retval);

	freeCmd(cmd);
	return 0;
}

int rdma_memory_write( MemRgnKey key, Node node, Pid pid, size_t offset, void* srcBuffer, size_t length, CompQueueId id, Context context )
{
	NicCmd* cmd = allocCmd();
	
	dbgPrint("key=%#x offset=%d length=%zu cqId=%d\n", key, offset, length, id );

	cmd->type = RdmaMemWrite;
	cmd->data.write.key = key;
	cmd->data.write.offset = offset;
	cmd->data.write.srcAddr = (Addr_t) srcBuffer;
	cmd->data.write.len = length;
	cmd->data.write.cqId = id;
	cmd->data.write.context = context;
	cmd->data.write.pe = pid;
	cmd->data.write.node = node;

	writeCmd( cmd );

	NicResp* resp = getResp(cmd);

	waitResp( resp );
	int retval = resp->retval;
	dbgPrint(" retval=%d\n",retval);

	freeCmd(cmd);
	return 0;
}

int rdma_memory_read( MemRgnKey key, Node node, Pid pid, size_t offset, void* destBuffer, size_t length, CompQueueId id, Context context )
{
	NicCmd* cmd = allocCmd();
	
	dbgPrint("key=%#x destBuffer=%p offset=%d length=%zu cqId=%d\n",key, destBuffer, offset, length, id );

	cmd->type = RdmaMemRead;
	cmd->data.read.key = key;
	cmd->data.read.offset = offset;
	cmd->data.read.destAddr = (Addr_t) destBuffer;
	cmd->data.read.len = length;
	cmd->data.read.cqId = id;
	cmd->data.read.context = context;
	cmd->data.read.pe = pid;
	cmd->data.read.node = node;

	writeCmd( cmd );

	NicResp* resp = getResp(cmd);

	waitResp( resp );
	int retval = resp->retval;
	dbgPrint("retval=%d\n",retval);

	freeCmd(cmd);
	return 0;
}

static int waitResp( NicResp* resp ) {
	dbgPrint("wait for response from NIC, addr %p\n",&resp->retval);

	while ( -INT_MAX == resp->retval );
	return 0;
}

static NicResp* getResp(NicCmd* cmd ) {
	return (NicResp*) cmd->respAddr;
}

