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

Nic::RecvMachine::MsgStream::MsgStream( Output& output, FireflyNetworkEvent* ev,
       RecvMachine& rm, int unit, int srcNode, int srcPid, int destPid ) : 
    StreamBase(output,rm,unit,srcNode,srcPid,destPid)
{
    // FIX ME, don't save the header
    m_hdr = *(MsgHdr*) ev->bufPtr();
    MatchMsgHdr& matchHdr = *(MatchMsgHdr*) ev->bufPtr( sizeof(MsgHdr) );

    m_dbg.verbose(CALL_INFO,1,NIC_DBG_RECV_MACHINE,"Msg Operation srcNode=%d tag=%#x length=%lu\n",
                            m_srcNode,matchHdr.tag,matchHdr.len);

    Callback callback;

    m_recvEntry = static_cast<DmaRecvEntry *>( m_rm.findRecv( m_myPid, m_srcNode, m_srcPid, m_hdr, matchHdr ) );

    if ( m_recvEntry ) {
        ev->bufPop( sizeof(MsgHdr) );
        ev->bufPop( sizeof(MatchMsgHdr) );

        callback = std::bind( &Nic::RecvMachine::StreamBase::processPkt, this, ev );

    } else {
        callback = std::bind( &Nic::RecvMachine::StreamBase::needRecv, this, ev );
    }
    m_rm.nic().schedCallback( callback, m_rm.m_rxMatchDelay );

    m_matched_len = matchHdr.len;
    m_matched_tag = matchHdr.tag;
}
