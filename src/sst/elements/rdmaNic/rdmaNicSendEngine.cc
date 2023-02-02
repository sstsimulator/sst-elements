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

#include <sst_config.h>

#include "rdmaNic.h"

using namespace SST::Interfaces;
using namespace SST::MemHierarchy;

RdmaNic::SendStream::SendStream( RdmaNic& nic, SendEntry* entry ) : m_nic(nic), m_numReadsPending(0), m_offset(0),
		m_sendEntry(entry), m_streamSeqNum(0), m_streamId( nic.generateStreamId() ), m_maxQueueSize(16), readId(0), curReadId(0)
{
    m_callback = new MemRequest::Callback;
    *m_callback = std::bind( &RdmaNic::SendStream::readResp, this, m_sendEntry->getThread(), std::placeholders::_1, std::placeholders::_2 );
   	m_pkt = new RdmaNicNetworkEvent();

	StreamHdr* hdr = entry->getStreamHdr();
	hdr->seqLen = calcSeqLen();

	// copy the Stream header into the packet
    uint8_t* buf = (uint8_t*) hdr; 
    for ( int i = 0; i< sizeof(*entry->getStreamHdr()); i++ ) {
        m_pkt->getData().insert( m_pkt->getData().end(), buf[i] );
    }
	
	// calculate the number of memory reads will will need
	if ( entry->getLength() ) {
    	// how many bytes from the address to the first cache line boundary, 
    	// this may be more the total length of data
    	// this may be 0 if algined
    	int tmp = 64 - ( entry->getAddr() & (64-1 ) );

    	// buffer is not aligned and total length is less that length to cache line boundary 
    	if ( tmp && tmp >= entry->getLength() ) {
        	m_numReadsPending = 1;
    	} else {
        	// reduce the length by tmp which make the remainder aligned
        	m_numReadsPending = (entry->getLength() - tmp ) / 64;
        	if ( ( entry->getLength() - tmp ) % 64 ) {
            	++m_numReadsPending;
        	}
        	// if there was some data before the first cache line boundary
        	if ( tmp ) {
            	++m_numReadsPending;
        	}
    	}
	} else {
   		nic.dbg.debug( CALL_INFO_LONG,1,DBG_X_FLAG,"zero length message\n");
		// get a new packet even though we will not use it, this way we will not have to check if we need one every time 
		// thru the process loop
		m_pkt = queuePkt( m_pkt );	
	}

    nic.dbg.debug( CALL_INFO_LONG,1,DBG_X_FLAG,"numReadsPending=%d\n",m_numReadsPending);
}


void RdmaNic::SendStream::readResp( int thread, StandardMem::Request* req, int id ) 
{
	StandardMem::ReadResp* resp = dynamic_cast<StandardMem::ReadResp*>(req);
    m_nic.dbg.debug( CALL_INFO_LONG,1,DBG_X_FLAG,"thread=%d numReadsPending=%d resp=%p pAddr=%" PRIx64 " size=%" PRIu64 " id=%d\n",
		thread,m_numReadsPending,resp,resp->pAddr,resp->size,id);

    assert( resp->size == resp->data.size() );
#if 0
// FIXME MH is returning a full cache line even if we request a partial
	size_t len = resp->pAddr - (resp->pAddr & ~(64-1));
	// shrink the cache data buffer before our data
	if ( len ) {
		resp->data.erase( resp->data.begin(), resp->data.begin() + len );
	}
	// shrink the cache data buffer after our data
	if ( resp->data.size() > resp->size	 ) {
		resp->data.erase( resp->data.begin() + resp->size, resp->data.end() );
	}
#endif
    m_respMap[id] =resp;
}

RdmaNic::SendStream::~SendStream() {

	if ( -1 != m_sendEntry->getCqId() ) {
    	RdmaCompletion comp;
    	comp.context = m_sendEntry->getContext();
    	m_nic.writeCompletionToHost( m_sendEntry->getThread(), m_sendEntry->getCqId(), comp );
	}

    if ( m_pkt ) {
        delete m_pkt;
    }
}

bool RdmaNic::SendStream::process()
{
	// send read requests if we need to and if we are not blocked 
	if ( m_offset < m_sendEntry->getLength() && ! m_nic.m_memReqQ->full( m_nic.m_dmaMemChannel ) ) {

    	m_nic.dbg.debug( CALL_INFO_LONG,1,DBG_X_FLAG,"pe=%d node=%d addr=%" PRIx32 " len=%d offset=%zu\n",
            m_sendEntry->getDestPid(), m_sendEntry->getDestNode(), m_sendEntry->getAddr(), m_sendEntry->getLength(), m_offset );

       	uint64_t addr = m_sendEntry->getAddr() + m_offset;
       	int len = addr & (64-1) ? 64 - ( addr & (64-1) ) : 64;
       	len = len > m_sendEntry->getLength() - m_offset ? m_sendEntry->getLength() - m_offset : len;
       	m_nic.m_memReqQ->read( m_nic.m_dmaMemChannel, addr, len, readId++, m_callback);
       	m_offset += len;
	}

	// we have a response back from memory read
	if ( m_respMap.find( curReadId ) != m_respMap.end() ) {
		auto resp = m_respMap[curReadId];

        m_nic.dbg.debug( CALL_INFO_LONG,1,DBG_X_FLAG,"resp=%p size=%zu\n", resp, resp->data.size() );
		int xferLen = m_nic.getNetPktMtuLen() - m_pkt->getData().size();
		xferLen = resp->data.size() < xferLen ? resp->data.size() : xferLen;   

        m_nic.dbg.debug( CALL_INFO_LONG,1,DBG_X_FLAG,"copy %d bytes to network packet\n", xferLen );
        m_pkt->getData().insert( m_pkt->getData().end(), resp->data.begin(), resp->data.begin() + xferLen );
        m_nic.dbg.debug( CALL_INFO_LONG,1,DBG_X_FLAG,"mem %s\n",getDataStr(resp->data,xferLen).c_str());
		resp->data.erase( resp->data.begin(), resp->data.begin() + xferLen );

		if ( resp->data.empty() ) { 
        	m_nic.dbg.debug( CALL_INFO_LONG,1,DBG_X_FLAG,"all data moved from read request\n" );
			--m_numReadsPending;
			delete resp;
			m_respMap.erase(curReadId);
			++curReadId;
		}

        if ( m_pkt->getData().size() == m_nic.getNetPktMtuLen() || 0 == m_numReadsPending ) {
			m_nic.dbg.debug(CALL_INFO_LONG,1,DBG_X_FLAG,"%p %p\n",m_pkt,m_pkt->getData().data());

            m_nic.dbg.debug( CALL_INFO_LONG,1,DBG_X_FLAG,"queue packet with %zu bytes\n",m_pkt->getData().size());
			m_pkt = queuePkt(m_pkt);
        }
    }

	processReadyQ();

	// if there are no pending reads left then all read response have been transferred to network packets
	// and if the ready Q is empty all packets have been sent so this stream is done
    return ( 0 == m_numReadsPending && m_readyPktQ.empty() );
}
