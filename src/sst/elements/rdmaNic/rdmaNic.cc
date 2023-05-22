// Copyright 2009-2023 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2023, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include <sst/core/params.h>

#include "rdmaNic.h"


using namespace SST::Interfaces;
using namespace SST::MemHierarchy;

RdmaNic::RdmaNic(ComponentId_t id, Params &params) : Component(id),
	m_activeNicCmd(NULL),
	m_nextCqId(0),
	m_streamId(1),
	m_netPktMtuLen( 1024 ),
	m_dmaLink( nullptr )
{
    m_nicId = params.find<int>("nicId", -1);
    assert( m_nicId != -1 );

    m_numNodes = params.find<int>("numNodes", 0);
    assert( m_numNodes > 1 );

	out.init("RdmaNic", params.find<int>("verbose", 1), 0, Output::STDOUT);

    // Output for debug
    char buffer[100];
    snprintf(buffer,100,"@t:node%d:RdmaNic::@p():@l ",m_nicId);
    dbg.init( buffer, 
        params.find<int>("debug_level", 0),
        params.find<int>("debug_mask",0), 
		Output::STDOUT );

    auto pesPerNode = params.find<int>("pesPerNode", 0);
	assert( pesPerNode > 0 );

    // multi pe has not been tested
	assert( pesPerNode == 1 );

	m_mmioWriteFunc = std::bind( &RdmaNic::mmioWriteSetup, this, std::placeholders::_1 );

    m_ioBaseAddr = params.find<uint64_t>( "baseAddr", 0x100000000 );
    assert( m_ioBaseAddr != -1 );

    auto cmdQSize = params.find<int>("cmdQSize", 64);
    assert( cmdQSize );

    // make sure a NicCmd is multiple of a ache line
    // Make this more general 
    assert( sizeof( NicCmd) == 64 );

    m_backing = new Backing< NicCmd, QueueIndex >( pesPerNode, cmdQSize, NUM_COMP_Q );

    auto mmioLength = 1024*1024 * pesPerNode;
    assert( m_backing->getMemorySize() <= mmioLength );

    dbg.debug(CALL_INFO_LONG,1,DBG_X_FLAG,"\n");
    if ( m_nicId == 0 ) {
        dbg.debug(CALL_INFO_LONG,1,DBG_X_FLAG,"nicId=%d pesPerNode=%d numNodes=%d\n", m_nicId, pesPerNode, m_numNodes );
		dbg.debug(CALL_INFO_LONG,1,DBG_X_FLAG,"number cmd Q entries %d, cmd Q memory footprint %zu\n", 
                m_backing->getCmdQSize(), m_backing->getCmdQueueMemSize() );
    }

    m_hostInfo.resize( pesPerNode );
    m_nicCmdQueueV.resize( pesPerNode );

    // Clock handler
    std::string clockFreq = params.find<std::string>("clock", "1GHz");
    m_clockHandler = new Clock::Handler<RdmaNic>(this, &RdmaNic::clock);
    m_clockTC = registerClock( clockFreq, m_clockHandler );

    m_mmioLink = loadUserSubComponent<StandardMem>("mmio", ComponentInfo::SHARE_NONE, m_clockTC, new StandardMem::Handler<RdmaNic>(this, &RdmaNic::handleEvent));

    if (!m_mmioLink) {
        out.fatal(CALL_INFO_LONG, -1, "Unable to load StandardMem subcomponent; check that 'mmio' slot is filled in input.\n");
    }

	m_mmioLink->setMemoryMappedAddressRegion(m_ioBaseAddr, mmioLength );

	auto useDmaCache = params.find<bool>("useDmaCache",false);
	if ( useDmaCache ) {
    	m_dmaLink = loadUserSubComponent<StandardMem>("dma", ComponentInfo::SHARE_NONE, m_clockTC, new StandardMem::Handler<RdmaNic>(this, &RdmaNic::handleEvent));

    	if (!m_dmaLink) {
            out.fatal(CALL_INFO_LONG, -1, " Unable to load StandardMem subcomponent; check that 'dma' slot is filled in input.\n");
    	}
	} else {
		m_dmaLink = m_mmioLink;
	}

    // Set up the linkcontrol for inter node network
    m_linkControl = loadUserSubComponent<Interfaces::SimpleNetwork>( "rtrLink", ComponentInfo::SHARE_NONE, 2 );
    assert( m_linkControl );

    m_tailWriteQnum = 0;
    m_respQueueMemChannel = 1;
    m_dmaMemChannel = 2;
    int numSrc = 3;

    int maxPending = params.find<int>("maxMemReqs",128);
    //m_memReqQ = new MemRequestQ( this, maxPending, maxPending/numSrc, numSrc );
    m_memReqQ = loadComponentExtension<MemRequestQ>( this, maxPending, maxPending/numSrc, numSrc );

	m_handlers = new Handlers(this, &out);

	m_numVC = 2;
	m_networkQ = new NetworkQueue( *this, m_numVC, 32 ); 
	m_recvEngine = new RecvEngine( *this, m_numVC, 32 );
	m_sendEngine = new SendEngine( *this, m_numVC ); 
	int degree = params.find<int>("barrierDegree", 4 );
	int vc = params.find<int>("barrierVC", 0 );
	m_barrier = new Barrier( *this, vc, degree, m_nicId, m_numNodes );
}

