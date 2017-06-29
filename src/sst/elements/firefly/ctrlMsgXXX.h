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

#ifndef COMPONENTS_FIREFLY_CTRLMSGXXX_H
#define COMPONENTS_FIREFLY_CTRLMSGXXX_H

#include "ctrlMsg.h"
#include <stdlib.h>

namespace SST {
namespace Firefly {
namespace CtrlMsg {

class MemoryBase;
class ProcessQueuesState;

class XXX : SubComponent  {

    typedef std::function<void()> Callback;
    typedef std::function<void(nid_t, uint32_t, size_t)> Callback2;

  public:

    XXX( Component* owner, Params& params );
    ~XXX();

    void setup();
    void finish();
    
    void setVars( Info* info, VirtNic*, Thornhill::MemoryHeapLink*, Link* retLink );

    void initMsgPassing();
    void makeProgress();
    void sendv( std::vector<IoVec>&,
        MP::PayloadDataType dtype, MP::RankID src, uint32_t tag,
        MP::Communicator group, CommReq* );

    void recvv( std::vector<IoVec>&,
        MP::PayloadDataType dtype, MP::RankID src, uint32_t tag,
        MP::Communicator group, CommReq* );

    void waitAll( std::vector<CommReq*>& reqs );

    void send(const Hermes::MemAddr& buf, uint32_t count,
        MP::PayloadDataType dtype, MP::RankID dest, uint32_t tag,
        MP::Communicator group );

    void isend(const Hermes::MemAddr& buf, uint32_t count,
        MP::PayloadDataType dtype, MP::RankID dest, uint32_t tag,
        MP::Communicator group, MP::MessageRequest* req );

    void recv(const Hermes::MemAddr& buf, uint32_t count,
        MP::PayloadDataType dtype, MP::RankID src, uint32_t tag,
        MP::Communicator group, MP::MessageResponse* resp );

    void irecv(const Hermes::MemAddr& buf, uint32_t count,
        MP::PayloadDataType dtype, MP::RankID src, uint32_t tag,
        MP::Communicator group, MP::MessageRequest* req );

    void wait( MP::MessageRequest, MP::MessageResponse* resp );

    void waitAny( int count, MP::MessageRequest req[], int *index,
        MP::MessageResponse* resp );

    void waitAll( int count, MP::MessageRequest req[],
        MP::MessageResponse* resp[] );

  private:

    bool notifyGetDone( void* );
    bool notifySendPioDone( void* );
    bool notifyRecvDmaDone( int, int, size_t, void* );
    bool notifyNeedRecv( int, int, size_t );

    uint64_t sendStateDelay() { return m_sendStateDelay; }
    uint64_t recvStateDelay() { return m_recvStateDelay; }
    uint64_t waitallStateDelay() { return m_waitallStateDelay; }
    uint64_t waitanyStateDelay() { return m_waitanyStateDelay; }

    void memHeapAlloc( size_t bytes, std::function<void(uint64_t)> callback ) {
        m_memHeapLink->alloc( bytes, callback );
    }

    Output          m_dbg;
    int             m_dbg_level;
    int             m_dbg_mask;

    ProcessQueuesState*     m_processQueuesState;

    Thornhill::MemoryHeapLink* m_memHeapLink;

    Info*       m_info;
    MemoryBase* m_mem;

    uint64_t m_sendStateDelay;
    uint64_t m_recvStateDelay;
    uint64_t m_waitallStateDelay;
    uint64_t m_waitanyStateDelay;
};

}
}
}

#endif
