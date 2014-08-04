
// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef COMPONENTS_FIREFLY_CTRLMSGXXX_H
#define COMPONENTS_FIREFLY_CTRLMSGXXX_H

#include "ctrlMsg.h"
#include "ioVec.h"

namespace SST {
namespace Firefly {
namespace CtrlMsg {


template< class T >
class SendState;

template< class T >
class RecvState;

template< class T >
class WaitAnyState;

template< class T >
class ProcessQueuesState;

template< class T1, class T2 >
class FunctorBase_1;

class StateArgsBase;

typedef unsigned char key_t;

static const key_t LongAckKey  = 1 << (sizeof(key_t) * 8 - 2);
static const key_t LongRspKey  = 2 << (sizeof(key_t) * 8 - 2);
static const key_t ReadReqKey  = 3 << (sizeof(key_t) * 8 - 2);
static const key_t ReadRspKey  = 0 << (sizeof(key_t) * 8 - 2);

struct MatchHdr {
    uint32_t count; 
    uint32_t dtypeSize;
    Hermes::RankID rank;
    Hermes::Communicator group;
    uint64_t    tag;
    key_t       ackKey;      
};

struct CtrlHdr {
    key_t     rspKey;
    key_t     ackKey;      
};

static const int    ShortMsgQ       = 0xf00d;

class ShortRecvBuffer;

class XXX  {

  public:
    XXX( Component* owner, Params& params );
    ~XXX();
    void init( Info* info, VirtNic* );
    void setup();
    void setRetLink( Link* link );

    Info*      info() { return m_info; }
    VirtNic&   nic() { return *m_nic; }           

    void sendv( bool blocking, std::vector<IoVec>&,
        Hermes::PayloadDataType dtype, Hermes::RankID src, uint32_t tag,
        Hermes::Communicator group, CommReq*, FunctorBase_0<bool>* );

    void recvv( bool blocking, std::vector<IoVec>&,
        Hermes::PayloadDataType dtype, Hermes::RankID src, uint32_t tag,
        Hermes::Communicator group, CommReq*, FunctorBase_0<bool>* );

    void waitAny( std::vector<CommReq*>& reqs, FunctorBase_1<CommReq*,bool>* );

    void send(Hermes::Addr buf, uint32_t count,
        Hermes::PayloadDataType dtype, Hermes::RankID dest, uint32_t tag,
        Hermes::Communicator group, FunctorBase_0<bool>* func );

    void isend(Hermes::Addr buf, uint32_t count,
        Hermes::PayloadDataType dtype, Hermes::RankID dest, uint32_t tag,
        Hermes::Communicator group, Hermes::MessageRequest* req,
		FunctorBase_0<bool>* func );

    void recv(Hermes::Addr buf, uint32_t count,
        Hermes::PayloadDataType dtype, Hermes::RankID src, uint32_t tag,
        Hermes::Communicator group, Hermes::MessageResponse* resp,
		FunctorBase_0<bool>* func );

    void irecv(Hermes::Addr buf, uint32_t count,
        Hermes::PayloadDataType dtype, Hermes::RankID src, uint32_t tag,
        Hermes::Communicator group, Hermes::MessageRequest* req,
        FunctorBase_0<bool>* func );

    void wait( Hermes::MessageRequest, Hermes::MessageResponse* resp,
		FunctorBase_0<bool>* func );

    void waitAny( int count, Hermes::MessageRequest req[], int *index,
        Hermes::MessageResponse* resp, FunctorBase_0<bool>* func  );

    void waitAll( int count, Hermes::MessageRequest req[],
        Hermes::MessageResponse* resp[], FunctorBase_0<bool>* func );

    size_t shortMsgLength() { return m_shortMsgLength; }

    void passCtrlToFunction(int delay, FunctorBase_0<bool>* );
    void passCtrlToFunction(int delay, FunctorBase_1<CommReq*,bool>*, CommReq*);
    void schedFunctor( FunctorBase_0<bool>*, int delay = 0 );

