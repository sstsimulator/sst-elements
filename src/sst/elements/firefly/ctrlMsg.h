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

#ifndef COMPONENTS_FIREFLY_CTRLMSG_H
#define COMPONENTS_FIREFLY_CTRLMSG_H

#include <sst/core/elementinfo.h>
#include <sst/core/component.h>
#include "protocolAPI.h"
#include "ctrlMsgFunctors.h"
#include "ioVec.h"
#include "sst/elements/hermes/msgapi.h"

using namespace Hermes;

namespace SST {
namespace Firefly {
namespace CtrlMsg {

typedef int  nid_t;

static const uint64_t AllgatherTag  = 0x10000000;
static const uint64_t AlltoallvTag  = 0x20000000;
static const uint64_t CollectiveTag = 0x30000000;
static const uint64_t GathervTag    = 0x40000000;
static const uint64_t LongProtoTag  = 0x50000000;
static const uint64_t TagMask       = 0xf0000000;

class MemoryBase;
class ProcessQueuesState;
class _CommReq;

struct CommReq {
    _CommReq* req;
};

static const uint64_t  AnyTag = -1; 

class API : public ProtocolAPI {

    typedef std::function<void()> Callback;
    typedef std::function<void(nid_t, uint32_t, size_t)> Callback2;

  public:
   SST_ELI_REGISTER_SUBCOMPONENT(
        API,
        "firefly",
        "CtrlMsgProto",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "",
        ""
    )

    SST_ELI_DOCUMENT_PARAMS(
    )
    API( Component* owner, Params& );
    ~API();

    virtual void setup();
    virtual void finish();
    virtual std::string name() { return "CtrlMsgProtocol"; }

    virtual void setVars( Info* info, VirtNic*, Thornhill::MemoryHeapLink*, Link* );

    void initMsgPassing();
    void makeProgress();
    void send( const Hermes::MemAddr&, size_t len, nid_t dest, uint64_t tag ); 
    void send( const Hermes::MemAddr&, size_t len, MP::RankID dest, uint64_t tag, 
                            MP::Communicator grp );
    void isend( const Hermes::MemAddr&, size_t len, nid_t dest, uint64_t tag, CommReq* );
    void isend( const Hermes::MemAddr&, size_t len, nid_t dest, uint64_t tag,
							MP::Communicator, CommReq* );
    void sendv( std::vector<IoVec>&, nid_t dest, uint64_t tag );
    void recv( const Hermes::MemAddr&, size_t len, nid_t src, uint64_t tag );
    void recv( const Hermes::MemAddr&, size_t len, nid_t src, uint64_t tag, MP::Communicator grp );
    void irecv( const Hermes::MemAddr&, size_t len, nid_t src, uint64_t tag, CommReq* );
    void irecv( const Hermes::MemAddr&, size_t len, MP::RankID src, uint64_t tag, 
                MP::Communicator grp, CommReq* );
    void irecvv( std::vector<IoVec>&, nid_t src, uint64_t tag, CommReq* );
    void wait( CommReq* );
    void waitAll( std::vector<CommReq*>& );

	void send( const Hermes::MemAddr& buf, uint32_t count, 
		MP::PayloadDataType dtype, MP::RankID dest, uint32_t tag,
        MP::Communicator group );

	void isend( const Hermes::MemAddr& buf, uint32_t count,
        MP::PayloadDataType dtype, MP::RankID dest, uint32_t tag,
        MP::Communicator group, MP::MessageRequest* req );

    void recv( const Hermes::MemAddr& buf, uint32_t count,
        MP::PayloadDataType dtype, MP::RankID src, uint32_t tag,
        MP::Communicator group, MP::MessageResponse* resp );

    void irecv( const Hermes::MemAddr& _buf, uint32_t _count,
        MP::PayloadDataType dtype, MP::RankID src, uint32_t tag,
        MP::Communicator group, MP::MessageRequest* req );

	void wait( MP::MessageRequest, MP::MessageResponse* resp );
   	void waitAny( int count, MP::MessageRequest req[], int *index,
              	MP::MessageResponse* resp );
    void waitAll( int count, MP::MessageRequest req[],
                MP::MessageResponse* resp[] );

  private:
    void sendv_common( std::vector<IoVec>& ioVec,
            MP::PayloadDataType dtype, MP::RankID dest, uint32_t tag,
            MP::Communicator group, CommReq* commReq );
    void recvv_common( std::vector<IoVec>& ioVec,
    MP::PayloadDataType dtype, MP::RankID src, uint32_t tag,
    MP::Communicator group, CommReq* commReq );

    bool notifyGetDone( void* );
    bool notifySendPioDone( void* );
    bool notifyRecvDmaDone( int, int, size_t, void* );
    bool notifyNeedRecv( int, size_t );

    uint64_t sendStateDelay() { return m_sendStateDelay; }
    uint64_t recvStateDelay() { return m_recvStateDelay; }
    uint64_t waitallStateDelay() { return m_waitallStateDelay; }
    uint64_t waitanyStateDelay() { return m_waitanyStateDelay; }

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
