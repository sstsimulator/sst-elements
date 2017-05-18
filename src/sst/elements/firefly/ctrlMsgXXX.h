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
#include "ioVec.h"
#include "latencyMod.h"
#include "mem.h"

namespace SST {
namespace Firefly {
namespace CtrlMsg {

template< class T >
class ProcessQueuesState;

typedef uint64_t MemAddr;
typedef unsigned short key_t;

static const key_t LongGetKey  = 1 << (sizeof(key_t) * 8 - 2);
static const key_t LongAckKey = LongGetKey;
static const key_t LongRspKey  = 2 << (sizeof(key_t) * 8 - 2);
static const key_t ReadReqKey  = 3 << (sizeof(key_t) * 8 - 2);
static const key_t ReadRspKey  = 0 << (sizeof(key_t) * 8 - 2);

struct MatchHdr {
    uint32_t count; 
    uint32_t dtypeSize;
    MP::RankID rank;
    MP::Communicator group;
    uint64_t    tag;
    key_t       key;      
};

struct CtrlHdr {
    key_t     key;
};

static const int    ShortMsgQ       = 0xf00d;

class ShortRecvBuffer;

class XXX : SubComponent  {

  public:

    typedef std::function<void()> Callback;
    typedef std::function<void(nid_t, uint32_t, size_t)> Callback2;

    XXX( Component* owner, Params& params );
    ~XXX();
    void init( Info* info, VirtNic*, Thornhill::MemoryHeapLink* );
    void setRetLink( Link* link );

    void setup();
    void finish();

    Info*      info() { return m_info; }
    VirtNic&   nic() { return *m_nic; }           
    Thornhill::MemoryHeapLink& memHeap() { return *m_memHeapLink; }

    void init();
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

    size_t shortMsgLength() { return m_shortMsgLength; }

    void schedCallback( Callback, uint64_t delay = 0 );
    void passCtrlToFunction( uint64_t delay = 0 );

    int rxPostDelay_ns( int bytes ) {
        if ( m_rxPostMod ) {
            return m_rxPostMod->getLatency( bytes );
        } else {
            return 0;
        }
    } 

    int regRegionDelay( int nbytes ) {
        double tmp = 0;

        if ( nbytes ) {
            tmp = m_regRegionBaseDelay_ns;
        }

        if ( nbytes > m_regRegionXoverLength  ) {
            tmp += (nbytes/4096) * m_regRegionPerPageDelay_ns; 
        }
        return tmp;
    }

    int txDelay( int bytes ) {
        return m_txSetupMod->getLatency( bytes );
    }

    int rxDelay( int bytes) {
        return m_rxSetupMod->getLatency( bytes );
    }

    int sendReqFiniDelay( int bytes ) {
        return m_txFiniMod->getLatency( bytes );
    }

    int recvReqFiniDelay( int bytes ) {
        return m_rxFiniMod->getLatency( bytes );
    }

    int sendAckDelay() {
        return m_sendAckDelay;
    }

    uint64_t sendStateDelay() {
        return m_sendStateDelay;
    }

    uint64_t recvStateDelay() {
        return m_recvStateDelay;
    }

    uint64_t waitallStateDelay() {
        return m_waitallStateDelay;
    }
    uint64_t waitanyStateDelay() {
        return m_waitanyStateDelay;
    }

    Statistic<uint64_t>* statRcvdMsg() { return m_statRcvdMsg; }
    Statistic<uint64_t>* statPstdRcv() { return m_statPstdRcv; }

    void memcpy( Callback, MemAddr to, MemAddr from, size_t );
    void memwrite( Callback, MemAddr to, size_t );
    void memread( Callback, MemAddr to, size_t );
    void mempin( Callback, uint64_t Memaddr, size_t );
    void memunpin( Callback, uint64_t Memaddr, size_t );
    void memwalk( Callback, int count );

  private:
    class DelayEvent : public SST::Event {
      public:

        DelayEvent( Callback _callback ) :
            Event(), 
            callback( _callback )
        {}

        Callback                callback;
        
        NotSerializable(DelayEvent)
    };

  private:
    void delayHandler( Event* );
    void loopHandler( Event* );
    void memEventHandler( Event* );
    bool notifyGetDone( void* );
    bool notifySendPioDone( void* );
    bool notifyRecvDmaDone( int, int, size_t, void* );
    bool notifyNeedRecv( int, int, size_t );

