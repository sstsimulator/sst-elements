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

Nic::RecvMachine::RdmaStream::RdmaStream( Output& output, Ctx* ctx, int unit, int srcNode,
        int srcPid, int destPid, FireflyNetworkEvent* ev ) :
    StreamBase( output, ctx, unit, srcNode, srcPid, destPid )
{
    MsgHdr& hdr         = *(MsgHdr*) ev->bufPtr();
    RdmaMsgHdr& rdmaHdr = *(RdmaMsgHdr*) ev->bufPtr( sizeof(MsgHdr) );

    m_dbg.debug(CALL_INFO,1,NIC_DBG_RECV_MACHINE,"RDMA Operation\n");

    switch ( rdmaHdr.op  ) {

      case RdmaMsgHdr::Put:
      case RdmaMsgHdr::GetResp:
        {
          m_dbg.debug(CALL_INFO,2,NIC_DBG_RECV_MACHINE,"%s Op\n", rdmaHdr.op == RdmaMsgHdr::Put ? "Put":"GetResp");

          m_recvEntry = m_ctx->findPut( m_srcNode, hdr, rdmaHdr );
          ev->bufPop(sizeof(MsgHdr) + sizeof(rdmaHdr) );
          m_startCallback  = std::bind( &Nic::RecvMachine::StreamBase::processPkt, this, ev );
          m_matched_len = m_recvEntry->totalBytes();
        }
        break;
      case RdmaMsgHdr::Get:
        {
          m_dbg.debug(CALL_INFO,1,NIC_DBG_RECV_MACHINE,"CtlMsg Get Operation srcNode=%d op=%d rgn=%d resp=%d, offset=%d\n",
                ev->getSrcNode(), rdmaHdr.op, rdmaHdr.rgnNum, rdmaHdr.respKey, rdmaHdr.offset );

          SendEntryBase* entry = m_ctx->findGet( ev->getSrcNode(), ev->getSrcPid(), rdmaHdr );
          assert(entry);

          ev->bufPop(sizeof(MsgHdr) + sizeof(rdmaHdr) );

          m_startCallback = std::bind( &Nic::RecvMachine::StreamBase::qSend, this, entry );
          m_startDelay = m_ctx->getHostReadDelay();

        delete ev;
        }
        break;

      default:
        assert(0);
    }
}
