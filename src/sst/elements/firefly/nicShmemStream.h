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

class ShmemStream : public StreamBase {
  public:

    ShmemStream( Output&, Ctx*, int srcNode, int srcPid, int destPid, FireflyNetworkEvent* );

    bool isBlocked() {
        return  StreamBase::isBlocked() || m_blocked;
    }
  protected:
    void processFirstPkt( FireflyNetworkEvent* ev ) {
        m_dbg.debug(CALL_INFO,1,NIC_DBG_RECV_STREAM,"\n");
        processPktHdr(ev);
        m_blocked = false;
    }
    void processPktHdr( FireflyNetworkEvent* ev ) {
        m_dbg.debug(CALL_INFO,1,NIC_DBG_RECV_STREAM,"\n");
        processOp(ev);
    }
  private:
    void processOp( FireflyNetworkEvent* ev );
    void processAck( ShmemMsgHdr&, FireflyNetworkEvent*, int, int );
    void processPut( ShmemMsgHdr&, FireflyNetworkEvent*, int, int );
    void processGetResp( ShmemMsgHdr&, FireflyNetworkEvent*, int, int );
    void processGet( ShmemMsgHdr&, FireflyNetworkEvent*, int, int );
    void processFadd( ShmemMsgHdr&, FireflyNetworkEvent*, int, int );
    void processAdd( ShmemMsgHdr&, FireflyNetworkEvent*, int, int );
    void processCswap( ShmemMsgHdr&, FireflyNetworkEvent*, int, int );
    void processSwap( ShmemMsgHdr&, FireflyNetworkEvent*, int, int );
    ShmemMsgHdr m_shmemHdr;
    bool m_blocked;
};
