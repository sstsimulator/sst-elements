
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

#include "rdmaNic.h"
using namespace SST::Interfaces;
using namespace SST::MemHierarchy;

void RdmaNic::RecvEngine::process()
{
    for ( int i = 0; i < queues.size(); i++ ) {
        if ( ! busy( i ) ) {
            SST::Interfaces::SimpleNetwork::Request* req = nic.m_linkControl->recv(i);
            if ( req ) {
                Event* payload;
                if ( ( payload = req->takePayload() ) ) {

                    nic.dbg.debug(CALL_INFO_LONG,1,DBG_X_FLAG,"got packet on vc=%d\n",i);

                    RdmaNicNetworkEvent* event = static_cast<RdmaNicNetworkEvent*>(payload);

                    add( i, event );
                }
            }
            delete req;
        }
        processQueuedPkts( queues[i] );
    }
	auto iter = m_recvStreamMap.begin();

	while ( iter != m_recvStreamMap.end() ) { 
		if ( iter->second->process() ) {
			delete iter->second;
			nic.dbg.debug(CALL_INFO_LONG,1,DBG_X_FLAG,"delete stream %zu\n",m_recvStreamMap.size());
			iter = m_recvStreamMap.erase(iter);
			nic.dbg.debug(CALL_INFO_LONG,1,DBG_X_FLAG,"delete stream %zu\n",m_recvStreamMap.size());
		} else {
			++iter;
		}
	}
}

void RdmaNic::RecvEngine::processQueuedPkts( std::queue< RdmaNicNetworkEvent* >& pktQ )
{
    if ( ! pktQ.empty() ) {

        RdmaNicNetworkEvent* pkt = pktQ.front();

		if ( RdmaNicNetworkEvent::Stream == pkt->getPktType() ) {

        	nic.dbg.debug(CALL_INFO_LONG,1,DBG_X_FLAG,"packet from node=%d pid=%d destPid=%d streamId=%d seqNum=%d pktlen=%zu\n",
            pkt->getSrcNode(),pkt->getSrcPid(),pkt->getDestPid(),pkt->getStreamId(),pkt->getStreamSeqNum(), pkt->getData().size());
        	if ( 0 == pkt->getStreamSeqNum() ) {
            	processStreamHdr( pkt );
				// the header was removed from the packet so now the payload can be processed
        	}
			processPayloadPkt( pkt );
		} else {
			nic.m_barrier->givePkt( pkt );
		}
        pktQ.pop();
    }
}

void RdmaNic::RecvEngine::processStreamHdr( RdmaNicNetworkEvent* pkt )
{
    StreamHdr* hdr = (StreamHdr*) pkt->getData().data();
    nic.dbg.debug(CALL_INFO_LONG,1,DBG_X_FLAG,"stream type=%d payloadSize=%d numOfPkts=%d pktLen=%zu\n", 
					hdr->type, hdr->payloadLength, hdr->seqLen, pkt->getData().size() );

    switch( hdr->type ) {
      case StreamHdr::Msg:
		processMsgHdr( pkt );
        break;
      case StreamHdr::Write:
		processWriteHdr( pkt );
        break;
      case StreamHdr::ReadReq:
		processReadReqHdr( pkt );
        break;
      case StreamHdr::ReadResp:
		processReadRespHdr( pkt );
        break;
      default:
        assert(0);
    }
	// Make sure this is last becuase it removes the hdr from the packet
	pkt->getData().erase( pkt->getData().begin(), pkt->getData().begin() + sizeof(StreamHdr));
}