    int memcpyDelay(int bytes ) {
        return (float) (bytes * m_memcpyDelay_ps)/ 1000.0; 
    } 

    int matchDelay( int i ) {
        return i * m_matchDelay_ns;
    }

    int regRegionDelay( int nbytes ) {
        // times are in ps.
        float tmp = m_regRegionBaseDelay_ps;   
        if ( nbytes > m_regRegionXoverLength  ) {
            tmp += nbytes * m_regRegionPerByteDelay_ps; 
        }
        // return ns.
        return tmp/1000.0;
    }

    int txDelay() {
        return m_txDelay;
    }

    int rxDelay() {
        return m_rxDelay;
    }

    int txNicDelay() {
        return m_txNicDelay;
    }

    int rxNicDelay() {
        return m_rxNicDelay;
    }


    int m_matchDelay_ns;
    int m_memcpyDelay_ps;
    int m_txDelay;
    int m_rxDelay;
    int m_txNicDelay;
    int m_rxNicDelay;
    int m_regRegionBaseDelay_ps;
    int m_regRegionPerByteDelay_ps;
    int m_regRegionXoverLength;

  private:
    class DelayEvent : public SST::Event {
      public:

        DelayEvent( FunctorBase_0<bool>* _functor) :
            Event(), 
            functor0( _functor ),
            functor1( NULL )
        {}

        DelayEvent( FunctorBase_1<CommReq*,bool>* _functor, CommReq* _req) :
            Event(), 
            functor0( NULL ),
            functor1( _functor ),
            req( _req )
        {}

        FunctorBase_0<bool>*    functor0;
        FunctorBase_1<CommReq*,bool>*    functor1;
        CommReq*                         req;
    };

  private:
    void delayHandler( Event* );
    void loopHandler( Event* );
    bool notifySendPioDone( void* );
    bool notifySendDmaDone( void* );
    bool notifyRecvDmaDone( int, int, size_t, void* );
    bool notifyNeedRecv( int, int, size_t );

    Output          m_dbg;
    Link*           m_retLink;
    Link*           m_delayLink;
    Link*           m_loopLink;
    Info*           m_info;
    VirtNic*        m_nic;

    SendState<XXX>*         m_sendState;
    RecvState<XXX>*         m_recvState;
    WaitAnyState<XXX>*      m_waitAnyState;
    Output::output_location_t   m_dbg_loc;
    int                         m_dbg_level;
    size_t                  m_shortMsgLength;
  public:
    ProcessQueuesState<XXX>*     m_processQueuesState;
    void loopSend( std::vector<IoVec>&, int, void* );
    void loopSend( int, void* );
};

class _CommReq : public Hermes::MessageRequestBase {
  public:

    enum Type { Recv, Send, Isend, Irecv };

    _CommReq( Type type, std::vector<IoVec>& _ioVec, 
        unsigned int dtypeSize, Hermes::RankID rank, uint32_t tag,
        Hermes::Communicator group ) : 
        m_type( type ),
        m_buf( NULL ),
        m_ioVec( _ioVec ),
        m_resp( NULL ),
        m_done( false ),
        m_destRank( Hermes::AnySrc ),
        m_ignore( 0 ),
        m_isMine( false )
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

    _CommReq( Type type, Hermes::Addr buf, uint32_t count,
        unsigned int dtypeSize, Hermes::RankID rank, uint32_t tag, 
        Hermes::Communicator group, Hermes::MessageResponse* resp = NULL ) :
        m_type( type ),
        m_buf( buf ),
        m_resp( resp ),
        m_done( false ),
        m_destRank( Hermes::AnySrc ),
        m_ignore( 0 ),
        m_isMine( true )
    { 
        m_hdr.count = count;
        m_hdr.dtypeSize = dtypeSize; 

        if ( m_type == Recv || m_type == Irecv ) {
            m_hdr.rank = rank;
            if ( Hermes::AnyTag == tag  ) {
                m_ignore = 0xffffffff;
            }
        } else {
            m_destRank = rank;
        }

        m_hdr.tag = tag;
        m_hdr.group = group;
        m_ioVec.resize( 1 );
        m_ioVec[0].ptr = buf;
        m_ioVec[0].len = dtypeSize * count;
    }
    ~_CommReq() {
    }