void RdmaNic::writeResp(StandardMem::WriteResp* req) {
    dbg.debug(CALL_INFO_LONG,2,DBG_X_FLAG,"Write addr=%#" PRIx64 " size=%" PRIu64 "\n",req->pAddr,req->size);
	if ( ! req->getSuccess() ) {
        out.fatal(CALL_INFO_LONG, -1, " Error: write to address %#" PRIx64 " failed\n",req->pAddr );
	}
	m_memReqQ->handleResponse( req );
}

void RdmaNic::readResp(StandardMem::ReadResp* req) {
    dbg.debug(CALL_INFO_LONG,1,DBG_X_FLAG,"Read addr=%#" PRIx64 "\n",req->pAddr);
	m_memReqQ->handleResponse( req );
	if ( ! req->getSuccess() ) {
        out.fatal(CALL_INFO_LONG, -1, " Error: read of address %#" PRIx64 " failed\n",req->pAddr );
	}
}

void RdmaNic::mmioWriteSetup( StandardMem::Write* req) {

    int thread = calcThread( req->pAddr );

	// Does this need to be a permanent structure?
    auto& hostInfo = m_hostInfo[thread].hostInfo;
    auto& offset = m_hostInfo[thread].offset;

    dbg.debug( CALL_INFO_LONG,2,DBG_X_FLAG,"Write size=%zu addr=%" PRIx64 " offset=%llu thread=%d data %s\n",
                    req->data.size(), req->pAddr, req->pAddr - m_ioBaseAddr, thread, getDataStr(req->data).c_str() );

	memcpy( (uint8_t*) (&hostInfo) + offset, req->data.data(), req->data.size() );

	offset += req->data.size();

	if ( offset == sizeof(hostInfo) ) {

        NicQueueInfo nicInfo;

		uint64_t threadMemoryBase = 0;

		dbg.debug( CALL_INFO_LONG,1,DBG_X_FLAG,"resp Address=%#" PRIx32 "\n", hostInfo.respAddress );

		// set up the info about the req queue in the NIC
		m_nicCmdQueueV[thread] = NicCmdQueueInfo( hostInfo.reqQueueTailIndexAddress );

		memset( (void*) &nicInfo, 1, sizeof( nicInfo ) );

		nicInfo.reqQueueAddress = (thread * m_backing->getPeMemorySize() );
		nicInfo.compQueuesAddress = nicInfo.reqQueueAddress + m_backing->getCompInfoOffset();

        dbg.debug( CALL_INFO_LONG,1,DBG_X_FLAG,"cmdQAddress=%#" PRIx32 " compInfoOffset=%#" PRIx32 "\n",nicInfo.reqQueueAddress,nicInfo.compQueuesAddress);

		// add 1 because the host is looking for something other that 1 to signal it has been writen by the NICt
		nicInfo.reqQueueAddress += 1;

		nicInfo.reqQueueSize = m_backing->getCmdQSize();
		nicInfo.nodeId = m_nicId + 1;
		nicInfo.numNodes = m_numNodes;
		nicInfo.numThreads = m_backing->getNumPEs();
		nicInfo.myThread = thread + 1;

        dbg.debug( CALL_INFO_LONG,1,DBG_X_FLAG,"write info to host address %#" PRIx64 "\n",threadMemoryBase + hostInfo.respAddress);

    	m_memReqQ->write( m_tailWriteQnum, threadMemoryBase + hostInfo.respAddress, sizeof(nicInfo), reinterpret_cast<uint8_t*>(&nicInfo) );

		dbg.debug( CALL_INFO_LONG,1,DBG_X_FLAG,"thread %d setup finished\n",thread);
    }

	if ( ! req->posted ) {
		m_mmioLink->send( req->makeResponse() );
	}
    // if all of the HostQueueInfo struct has been written we are done with setup, switch to normal processing of write to MMIO space
	if ( offset == sizeof(HostQueueInfo) ) {
		m_mmioWriteFunc = std::bind( &RdmaNic::mmioWrite, this, std::placeholders::_1 );
	}
}


void RdmaNic::mmioWrite(StandardMem::Write* req) {

	int thread = calcThread( req->pAddr );
	uint64_t offset = calcOffset( req->pAddr );

    dbg.debug( CALL_INFO_LONG,2,DBG_X_FLAG,"thread=%d Write size=%zu addr=%" PRIx64 " offset=%llu data %s\n",
                    thread,req->data.size(), req->pAddr, offset, getDataStr(req->data).c_str() );

    if ( ! m_backing->write( offset, req->data.data(), req->data.size() ) ) {
        out.fatal(CALL_INFO_LONG, -1, "Failed to write: thread=%d Write size=%zu addr=%" PRIx64 " offset=%llu data %s\n",
                    thread,req->data.size(), req->pAddr, offset, getDataStr(req->data).c_str() );
    }

    if ( m_backing->isCmdReady( thread ) ) {
		NicCmdEntry* entry = createNewCmd( *this, thread, m_backing->readCmd(thread) ); 
		dbg.debug( CALL_INFO_LONG,1,DBG_X_FLAG,"new command from thread %d op=%s\n", thread, entry->name().c_str() );
        m_nicCmdQ.push( entry );
    }

	if ( ! req->posted ) {
		m_mmioLink->send( req->makeResponse() );
    }
}

