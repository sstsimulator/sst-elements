// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include "sst_config.h"
#include "nic.h"

using namespace SST;
using namespace SST::Firefly;

Nic::RecvMachine::MsgStream::MsgStream( Output& output, Ctx* ctx,
        int srcNode, int srcPid, int destPid, FireflyNetworkEvent* ev ) : 
    StreamBase(output,ctx,srcNode,srcPid,destPid), m_blocked(false)
{
    m_unit = m_ctx->allocRecvUnit();
    m_dbg.debug(CALL_INFO,1,NIC_DBG_RECV_STREAM,"%p\n",this);

    MsgHdr& hdr =           *(MsgHdr*) ev->bufPtr();
    MatchMsgHdr& matchHdr = *(MatchMsgHdr*) ev->bufPtr( sizeof(MsgHdr) );
    m_matched_len = matchHdr.len;
    m_matched_tag = matchHdr.tag;

    m_dbg.debug(CALL_INFO,1,NIC_DBG_RECV_STREAM,"Msg Operation srcNode=%d tag=%#x length=%lu\n",
                            m_srcNode,matchHdr.tag,matchHdr.len);

    m_recvEntry = static_cast<DmaRecvEntry *>( m_ctx->findRecv( m_srcNode, m_srcPid, hdr, matchHdr ) );

    Callback callback;
    if ( NULL== m_recvEntry ) {
        callback =  std::bind( &Nic::RecvMachine::StreamBase::needRecv, this, ev );
    } else {
        m_blocked = true;
        ev->bufPop( sizeof(MsgHdr) + sizeof(MatchMsgHdr) );
        ev->clearHdr();
        callback = std::bind( &Nic::RecvMachine::MsgStream::processFirstPkt, this, ev );
    }
    m_ctx->nic().schedCallback( callback,  m_ctx->getRxMatchDelay() );
}