void RdmaNic::RecvEngine::processMsgHdr( RdmaNicNetworkEvent* pkt ) 
{
    StreamHdr* hdr = (StreamHdr*) pkt->getData().data();
	int rqId;
	RecvQueue* queue;
	try {
		rqId = m_recvQueueKeyMap.at( hdr->data.msgKey );
	} catch ( std::exception& e ) {
		nic.out.fatal(CALL_INFO_LONG, -1, "%s, Error: could not find receive queue with key %#x\n", nic.getName().c_str(), hdr->data.msgKey);
	}
	try {
		queue = m_recvQueueMap.at( rqId );
	} catch ( std::exception& e ) {
		nic.out.fatal(CALL_INFO_LONG, -1, "%s, Error: could not find receive queue with id %d\n", nic.getName().c_str(), rqId);
	}
	if ( queue->getQ().empty() ) {
		nic.out.fatal(CALL_INFO_LONG, -1, "%s, Error: receive queue %d \n", nic.getName().c_str(), rqId);
	}

	auto entry = queue->getQ().front();
	queue->getQ().pop();
	if ( entry->getPayloadLength() < hdr->payloadLength ) {
		nic.out.fatal(CALL_INFO_LONG, -1, "%s, Error: incoming message (%zu) is too big for receive buffer (%zu)\n", 
					nic.getName().c_str(), rqId,hdr->payloadLength, entry->getPayloadLength() );
	}
	Addr_t destAddr = entry->getAddr();
	nic.dbg.debug(CALL_INFO_LONG,1,DBG_X_FLAG,"key=%#x rqId=%d destAddr=%#x\n", hdr->data.msgKey, rqId, destAddr );

	m_recvStreamMap[ calcNodeStreamId( pkt->getSrcNode(), pkt->getStreamId() ) ] = new RecvStream( nic, destAddr, hdr->payloadLength, entry );

}
void RdmaNic::RecvEngine::processWriteHdr( RdmaNicNetworkEvent* pkt ) 
{
    StreamHdr* hdr = (StreamHdr*) pkt->getData().data();
	int rqId;
    nic.dbg.debug(CALL_INFO_LONG,1,DBG_X_FLAG,"memRgnKey=%#x offset=%zu\n", hdr->data.rdma.memRgnKey, hdr->data.rdma.offset );
	MemRgnEntry* entry;
	try {
		entry = m_memRegionMap.at( hdr->data.rdma.memRgnKey );
	} catch ( std::exception& e ) {
		nic.out.fatal(CALL_INFO_LONG, -1, "%s, Error: could not find memory region with key %#x\n", nic.getName().c_str(), hdr->data.rdma.memRgnKey);
	}

// FIXME check for length violation

	Addr_t destAddr = entry->getAddr() + hdr->data.rdma.offset;
    nic.dbg.debug(CALL_INFO_LONG,1,DBG_X_FLAG,"destAddr=%x\n", destAddr );
	m_recvStreamMap[ calcNodeStreamId( pkt->getSrcNode(), pkt->getStreamId() ) ] = new RecvStream( nic, destAddr, hdr->payloadLength, entry );
}

void RdmaNic::RecvEngine::processReadReqHdr( RdmaNicNetworkEvent* pkt ) 
{
    StreamHdr* hdr = (StreamHdr*) pkt->getData().data();
    nic.dbg.debug(CALL_INFO_LONG,1,DBG_X_FLAG,"memRgnKey=%#x offset=%zu readLength=%zu\n", hdr->data.rdma.memRgnKey, hdr->data.rdma.offset, hdr->data.rdma.readLength );
	
	MemRgnEntry* memRgnEntry;
	try {
		memRgnEntry = m_memRegionMap.at( hdr->data.rdma.memRgnKey );
	} catch ( std::exception& e ) {
		nic.out.fatal(CALL_INFO_LONG, -1, "%s, Error: could not find memory region with key %#x\n", nic.getName().c_str(), hdr->data.rdma.memRgnKey);
	}
	Addr_t srcAddr = memRgnEntry->getAddr() + hdr->data.rdma.offset;	

	m_recvStreamMap[ calcNodeStreamId( pkt->getSrcNode(), pkt->getStreamId() ) ] = new RecvStream( nic, 0, 0, NULL );
	
	nic.dbg.debug(CALL_INFO_LONG,1,DBG_X_FLAG,"destPid=%d srcNode=%d srcPid=%d readRespKey=%d\n",
				pkt->getDestPid(), pkt->getSrcNode(), pkt->getSrcPid(), hdr->data.rdma.readRespKey );
	SendEntry* sendEntry = new ReadRespSendEntry( pkt->getDestPid(), pkt->getSrcNode(), pkt->getSrcPid(), srcAddr, hdr->data.rdma.readLength, hdr->data.rdma.readRespKey );   
	nic.m_sendEngine->add( 0, sendEntry );
}

void RdmaNic::RecvEngine::processReadRespHdr( RdmaNicNetworkEvent* pkt ) 
{
    StreamHdr* hdr = (StreamHdr*) pkt->getData().data();
    nic.dbg.debug(CALL_INFO_LONG,1,DBG_X_FLAG,"readRespKey=%#x payloadLength=%zu\n", hdr->data.rdma.readRespKey, hdr->payloadLength );
	ReadRespRecvEntry* entry;
	try {
		entry = m_readRespMap.at( hdr->data.rdma.readRespKey );
	} catch ( std::exception& e ) {
		nic.out.fatal(CALL_INFO_LONG, -1, "%s, Error: could not find read response buffer with key %#x\n", nic.getName().c_str(), hdr->data.rdma.readRespKey);
	}

// FIXME check for length violation

	m_readRespMap.erase( hdr->data.rdma.readRespKey );
	m_recvStreamMap[ calcNodeStreamId( pkt->getSrcNode(), pkt->getStreamId() ) ] = new RecvStream( nic, entry->getAddr(), hdr->payloadLength, entry );
}


