// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include "sst_config.h"
#include "nic.h"

using namespace SST;
using namespace SST::Firefly;
using namespace SST::Interfaces;

static void print( Output& dbg, char* buf, int len );

Nic::SendMachine::~SendMachine() 
{
    while ( ! m_sendQ.empty() ) {
        delete m_sendQ.front();
        m_sendQ.pop_front();
    }

    if ( m_currentSend ) delete m_currentSend;
}

void Nic::SendMachine::run( SendEntry* entry )
{
    m_dbg.verbose(CALL_INFO,1,0,"SendMachine\n");

    if ( entry ) {
        m_sendQ.push_back( entry );
        if ( WaitTX == m_state || WaitRead == m_state || WaitDelay == m_state ){
            return;
        }
    }

    bool blocked = false;
    while ( ! blocked ) {

        switch ( m_state ) {
          case Idle:
            assert ( ! m_sendQ.empty() );

            m_currentSend = m_sendQ.front();
            m_sendQ.pop_front();

            m_nic.schedEvent(
                    new SelfEvent( SelfEvent::RunSendMachine ), m_txDelay );

            blocked = true;
            m_state = WaitDelay;
            break; 

          case WaitTX:
            m_nic.m_linkControl->setNotifyOnSend( NULL );
            // fall through

          case WaitRead:
          case WaitDelay:
            m_state = Sending;
            // fall through

          case Sending:
            m_state = processSend( m_currentSend );
            if ( Idle == m_state ) {
                m_currentSend = NULL;
                if ( m_sendQ.empty() ) {
                    blocked = true;
                }
            } else {
                if ( WaitTX == m_state )  {
                    m_nic.m_linkControl->setNotifyOnSend(
                                        m_nic.m_sendNotifyFunctor );
                }
                blocked = true;
            }
            break;
        }
    }
}

Nic::SendMachine::State Nic::SendMachine::processSend( SendEntry* entry )
{
    bool ret = false;
    while ( entry ) 
    {
        if ( ! m_nic.m_linkControl->spaceToSend(0,m_packetSizeInBits)  ) {
            return WaitTX;
        }

        if ( ! m_nic.m_arbitrateDMA->canIRead( m_packetSizeInBits/8 ) ) {
            return WaitRead;
        }

        FireflyNetworkEvent* ev = new FireflyNetworkEvent;
        SimpleNetwork::Request* req = new SimpleNetwork::Request();
        
        if ( 0 == entry->currentVec && 0 == entry->currentPos  ) {
            
            MsgHdr hdr;

            hdr.op= entry->getOp();
            hdr.tag = entry->tag();
            hdr.len = entry->totalBytes();
            hdr.dst_vNicId = entry->dst_vNic();
            hdr.src_vNicId = entry->local_vNic(); 

            m_dbg.verbose(CALL_INFO,1,0,"src_vNic=%d, send dstNid=%d "
                    "dst_vNic=%d tag=%#x bytes=%lu\n",
                    hdr.src_vNicId,entry->node(), 
                    hdr.dst_vNicId, hdr.tag, entry->totalBytes()) ;

            ev->bufAppend( &hdr, sizeof(hdr) );
        }
        ret = copyOut( m_dbg, *ev, *entry ); 

        // ev->setDest( m_nic.IdToNet( entry->node() ) );
        // ev->setSrc( m_nic.IdToNet( m_nic.m_myNodeId ) );
        // ev->setPktSize();
        req->dest = m_nic.IdToNet( entry->node() );
        req->src = m_nic.IdToNet( m_nic.m_myNodeId );
        req->size_in_bits = ev->bufSize() * 8;
        req->vn = 0;
        req->payload = ev;

        #if 0
        req->setTraceType( SimpleNetwork::Request::FULL );
        req->setTraceID( m_packetId++ );
        #endif
        m_dbg.verbose(CALL_INFO,2,0,"sending event with %lu bytes\n",
                                                        ev->bufSize());
        assert( ev->bufSize() );
        bool sent = m_nic.m_linkControl->send( req, 0 );
        assert( sent );

        if ( ret ) {

            m_dbg.verbose(CALL_INFO,1,0,"send entry done\n");

            entry->notify();

            delete entry;
            entry = NULL;
        }
    }
    return Idle;
}

static void print( Output& dbg, char* buf, int len )
{
    std::string tmp;
    dbg.verbose(CALL_INFO,4,0,"addr=%p len=%d\n",buf,len);
    for ( int i = 0; i < len; i++ ) {
        dbg.verbose(CALL_INFO,3,0,"%#03x\n",(unsigned char)buf[i]);
    }
}

bool  Nic::SendMachine::copyOut( Output& dbg,
                    FireflyNetworkEvent& event, Nic::Entry& entry )
{
    dbg.verbose(CALL_INFO,3,0,"ioVec.size()=%lu\n", entry.ioVec().size() );

    for ( ; entry.currentVec < entry.ioVec().size() &&
                event.bufSize() <  m_packetSizeInBytes;
                entry.currentVec++, entry.currentPos = 0 ) {

        dbg.verbose(CALL_INFO,3,0,"vec[%lu].len %lu\n",entry.currentVec,
                    entry.ioVec()[entry.currentVec].len );

        if ( entry.ioVec()[entry.currentVec].len ) {
            size_t toLen = m_packetSizeInBytes - event.bufSize();
            size_t fromLen = entry.ioVec()[entry.currentVec].len -
                                                        entry.currentPos;

            size_t len = toLen < fromLen ? toLen : fromLen;

            dbg.verbose(CALL_INFO,3,0,"toBufSpace=%lu fromAvail=%lu, "
                            "memcpy len=%lu\n", toLen,fromLen,len);

            const char* from = 
                    (const char*) entry.ioVec()[entry.currentVec].ptr + 
                                                        entry.currentPos;

            if ( entry.ioVec()[entry.currentVec].ptr ) {
                event.bufAppend( from, len );
            } else {
                event.bufAppend( NULL, len );
            }

            entry.currentPos += len;
            if ( event.bufSize() == m_packetSizeInBytes &&
                    entry.currentPos != entry.ioVec()[entry.currentVec].len ) {
                break;
            }
        }
    }
    dbg.verbose(CALL_INFO,3,0,"currentVec=%lu, currentPos=%lu\n",
                entry.currentVec, entry.currentPos);
    return ( entry.currentVec == entry.ioVec().size() ) ;
}