    bool isBlocking() {
        return m_type == Recv || m_type == Send;
    }

    uint64_t ignore() { return m_ignore; }
    void setSrcRank( Hermes::RankID rank ) {
        m_hdr.rank = rank;
    }

    MatchHdr& hdr() { return m_hdr; }
    
    std::vector<IoVec>& ioVec() { 
        assert( ! m_ioVec.empty() );
        return m_ioVec; 
    }

    bool isDone() { return m_done; }
    void setDone() { m_done = true; }

    void getResp( Hermes::MessageResponse* resp ) {
        *resp = m_matchInfo;
    }

    void setResp( uint32_t tag, Hermes::RankID src, uint32_t count )
    {
        m_matchInfo.tag = tag;
        m_matchInfo.src = src;
        m_matchInfo.count = count;

        if ( m_resp ) {
            *m_resp = m_matchInfo;
        }
    }

    Hermes::RankID getDestRank() { return m_destRank; }
    Hermes::Communicator getGroup() { return m_hdr.group; }
    
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

  private:

    MatchHdr            m_hdr; 
    Type                m_type;
    Hermes::Addr        m_buf;
    std::vector<IoVec>  m_ioVec;
    Hermes::MessageResponse* m_resp;
    bool                m_done;
    Hermes::RankID      m_destRank;
    Hermes::MessageResponse  m_matchInfo;
    uint64_t            m_ignore;
    bool                m_isMine; 
};

class WaitReq {
    struct X {
        X( _CommReq* _req, Hermes::MessageResponse* _resp = NULL ) : 
            pos(0), req(_req), resp(_resp) {}

        X( int _pos, _CommReq* _req, Hermes::MessageResponse* _resp = NULL ) : 
            pos(_pos), req(_req), resp(_resp) {}

        int pos;
        _CommReq* req;
        Hermes::MessageResponse* resp;
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

    WaitReq( Hermes::MessageRequest req, Hermes::MessageResponse* resp ) :
        indexPtr(NULL)
    {
        reqQ.push_back( X( static_cast<_CommReq*>(req), resp ) );
    }

    WaitReq( int count, Hermes::MessageRequest req[], int *index,
                                        Hermes::MessageResponse* resp ) :
        indexPtr(index)
    {
        for ( int i = 0; i < count; i++ ) {
            reqQ.push_back( X( i, static_cast<_CommReq*>(req[i]), resp ) );
        }
    }

    WaitReq( int count, Hermes::MessageRequest req[],
                                        Hermes::MessageResponse* resp[] ) : 
        indexPtr(NULL)
    {
        for ( int i = 0; i < count; i++ ) {
            reqQ.push_back( X( i, static_cast<_CommReq*>(req[i]), resp[i] ) );
        }
    }

    bool isDone() {
        std::deque<X>::iterator iter = reqQ.begin();

        while ( iter != reqQ.end() ) {
            if ( iter->req->isDone() ) {
                if ( iter->resp ) {
                    iter->req->getResp( iter->resp );
                }

                if ( iter->req->isMine() ) {
                    delete iter->req;
                }

                // a waitany will have an valid indexPtr
                if ( indexPtr ) { 
                    *indexPtr = iter->pos;
                    reqQ.clear();
                    iter = reqQ.end();
                } else {
                    iter = reqQ.erase( iter );
                }


            } else {
                ++iter;
            }
        } 
        return reqQ.empty();
    }

    std::deque< X > reqQ;
    int* indexPtr; 
};

}
}
}

#endif
