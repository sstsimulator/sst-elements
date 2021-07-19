// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
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

Nic::RecvMachine::RdmaStream::RdmaStream( Output& output, Ctx* ctx, int srcNode,
        int srcPid, int destPid, FireflyNetworkEvent* ev ) :
    StreamBase( output, ctx, srcNode, srcPid, destPid ), m_blocked(true)
{
    m_unit = m_ctx->allocRecvUnit();
    processPktHdr(ev);
    m_blocked = false;
}
void Nic::RecvMachine::RdmaStream::processPktHdr( FireflyNetworkEvent* ev ) {

    MsgHdr& hdr         = *(MsgHdr*) ev->bufPtr();
    RdmaMsgHdr& rdmaHdr = *(RdmaMsgHdr*) ev->bufPtr( sizeof(MsgHdr) );
    Callback callback;
    int delay = 0;
    switch ( rdmaHdr.op  ) {

      case RdmaMsgHdr::Put:
      case RdmaMsgHdr::GetResp:
        {
          m_dbg.debug(CALL_INFO,2,NIC_DBG_RECV_STREAM,"%s Op\n", rdmaHdr.op == RdmaMsgHdr::Put ? "Put":"GetResp");

          m_recvEntry = m_ctx->findPut( m_srcNode, hdr, rdmaHdr );
          ev->bufPop(sizeof(MsgHdr) + sizeof(rdmaHdr) );
          ev->clearHdr();
          m_matched_len = m_recvEntry->totalBytes();

          callback  = std::bind( &Nic::RecvMachine::StreamBase::processPktBody, this, ev );
        }
        break;
      case RdmaMsgHdr::Get:
        {
          m_dbg.debug(CALL_INFO,1,NIC_DBG_RECV_STREAM,"CtlMsg Get Operation srcNode=%d op=%d rgn=%d resp=%d, offset=%d\n",
                ev->getSrcNode(), rdmaHdr.op, rdmaHdr.rgnNum, rdmaHdr.respKey, rdmaHdr.offset );

          SendEntryBase* entry = m_ctx->findGet( ev->getSrcNode(), ev->getSrcPid(), rdmaHdr );
          assert(entry);

          ev->bufPop(sizeof(MsgHdr) + sizeof(rdmaHdr) );

          callback = std::bind( &Nic::RecvMachine::StreamBase::qSend, this, entry );
          delay = m_ctx->getHostReadDelay();

          delete ev;
        }
        break;

      default:
        assert(0);
    }
    m_ctx->nic().schedCallback( callback, delay );
}
