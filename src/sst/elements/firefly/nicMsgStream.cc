// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2017, Sandia Corporation
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

Nic::RecvMachine::MsgStream::MsgStream( Output& output, Ctx* ctx, int unit,
        int srcNode, int srcPid, int destPid, FireflyNetworkEvent* ev ) : 
    StreamBase(output,ctx,unit,srcNode,srcPid,destPid)
{
    MsgHdr& hdr =           *(MsgHdr*) ev->bufPtr();
    MatchMsgHdr& matchHdr = *(MatchMsgHdr*) ev->bufPtr( sizeof(MsgHdr) );

    m_dbg.debug(CALL_INFO,1,NIC_DBG_RECV_MACHINE,"Msg Operation srcNode=%d tag=%#x length=%lu\n",
                            m_srcNode,matchHdr.tag,matchHdr.len);

    m_recvEntry = static_cast<DmaRecvEntry *>( m_ctx->findRecv( m_srcNode, m_srcPid, hdr, matchHdr ) );

    m_matched_len = matchHdr.len;
    m_matched_tag = matchHdr.tag;
    m_startDelay = m_ctx->getRxMatchDelay();

    if ( m_recvEntry ) {
        ev->bufPop( sizeof(MsgHdr) + sizeof(MatchMsgHdr) );

        m_startCallback = std::bind( &Nic::RecvMachine::StreamBase::processPkt, this, ev );

    } else {
        m_startCallback = std::bind( &Nic::RecvMachine::StreamBase::needRecv, this, ev );
    }
}
