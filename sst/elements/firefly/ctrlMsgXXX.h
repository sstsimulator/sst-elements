
// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2013, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef COMPONENTS_FIREFLY_CTRLMSGXXX_H
#define COMPONENTS_FIREFLY_CTRLMSGXXX_H

#include "ctrlMsg.h"
#include "nic.h"
#include "ioVec.h"

namespace SST {
namespace Firefly {
namespace CtrlMsg {

static const int    ShortMsgQ       = 0xf00d;
static const size_t ShortMsgLength  = 1024*16;

template< class T >
class SendState;

template< class T >
class RecvState;

template< class T >
class ReadState;

template< class T >
class WaitAnyState;

template< class T >
class ProcessQueuesState;

template< class T1, class T2 >
class FunctorBase_1;

class StateArgsBase;

class VirtNic; 

typedef unsigned char key_t;

static const key_t LongAckKey  = 1 << (sizeof(key_t) * 8 - 2);
static const key_t LongRspKey  = 2 << (sizeof(key_t) * 8 - 2);
static const key_t ReadReqKey  = 3 << (sizeof(key_t) * 8 - 2);
static const key_t ReadRspKey  = 0 << (sizeof(key_t) * 8 - 2);

struct MsgHdr {
    tag_t     tag;
    nid_t     nid;
    size_t    length;
    key_t     ackKey;      
    key_t     rspKey;
};

class ShortRecvBuffer;

class XXX  {

  public:
    XXX( Component* owner, Params& params );
    void init( Info* info, Nic::VirtNic* );
    void setup();
    void setRetLink( Link* link );

    void sendv( bool blocking,  std::vector<IoVec>&, nid_t dest,
                        tag_t tag, CommReq*, FunctorBase_0<bool>* );
    void recvv( bool blocking, std::vector<IoVec>&, nid_t src,
                        tag_t tag, CommReq*, FunctorBase_0<bool>* );
    void waitAny( std::vector<CommReq*>& reqs, FunctorBase_1<CommReq*,bool>* );

    void read( nid_t, region_t, void* buf, size_t len,
                                    FunctorBase_0<bool>* = NULL );
    void registerRegion( region_t, nid_t, void* buf, size_t len, RegionEventQ* );
    void unregisterRegion( region_t );
    RegionEventQ* createEventQueue();

    Info*           info() { return m_info; }
    Nic::VirtNic&   nic() { return *m_nic; }           

    void setReturnState( FunctorBase_0<bool>* state ) {
        m_returnState = state; 
    }

    void passCtrlToFunction(int delay, FunctorBase_0<bool>* );
    void passCtrlToFunction(int delay, FunctorBase_1<CommReq*,bool>*, CommReq* );
    void schedFunctor( FunctorBase_0<bool>*, int delay = 0 );
    int memcpyDelay(int bytes ) {
        return bytes * m_memcpyDelay; 
    } 

    int matchDelay( int i ) {
        return i * m_matchDelay;
    }

    int txDelay() {
        return m_txDelay;
    }

    int rxDelay() {
        return m_rxDelay;
    }
    int m_matchDelay;
    int m_memcpyDelay;
    int m_txDelay;
    int m_rxDelay;

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
    bool notifySendPioDone( void* );
    bool notifySendDmaDone( void* );
    bool notifyRecvDmaDone( int, int, size_t, void* );
    bool notifyNeedRecv( int, int, size_t );

    bool checkMsgHdr( MsgHdr&, MsgHdr& );

    Output          m_dbg;
    Link*           m_retLink;
    Link*           m_delayLink;
    Info*           m_info;
    Nic::VirtNic*   m_nic;

    int             m_matchTime;
    int             m_copyTime;

    FunctorBase_0<bool>*    m_returnState;
    SendState<XXX>*         m_sendState;
    RecvState<XXX>*         m_recvState;
    ReadState<XXX>*         m_readState;
    WaitAnyState<XXX>*      m_waitAnyState;
    Output::output_location_t   m_dbg_loc;
    int                         m_dbg_level;
  public:
    ProcessQueuesState<XXX>*     m_processQueuesState;
};

class ShortRecvBuffer {
  public:
    ShortRecvBuffer() { 
        ioVec.resize(2);    

        ioVec[0].ptr = &hdr;
        ioVec[0].len = sizeof(hdr);

        buf.resize( ShortMsgLength );
        ioVec[1].ptr = &buf[0];
            ioVec[1].len = buf.size();
        }
    MsgHdr              hdr;
    std::vector<IoVec>  ioVec; 
    std::vector<unsigned char> buf;
};

class _CommReq {
  public:

    enum Type { Recv, Send };

    _CommReq( Type type, std::vector<IoVec> _ioVec, nid_t nid,
              tag_t tag, Status* status = NULL) : 
        m_type( type ),
        m_ioVec( _ioVec ),
        m_status( status ),
        m_done( false ),
        m_nid ( nid )
    {
        m_hdr.tag  = tag;
        m_hdr.length = getLength();
    }
    void initHdrNid( nid_t nid ) {
        m_hdr.nid  = nid;
    }

    MsgHdr& hdr() { return m_hdr; }
    
    std::vector<IoVec>& ioVec() {
        return m_ioVec;
    }

    bool isDone() { return m_done; }
    bool isSend() { return m_type == Send; }

    void setStatus( nid_t nid, tag_t tag, size_t length ) {
        if ( m_status ) {
            m_status->nid = nid;
            m_status->tag = tag;
            m_status->length = length;
        }
    }

    void setDone() { m_done = true; }
    nid_t nid() { return m_nid; }

  private:
    size_t  getLength() {
        size_t length = 0;
        for ( size_t i = 0; i < m_ioVec.size(); i++ ) {
            length += m_ioVec[i].len;
        }
        return length;
    }

    MsgHdr              m_hdr; 
    Type                m_type;
    std::vector<IoVec>  m_ioVec;
    Status*             m_status;
    bool                m_done;
    nid_t               m_nid;
};

}
}
}

#endif
