
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

#ifndef COMPONENTS_FIREFLY_CTRLMSG_H
#define COMPONENTS_FIREFLY_CTRLMSG_H

#include <sst/core/output.h>
#include "protocolAPI.h"

namespace SST {

class Params;

namespace Firefly {

class Info;

// Callback Functor
class CBF {
  public:
    typedef VoidArg_FunctorBase< CBF* > Functor;
    CBF() : callback(NULL) {}
    virtual ~CBF() { if ( callback ) delete callback; }

    Functor* callback;
};

class CtrlMsg : public ProtocolAPI {

  public:

    typedef uint32_t tag_t;
    typedef uint32_t rank_t;
    typedef uint32_t group_t;

    static const rank_t  AnyRank = -1;
    static const tag_t   AnyTag = -1; 

    struct Response {
        tag_t   tag;
        rank_t  rank; 
        size_t  length;
    };

  private:

    typedef StaticArg_Functor<CtrlMsg, IO::Entry*, IO::Entry*>  IO_Functor;

    struct MsgHdr {
        size_t    len;
        tag_t     tag;
        rank_t    rank;
        group_t   group;
    };

    struct BaseInfo {
        BaseInfo( std::vector<IoVec> _ioVec, rank_t rank,
            tag_t tag, group_t group, Response* resp ) : 
            ioVec( _ioVec ),
            response( resp )
        {
            hdr.rank  = rank,
            hdr.tag   = tag;
            hdr.group = group;
            hdr.len   = 0;
            for ( unsigned int i = 0; i < ioVec.size(); i++ ) {
                hdr.len += ioVec[i].len;
            }
        }

        virtual ~BaseInfo() {} 

        std::vector<IoVec>  ioVec;
        MsgHdr              hdr; 
        Response*           response;
    };

    typedef struct BaseInfo RecvInfo;
    typedef struct BaseInfo SendInfo;

  public:

    class CommReq {
      public:
        CommReq() : m_info( NULL ) {}
        virtual ~CommReq() {
            if ( m_info ) delete m_info;
        }
        void initRecv( bool blocked, std::vector<IoVec>& ioVec, int src,
                       int tag, int group, Response* resp ) 
        {
            assert( ! m_info );
            m_info = new RecvInfo( ioVec, src, tag, group, resp );
            m_type = CommReq::Recv;
        }
        void initSend( bool blocked, std::vector<IoVec>& ioVec, int src,
                                       int tag, int group ) 
        {
            assert( ! m_info );
            m_info = new SendInfo( ioVec, src, tag, group, NULL );
            m_type = CommReq::Send;
        }
        void setDone() {
            assert( m_info );
            delete m_info;
            m_info = NULL;
        }

        bool isSend() { return m_type == Send; }
        bool isDone() { return m_info == NULL; }
        BaseInfo* info() { assert( m_info); return m_info; }
        void setResponse( rank_t rank, tag_t tag, size_t length ) {
            if ( m_info->response ) {
                m_info->response->rank = rank;
                m_info->response->tag = tag;
                m_info->response->length = length;
            }
        }
        void setDoneFunc( CBF::Functor* doneFunc ) { m_doneFunc = doneFunc; }
        CBF::Functor* doneFunc() { return m_doneFunc; }

      private:
        enum { Send, Recv } m_type;
        BaseInfo*           m_info;
        CBF::Functor*       m_doneFunc;
    };

  private:

    class IORequest : public IO::Entry {
      public:
        enum { Hdr, Body }  state;
        IORequest() : state( Hdr ) {}
        IO::NodeId          node;
        CommReq*            commReq;
        MsgHdr              hdr;
        std::vector<IoVec>  ioVec; 
        std::vector<unsigned char> buf;
    };

    class DelayEvent : public SST::Event {
      public:

        DelayEvent( IORequest* req ) : 
            Event(), 
            ioRequest(req),
            commReq(NULL),
            type( FromOther ) 
        {}

        DelayEvent( CommReq* req, IORequest* ioReq, bool _blocked ) : 
            Event(), 
            ioRequest(ioReq),
            commReq(req),
            type( FromFunc ),
            blocked( _blocked )
        {}
    