    int txMemcpyDelay( int bytes ) {
        return m_txMemcpyMod->getLatency( bytes );
    }

    int rxMemcpyDelay( int bytes ) {
        return m_rxMemcpyMod->getLatency( bytes );
    } 

    int matchDelay( int i ) {
        return i * m_matchDelay_ns;
    }

    Output          m_dbg;
    Link*           m_retLink;
    Link*           m_delayLink;
    Link*           m_loopLink;
    Thornhill::MemoryHeapLink* m_memHeapLink;
    Info*           m_info;
    VirtNic*        m_nic;

    Statistic<uint64_t>* m_statRcvdMsg;
    Statistic<uint64_t>* m_statPstdRcv;

    int                         m_dbg_level;
    int                         m_dbg_mask;
    size_t                  m_shortMsgLength;

    uint64_t m_sendStateDelay;
    uint64_t m_recvStateDelay;
    uint64_t m_waitallStateDelay;
    uint64_t m_waitanyStateDelay;

    int m_matchDelay_ns;
    int m_regRegionBaseDelay_ns;
    int m_regRegionPerPageDelay_ns;
    int m_regRegionXoverLength;
    int m_sendAckDelay;

    LatencyMod* m_txMemcpyMod;
    LatencyMod* m_rxMemcpyMod;
    LatencyMod* m_txSetupMod;
    LatencyMod* m_rxSetupMod;
    LatencyMod* m_rxPostMod;
    LatencyMod* m_txFiniMod;
    LatencyMod* m_rxFiniMod;

  public:
    ProcessQueuesState<XXX>*     m_processQueuesState;
    void loopSend( std::vector<IoVec>&, int, void* );
    void loopSend( int, void* );
};

class _CommReq : public MP::MessageRequestBase {
  public:

    enum Type { Recv, Send, Isend, Irecv };

    _CommReq( Type type, std::vector<IoVec>& _ioVec, 
        unsigned int dtypeSize, MP::RankID rank, uint32_t tag,
        MP::Communicator group ) : 
        m_type( type ),
        m_ioVec( _ioVec ),
        m_resp( NULL ),
        m_done( false ),
        m_destRank( MP::AnySrc ),
        m_ignore( 0 ),
        m_isMine( false ),
        m_finiDelay_ns( 0 )
    {
        m_hdr.count = getLength() / dtypeSize;
        m_hdr.dtypeSize = dtypeSize; 

        if ( m_type == Recv || m_type == Irecv ) {
            m_hdr.rank = rank;
        } else {
            m_destRank = rank;
        }

        m_hdr.tag = tag;
        m_hdr.group = group;
    }

    _CommReq( Type type, const Hermes::MemAddr& buf, uint32_t count,
        unsigned int dtypeSize, MP::RankID rank, uint32_t tag, 
        MP::Communicator group, MP::MessageResponse* resp = NULL ) :
        m_type( type ),
        m_resp( resp ),
        m_done( false ),
        m_destRank( MP::AnySrc ),
        m_ignore( 0 ),
        m_isMine( true ),
        m_finiDelay_ns( 0 )
    { 
        m_hdr.count = count;
        m_hdr.dtypeSize = dtypeSize; 

        if ( m_type == Recv || m_type == Irecv ) {
            m_hdr.rank = rank;
            if ( MP::AnyTag == tag  ) {
                m_ignore = 0xffffffff;
            }
        } else {
            m_destRank = rank;
        }

        m_hdr.tag = tag;
        m_hdr.group = group;
        m_ioVec.resize( 1 );
        m_ioVec[0].addr = buf;
        m_ioVec[0].len = dtypeSize * count;
    }
    ~_CommReq() {
    }

    bool isBlocking() {
        return m_type == Recv || m_type == Send;
    }

    uint64_t ignore() { return m_ignore; }
    void setSrcRank( MP::RankID rank ) {
        m_hdr.rank = rank;
    }

    MatchHdr& hdr() { return m_hdr; }
    
    std::vector<IoVec>& ioVec() { 
        assert( ! m_ioVec.empty() );
        return m_ioVec; 
    }