bool RdmaNic::clock(Cycle_t cycle) {

#if 0
    bool unclock = m_link->clock();
#endif

    processThreadCmdQs();

    m_memReqQ->process();
	m_networkQ->process();
	m_recvEngine->process();
	m_sendEngine->process();

    return false;
}

void RdmaNic::processThreadCmdQs( ) {

#if 0
&& ! m_memReqQ->full( m_tailWriteQnum ) ) {
#endif

    if ( NULL == m_activeNicCmd && ! m_nicCmdQ.empty() ) {

		m_activeNicCmd = m_nicCmdQ.front();
		m_nicCmdQ.pop();

		NicCmdQueueInfo& info = m_nicCmdQueueV[m_activeNicCmd->getThread()]; 

		dbg.debug( CALL_INFO_LONG,1,DBG_X_FLAG,"thread %d cmd available tail=%d %s\n",m_activeNicCmd->getThread(),info.localTailIndex, m_activeNicCmd->name().c_str() );

        ++info.localTailIndex;
        info.localTailIndex %= m_backing->getCmdQSize();
        dbg.debug( CALL_INFO_LONG,1,DBG_X_FLAG,"write tail=%d at %#" PRIx64 "\n",info.localTailIndex, info.tailAddr );
        m_memReqQ->write( m_tailWriteQnum, info.tailAddr, 4, info.localTailIndex );
    }
	if ( m_activeNicCmd ) {
		if ( m_activeNicCmd->process() ) {
			dbg.debug( CALL_INFO_LONG,1,DBG_X_FLAG,"command %s has finished\n",m_activeNicCmd->name().c_str());
			delete m_activeNicCmd;
			m_activeNicCmd = NULL;
		}
	}	
}


void RdmaNic::sendRespToHost( Addr_t addr, NicResp& resp, int thread )
{
	dbg.debug( CALL_INFO_LONG,1,DBG_X_FLAG,"resp=%p retval=%p\n", &resp, &resp.retval);
	Addr_t offset = ((uint64_t) &resp.retval - (uint64_t) &resp);
	m_memReqQ->write( m_respQueueMemChannel, addr, sizeof(resp.data), reinterpret_cast<uint8_t*>(&resp) );
    m_memReqQ->fence( m_respQueueMemChannel );
	m_memReqQ->write( m_respQueueMemChannel, addr + offset, sizeof(resp.retval), reinterpret_cast<uint8_t*>(&resp) + offset );
}

void RdmaNic::writeCompletionToHost(int thread, int cqId, RdmaCompletion& comp )
{
    CompletionQueue& q = *m_compQueueMap[cqId]; 

    int tailIndex = readCompQueueTailIndex(thread,cqId);
    dbg.debug( CALL_INFO_LONG,1,DBG_X_FLAG,"cqId=%d headIndex=%d tailIndex=%d queueSize=%d headPtr=%x dataPtr=%x\n", 
		cqId, q.headIndex(), tailIndex, q.cmd().data.createCQ.num, q.cmd().data.createCQ.headPtr, q.cmd().data.createCQ.dataPtr );
    dbg.debug( CALL_INFO_LONG,2,DBG_X_FLAG,"ctx=%x\n", comp.context);

	// if we move the head index and it's equal to the tail index we are full
	// Note that with this logic the max number of items in the circular queue is N - 1
	// where N is the total number of slots
    if ( ( q.headIndex()  + 1 ) % q.cmd().data.createCQ.num == tailIndex ) {
        assert(0);
    }
	Addr_t data = q.cmd().data.createCQ.dataPtr + q.headIndex() * sizeof(comp);

	q.incHeadIndex();
	
	m_memReqQ->write( m_respQueueMemChannel, data, sizeof(comp), reinterpret_cast<uint8_t*>(&comp) );
    m_memReqQ->fence( m_respQueueMemChannel );
	m_memReqQ->write( m_respQueueMemChannel, q.cmd().data.createCQ.headPtr, sizeof(q.headIndex()), q.headIndex() );
}

void RdmaNic::init(unsigned int phase) {
	dbg.debug( CALL_INFO_LONG,2,DBG_X_FLAG,"phase=%d\n",phase);

	if ( m_dmaLink ) {
    	m_dmaLink->init(phase);
	}
    m_mmioLink->init(phase);

    m_linkControl->init(phase);
}

void RdmaNic::setup(void) {
	if ( m_dmaLink ) {
    	m_dmaLink->setup();
	}
    m_mmioLink->setup();
}


void RdmaNic::finish(void) {

	if ( m_dmaLink ) {
    	m_dmaLink->finish();
	}
    m_mmioLink->finish();
}