        DelayEvent( CBF::Functor* _functor ) :
            Event(), 
            functor( _functor ),
            type( Functor )
        { }

        IORequest* ioRequest;
        CommReq*   commReq;
        CBF::Functor* functor;
        enum { FromFunc, FromOther, Functor } type;
        bool    blocked;
    };

  public:

    CtrlMsg( Component* owner, Params& );

    virtual void init( OutBase* out, Info* info, Link* link );

    // this group of function is used by hades 
    virtual void setup();
    virtual std::string name() { return "CtrlMsg"; }

    virtual IO::NodeId canSend();
    virtual void startSendStream( IO::NodeId );
    virtual void startRecvStream( IO::NodeId );
    virtual void setRetLink( Link* link ) { m_retLink = link;}

    // this group of functions is used by the function state machine
    void send( void* buf, size_t len, int dest, int tag, int group,
                                        CBF::Functor* = NULL );

    void sendv( std::vector<IoVec>&, int dest, int tag, int group,
                                        CBF::Functor* = NULL );

    void sendv( std::vector<IoVec>&, int dest, int tag,
                                        CBF::Functor* = NULL );

    void isendv( std::vector<IoVec>&, int dest, int tag, int group, CommReq*,
                                        CBF::Functor* = NULL );
    void isendv( std::vector<IoVec>&, int dest, int tag, CommReq*,
                                        CBF::Functor* = NULL );

    void recv( void* buf, size_t len, int src, int tag, int group,
                                        CBF::Functor* = NULL );
    void recvv( std::vector<IoVec>&, int src, int tag, Response*,
                                        CBF::Functor* = NULL );
    void irecv( void* buf, size_t len, int src, int tag, int group, CommReq*,
                                        CBF::Functor* = NULL );
    void irecvv( std::vector<IoVec>&, int src,  int tag, int group, CommReq*,
                                        CBF::Functor* = NULL );
    void irecvv( std::vector<IoVec>&, int src,  int tag, CommReq*, 
                 CBF::Functor*, Response*, CBF::Functor* = NULL );
    void wait( CommReq*, CBF::Functor* = NULL );
    void waitAny( std::set<CommReq*>&, CBF::Functor* = NULL );

  protected:

    Info* info() { return m_info; } 

  private:

    void delayHandler( Event* );
    IO::Entry* sendIODone( IO::Entry* );
    IO::Entry* recvIODone( IO::Entry* );
    void recvBody( IORequest* );
    void endSendStream( IORequest* );
    void endRecvStream( IORequest* );
    void endStream( IORequest* );
    IORequest* processUnexpectedMatch( CommReq* r, int& delay ); 
    IORequest* findUnexpectedRecv( RecvInfo& info, int& delay );
    CommReq* findPostedRecv( MsgHdr&, int& delay );
    void removePostedRecv( CommReq* );

    int getCopyDelay( int nbytes ) { return m_copyTime * nbytes; }

    void _recv( bool blocking, void* buf, size_t len, int src,
                            int tag, int group, CommReq*, CBF::Functor* );
    void _recvv( bool blocking, std::vector<IoVec>&, int src,
                int tag, int group, CommReq*, 
                CBF::Functor*, Response*, CBF::Functor* );
    void _send( bool blocking, void* buf, size_t len, int dest,
                            int tag, int group, CommReq*, CBF::Functor* );
    void _sendv( bool blocking,  std::vector<IoVec>&, int dest,
                            int tag, int group, CommReq*, CBF::Functor* );

    void passCtrlToFunction(int delay );
    void passCtrlToHades( int delay );

    OutBase*    m_out;
    Info*       m_info;

    std::deque< CommReq* >   m_sendQ;
    std::deque< CommReq* >   m_postedQ;
    std::deque< IORequest* > m_unexpectedQ;

    std::map< IO::NodeId, CommReq* > m_activeSendM; 

    CommReq     m_xxxReq;
    std::set<CommReq*>    m_blockedReqs;

    int m_matchTime;
    int m_copyTime;

    Link* m_outLink;
    Link* m_retLink;
    Link* m_delayLink;
    CBF::Functor* m_returnFunctor;
};

}
}

#endif