    bool isDone() { return m_done; }
    void setDone(int delay = 0 ) { 
        m_finiDelay_ns = delay;
        m_done = true; 
    }

    MP::MessageResponse* getResp(  ) {
        return &m_matchInfo;
    }

    void setResp( uint32_t tag, MP::RankID src, uint32_t count )
    {
        m_matchInfo.tag = tag;
        m_matchInfo.src = src;
        m_matchInfo.count = count;

        if ( m_resp ) {
            *m_resp = m_matchInfo;
        }
    }

    MP::RankID getDestRank() { return m_destRank; }
    MP::Communicator getGroup() { return m_hdr.group; }
    
    size_t getLength( ) {
        size_t length = 0;
        for ( size_t i = 0; i < m_ioVec.size(); i++ ) {
            length += m_ioVec[i].len;
        }
        return length;
    }

    bool isMine( ) {
        return m_isMine;
    }
    int  getFiniDelay() { return m_finiDelay_ns; }

    bool isSend() {
        if ( m_type == Isend  || m_type == Send ) {
            return true;
        } else {
            return false;
        }
    }

    // need to save info for the long protocol ack
    int m_ackKey;
    int m_ackNid;

  private:

    MatchHdr            m_hdr; 
    Type                m_type;
    std::vector<IoVec>  m_ioVec;
    MP::MessageResponse* m_resp;
    bool                m_done;
    MP::RankID      m_destRank;
    MP::MessageResponse  m_matchInfo;
    uint64_t            m_ignore;
    bool                m_isMine; 
    int                 m_finiDelay_ns;
};

class WaitReq {
    struct X {
        X( _CommReq* _req, MP::MessageResponse* _resp = NULL ) : 
            pos(0), req(_req), resp(_resp) {}

        X( int _pos, _CommReq* _req, MP::MessageResponse* _resp = NULL ) : 
            pos(_pos), req(_req), resp(_resp) {}

        int pos;
        _CommReq* req;
        MP::MessageResponse* resp;
    };

  public:
    WaitReq( _CommReq* req ) : indexPtr(NULL) {
        reqQ.push_back( X( req ) ); 
    }

    WaitReq( std::vector<_CommReq*> reqs ) : indexPtr(NULL) {
        for ( unsigned int i = 0; i < reqs.size(); i++ ) {
            reqQ.push_back( X( i, reqs[i] ) ); 
        } 
    }

    WaitReq( MP::MessageRequest req, MP::MessageResponse* resp ) :
        indexPtr(NULL)
    {
        reqQ.push_back( X( static_cast<_CommReq*>(req), resp ) );
    }

    WaitReq( int count, MP::MessageRequest req[], int *index,
                                        MP::MessageResponse* resp ) :
        indexPtr(index)
    {
        for ( int i = 0; i < count; i++ ) {
            reqQ.push_back( X( i, static_cast<_CommReq*>(req[i]), resp ) );
        }
    }

    WaitReq( int count, MP::MessageRequest req[],
                                        MP::MessageResponse* resp[] ) : 
        indexPtr(NULL)
    {
        MP::MessageResponse* tmp = (MP::MessageResponse*)resp;
        for ( int i = 0; i < count; i++ ) {
			if ( resp ) {
                reqQ.push_back( X( i, static_cast<_CommReq*>(req[i]), &tmp[i] ) );
			} else {
                reqQ.push_back( X( i, static_cast<_CommReq*>(req[i]) ) );
			}
        }
    }

    bool isDone() {
        return reqQ.empty();
    }

    _CommReq* getFiniReq() {
        std::deque<X>::iterator iter = reqQ.begin();

        while ( iter != reqQ.end() ) {
            if ( iter->req->isDone() ) {

                _CommReq* req = iter->req;

                if ( iter->resp ) {
                    *iter->resp = *iter->req->getResp(  );
                }

                // a waitany will have an valid indexPtr
                if ( indexPtr ) { 
                    *indexPtr = iter->pos;
                    reqQ.clear();
                } else {
                    iter = reqQ.erase( iter );
                }
                return req;

            } else {
                ++iter;
            }
        } 
        return NULL;
    }

  private:
    std::deque< X > reqQ;
    int* indexPtr; 
};

}
}
}

#endif
