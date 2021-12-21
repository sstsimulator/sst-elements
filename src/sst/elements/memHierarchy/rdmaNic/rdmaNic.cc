// Copyright 2009-2019 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2019, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include <sst/core/params.h>
#include <sst/core/simulation.h>

#include "rdmaNic.h"

using namespace SST::Interfaces;
using namespace SST::MemHierarchy;

int round_up(int num, int factor)
{
    return num + factor - 1 - (num + factor - 1) % factor;
}

RdmaNic::RdmaNic(ComponentId_t id, Params &params) : Component(id),
	m_activeNicCmd(NULL),
	m_nextCqId(0),
	m_streamId(1),
	m_netPktMtuLen( 1024 )
{
    m_nicId = params.find<int>("nicId", -1);
    m_numNodes = params.find<int>("numNodes", 0);

	printf("%s() sizeof(NicCmd) %zu\n",__func__,sizeof(NicCmd));

	out.init("RdmaNic", params.find<int>("verbose", 1), 0, Output::STDOUT);

    // Output for debug
    char buffer[100];
    snprintf(buffer,100,"@t:node%d:RdmaNic::@p():@l ",m_nicId);
    dbg.init( buffer, 
        params.find<int>("debug_level", 0),
        params.find<int>("debug_mask",0), 
		Output::STDOUT );

    m_pesPerNode = params.find<int>("pesPerNode", 0);
	assert( m_pesPerNode > 0 );

	m_mmioWriteFunc = std::bind( &RdmaNic::mmioWriteSetup, this, std::placeholders::_1 );

    m_ioBaseAddr = params.find<uint64_t>( "baseAddr", 0x100000000 );
    m_cmdQSize = params.find<int>("cmdQSize", 64);

    m_perPeReqQueueMemSize = m_cmdQSize * sizeof(NicCmd) + 64; // 64 bytes for for queue index info
    m_perPeReqQueueMemSize =  round_up(m_perPeReqQueueMemSize, 4096);

	m_perPeTotalMemSize = m_perPeReqQueueMemSize;

	// do we need this?
    m_cacheLineSize = params.find<uint64_t>("cache_line_size",0);

    assert( m_nicId != -1 );
    assert( m_cmdQSize );
    assert( m_pesPerNode );
    assert( m_ioBaseAddr != -1 );

    m_cmdBytesWritten.resize( m_perPeReqQueueMemSize * m_pesPerNode, 0 );
    m_reqQueueBacking.resize( m_perPeReqQueueMemSize * m_pesPerNode );
	m_compQueuesBacking.resize( m_pesPerNode, std::vector<int>( NUM_COMP_Q, 0 ) );

	m_perPeTotalMemSize += round_up( m_compQueuesBacking[0].size() ,4096 );	

    dbg.debug(CALL_INFO_LONG,1,DBG_X_FLAG,"\n");
    if ( m_nicId == 0 ) {
        dbg.debug(CALL_INFO_LONG,1,DBG_X_FLAG,"nicId=%d pesPerNode=%d numNodes=%d\n",m_nicId,m_pesPerNode,m_numNodes );
		dbg.debug(CALL_INFO_LONG,1,DBG_X_FLAG,"numCmdQueueEntryes=%d cmdQsize %zu perPeReqQueueMemSize=%zu\n", m_cmdQSize, m_cmdQSize* sizeof(NicCmd),  m_perPeReqQueueMemSize );
        dbg.debug(CALL_INFO_LONG,1,DBG_X_FLAG,"nicBaseAddr=%#" PRIx64 " total size command Q space %zu\n",  m_ioBaseAddr, m_reqQueueBacking.size()); 
        dbg.debug(CALL_INFO_LONG,1,DBG_X_FLAG,"nicCompQueueAddr=%#" PRIx64 " compQ backing size %zu\n",  m_ioBaseAddr + m_reqQueueBacking.size(), m_compQueuesBacking[0].size() );
		dbg.debug(CALL_INFO_LONG,1,DBG_X_FLAG,"total per PE memory size %zu\n", m_perPeTotalMemSize);
    }

    m_threadInfo.resize( m_pesPerNode );
    m_nicCmdQueueV.resize( m_pesPerNode );

    // Clock handler
    std::string clockFreq = params.find<std::string>("clock", "1GHz");
    m_clockHandler = new Clock::Handler<RdmaNic>(this, &RdmaNic::clock);
    m_clockTC = registerClock( clockFreq, m_clockHandler );

    m_mmioLink = loadUserSubComponent<StandardMem>("mmio", ComponentInfo::SHARE_NONE, m_clockTC, new StandardMem::Handler<RdmaNic>(this, &RdmaNic::handleEvent));

    if (!m_mmioLink) {
        out.fatal(CALL_INFO_LONG, -1, "Unable to load memHierarchy.standardInterface subcomponent; check that 'mmio' slot is filled in input.\n");
    }
	m_mmioLink->setMemoryMappedAddressRegion(m_ioBaseAddr, 1024*1024 * m_pesPerNode);

	m_useDmaCache = params.find<bool>("useDmaCache",false);
	if ( m_useDmaCache ) {
    	m_dmaLink = loadUserSubComponent<StandardMem>("dma", ComponentInfo::SHARE_NONE, m_clockTC, new StandardMem::Handler<RdmaNic>(this, &RdmaNic::handleEvent));

    	if (!m_dmaLink) {
        	out.fatal(CALL_INFO_LONG, -1, "Unable to load memHierarchy.standardInterface subcomponent; check that 'mmio' slot is filled in input.\n");
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
    m_memReqQ = new MemRequestQ( this, maxPending, maxPending/numSrc, numSrc );

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

static std::string getDataStr( std::vector<uint8_t>& data ) {
    std::ostringstream tmp(std::ostringstream::ate);
        tmp << std::hex;
        for ( auto i = 0; i < data.size(); i++ ) {
            tmp << "0x" << (int) data.at(i)  << ",";
        }
	return tmp.str();
}

void RdmaNic::mmioWriteSetup( StandardMem::Write* req) {
	// each thread will write a pointer to a block in memory at offset  
    int thread = ( req->pAddr - m_ioBaseAddr ) / m_perPeTotalMemSize;

	// Does this need to be a permanent structure?
    auto& info = m_threadInfo[thread].setupInfo;

    dbg.debug( CALL_INFO_LONG,1,DBG_X_FLAG,"Write size=%zu addr=%" PRIx64 " offset=%llu thread=%d data %s\n",
                    req->data.size(), req->pAddr, req->pAddr - m_ioBaseAddr, thread, getDataStr(req->data).c_str() );

	memcpy( (uint8_t*) (&info.hostInfo) + info.offset, req->data.data(), req->data.size() );

	info.offset += req->data.size(); 

	if ( info.offset == sizeof(HostQueueInfo) ) {

		uint64_t threadMemoryBase = 0;

		dbg.debug( CALL_INFO_LONG,1,DBG_X_FLAG,"resp Address=%#" PRIx32 "\n", 
				info.hostInfo.respAddress );

		// set up the info about the req queue in the NIC
		m_nicCmdQueueV[thread] = NicCmdQueueInfo( info.hostInfo.reqQueueTailIndexAddress );

		memset( (void*) &info.nicInfo, 1, sizeof( info.nicInfo ) );
		info.nicInfo.reqQueueAddress = m_ioBaseAddr + thread * m_perPeReqQueueMemSize;
		info.nicInfo.reqQueueHeadIndexAddress = (info.nicInfo.reqQueueAddress + m_perPeReqQueueMemSize ) - 8; 
		info.nicInfo.reqQueueSize = m_cmdQSize;
		info.nicInfo.nodeId = m_nicId + 1;
		info.nicInfo.numNodes = m_numNodes; 
		info.nicInfo.numThreads = m_pesPerNode;
		info.nicInfo.myThread = thread + 1;

    	m_memReqQ->write( m_tailWriteQnum, threadMemoryBase + info.hostInfo.respAddress, sizeof(info.nicInfo), reinterpret_cast<uint8_t*>(&info.nicInfo) );

       	uint32_t* tailPtr = (uint32_t*) (m_reqQueueBacking.data() + m_perPeReqQueueMemSize * thread + m_perPeReqQueueMemSize  - 4); 

		dbg.debug( CALL_INFO_LONG,1,DBG_X_FLAG,"thread %d setup finished\n",thread);
    }

	if ( ! req->posted ) {
		m_mmioLink->send( req->makeResponse() );
	}
	if ( info.offset == sizeof(HostQueueInfo) ) {
		m_mmioWriteFunc = std::bind( &RdmaNic::mmioWrite, this, std::placeholders::_1 );
	}
}


void RdmaNic::mmioWrite(StandardMem::Write* req) {

	int thread = ( req->pAddr - m_ioBaseAddr ) / m_perPeTotalMemSize;

    dbg.debug( CALL_INFO_LONG,2,DBG_X_FLAG,"thread=%d Write size=%zu addr=%" PRIx64 " offset=%llu data %s\n",
                    thread,req->data.size(), req->pAddr, req->pAddr - m_ioBaseAddr, getDataStr(req->data).c_str() );

	uint64_t offset = req->pAddr - m_ioBaseAddr;

	if ( offset >= m_perPeReqQueueMemSize ) {
		
		offset -= m_perPeReqQueueMemSize;
		memcpy( &m_compQueuesBacking[thread][ offset/sizeof(QueueIndex) ], req->data.data(), req->data.size() );
		dbg.debug( CALL_INFO_LONG,2,DBG_X_FLAG,"Write of Comp Queue tail %d %x\n", offset/sizeof(QueueIndex), m_compQueuesBacking[thread][ offset/sizeof(QueueIndex) ]);

	} else { 

		int index = offset / sizeof(NicCmd);
		m_cmdBytesWritten[index] += req->data.size();
		dbg.debug( CALL_INFO_LONG,2,DBG_X_FLAG,"index=%d bytesWriten=%d\n",index,m_cmdBytesWritten[index]);
		memcpy( m_reqQueueBacking.data() + offset, req->data.data(), req->data.size() );
        if ( m_cmdBytesWritten[index] == sizeof(NicCmd) ) {
			NicCmd* cmd = reinterpret_cast<NicCmd*>(m_reqQueueBacking.data() + index * sizeof(NicCmd) );

			NicCmdEntry* entry = createNewCmd( *this, thread, cmd ); 
		//	dbg.debug( CALL_INFO_LONG,1,DBG_X_FLAG,"new command %p from thread %d op=%s respAddr=%#x\n",cmd, thread, entry->name().c_str(), cmd->respAddr );
			m_nicCmdQ.push( entry );

			m_cmdBytesWritten[index] = 0;
		}
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
        info.localTailIndex %= m_cmdQSize;		
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
    if ( ( q.headIndex()  + 1 ) % q.cmd().data.createCQ.num == q.headIndex() ) {
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

	if ( m_useDmaCache ) {
    	m_dmaLink->init(phase);
	}
    m_mmioLink->init(phase);

    m_linkControl->init(phase);
}

void RdmaNic::setup(void) {
	if ( m_useDmaCache ) {
    	m_dmaLink->setup();
	}
    m_mmioLink->setup();
}


void RdmaNic::finish(void) {

	if ( m_useDmaCache ) {
    	m_dmaLink->finish();
	}
    m_mmioLink->finish();
}