void RdmaNic::RecvEngine::processPayloadPkt( RdmaNicNetworkEvent* pkt ) 
{
	RecvStream* stream;
    nic.dbg.debug(CALL_INFO_LONG,1,DBG_X_FLAG,"streamId=%d pktSeqNum=%d pktLen=%zu\n", pkt->getStreamId(), pkt->getStreamSeqNum(), pkt->getData().size() );
	try {
		stream = m_recvStreamMap.at( calcNodeStreamId( pkt->getSrcNode(), pkt->getStreamId() ) );
	} catch ( std::exception& e ) {
		nic.out.fatal(CALL_INFO_LONG, -1, "%s, Error: can't find stream %d\n", nic.getName().c_str(), pkt->getStreamId() );
	}
	stream->addPkt( pkt );
}

void RdmaNic::RecvStream::writeResp( int thread, StandardMem::Request* req ) {
	StandardMem::WriteResp* resp = dynamic_cast<StandardMem::WriteResp*>(req);
	bytesWritten += resp->size; 
    nic.dbg.debug( CALL_INFO_LONG,1,DBG_X_FLAG,"thread=%d btyes=%" PRIu64 " bytesWritten=%d\n",thread,resp->size,bytesWritten);
	if ( ! resp->getSuccess() ) {
        nic.out.fatal(CALL_INFO_LONG, -1, " Error: write to address %#" PRIx64 " failed\n",resp->pAddr );
    }

	delete resp;
}

RdmaNic::RecvStream::~RecvStream() {
	if ( recvEntry ) {
    	nic.dbg.debug( CALL_INFO_LONG,1,DBG_X_FLAG,"cqId=%d\n",recvEntry->getCqId());
		if ( -1 != recvEntry->getCqId() ) {
    		RdmaCompletion comp;
    		comp.context = recvEntry->getContext();
    		nic.writeCompletionToHost( recvEntry->getThread(), recvEntry->getCqId(), comp );
		}
		delete recvEntry;
	}
}

static std::string getDataStr( std::vector<uint8_t>& data, size_t len ) {
    std::ostringstream tmp(std::ostringstream::ate);
//        tmp << std::hex;
        for ( auto i = 0; i < data.size() && i < len; i++ ) {
//            tmp << "0x" << (int) data.at(i)  << ",";
            tmp << (int) data.at(i)  << ",";
        }
    return tmp.str();
}


bool RdmaNic::RecvStream::process() {

	// if bytes writen equal length we must be done
	if ( bytesWritten == length ) {
		if ( ! pktQ.empty() ) {
			delete pktQ.front();
			pktQ.pop();
		}
    	nic.dbg.debug( CALL_INFO_LONG,1,DBG_X_FLAG,"all writes have completed, stream is done\n");
		return true;
	}

	// if the packetQ is empty there is nothing to do until we get another packet
	// OR is the memory request queue is full
	if ( pktQ.empty() || nic.m_memReqQ->full( nic.m_dmaMemChannel ) ) {
		return false;
	}

    nic.dbg.debug( CALL_INFO_LONG,1,DBG_X_FLAG,"offset=%zu length=%zu\n",offset,length);
	auto pkt = pktQ.front();
	Addr_t addr = destAddr + offset;
    int len = calcLen( addr, pkt->getData().size(), 64 );
    nic.dbg.debug( CALL_INFO_LONG,1,DBG_X_FLAG,"destAddr=%#x len=%d\n",addr,len);
    nic.dbg.debug( CALL_INFO_LONG,1,DBG_X_FLAG,"%s\n",getDataStr(pkt->getData(),len).c_str());

	if ( len ) {
    	nic.m_memReqQ->write( nic.m_dmaMemChannel, addr, len, pkt->getData().data(), callback );
		offset += len;
    	pkt->getData().erase( pkt->getData().begin(), pkt->getData().begin() + len);
	}

	if ( pkt->getData().size() == 0 ) {
    	nic.dbg.debug( CALL_INFO_LONG,1,DBG_X_FLAG,"done with packet\n");
		delete pkt;
		pktQ.pop();
	}
	return false;
}
