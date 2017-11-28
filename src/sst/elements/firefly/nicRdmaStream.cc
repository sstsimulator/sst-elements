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

Nic::RecvMachine::RdmaStream::RdmaStream( Output& output, FireflyNetworkEvent* ev,
       RecvMachine& rm  ) : 
    StreamBase( output, rm )
{
    m_hdr = *(MsgHdr*) ev->bufPtr();
    RdmaMsgHdr& rdmaHdr = *(RdmaMsgHdr*) ev->bufPtr( sizeof(MsgHdr) );

    m_dbg.verbose(CALL_INFO,1,NIC_DBG_RECV_MACHINE,"RDMA Operation\n");

    Callback callback;
    switch ( rdmaHdr.op  ) {

      case RdmaMsgHdr::Put:
      case RdmaMsgHdr::GetResp:
        {
          m_dbg.verbose(CALL_INFO,2,NIC_DBG_RECV_MACHINE,"%s Op\n", rdmaHdr.op == RdmaMsgHdr::Put ? "Put":"GetResp");

          m_recvEntry = m_rm.nic().findPut( ev->src, m_hdr, rdmaHdr );
          ev->bufPop(sizeof(MsgHdr) + sizeof(rdmaHdr) );
          callback = std::bind( &Nic::RecvMachine::state_move_0, &m_rm, ev, this );
        }
        break;

      default:
        assert(0);
    }
    m_matched_len = m_recvEntry->totalBytes();

    m_rm.nic().schedCallback( callback, 0 );
}
