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
       RecvMachine& rm, int unit  ) : 
    StreamBase(output,rm,unit)
{
    m_hdr = *(MsgHdr*) ev->bufPtr();
    MatchMsgHdr& matchHdr = *(MatchMsgHdr*) ev->bufPtr( sizeof(MsgHdr) );
    m_src = ev->src;

    m_dbg.verbose(CALL_INFO,1,NIC_DBG_RECV_MACHINE,"Msg Operation srcNode=%d tag=%#x lenght=%lu\n",
                            m_src,matchHdr.tag,matchHdr.len);

    Callback callback;

    m_recvEntry = static_cast<DmaRecvEntry *>( m_rm.nic().findRecv( m_src, m_hdr, matchHdr ) );
    if ( m_recvEntry ) {
        ev->bufPop( sizeof(MsgHdr) );
        ev->bufPop( sizeof(MatchMsgHdr) );

        callback = std::bind( &Nic::RecvMachine::state_move_0, &m_rm, ev, this );

    } else {
        callback = std::bind( &Nic::RecvMachine::state_2, &m_rm, ev );
    }
    m_rm.nic().schedCallback( callback, m_rm.m_rxMatchDelay );

    m_matched_len = matchHdr.len;
    m_matched_tag = matchHdr.tag;
}
