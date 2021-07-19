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

class MsgStream : public StreamBase {
  public:
    MsgStream( Output&, Ctx*, int srcNode, int srcPid, int destPid, FireflyNetworkEvent* );
    ~MsgStream() {
        m_dbg.debug(CALL_INFO,1,NIC_DBG_RECV_STREAM,"\n");
    }
    bool isBlocked() {
        return m_hdrPkt || m_blockedFirstPktDelay || StreamBase::isBlocked();
    }
  protected:
    void processFirstPkt( FireflyNetworkEvent* ev ) {
        m_blockedFirstPktDelay = false;
        processPktBody(ev);
    }
  private:
    bool m_blockedFirstPktDelay;
};
