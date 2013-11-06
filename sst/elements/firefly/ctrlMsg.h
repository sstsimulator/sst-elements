
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

class CtrlMsg : public ProtocolAPI {

  private:

    struct Hdr {
        size_t  len;
        int     tag;
        int     src;
        int     group;
    };

    struct BaseInfo {
        BaseInfo( std::vector<IoVec> _ioVec, int _tag, int _group) : 
            ioVec( _ioVec ),
            tag( _tag ),
            group( _group ),
            len( 0 )
        {
            for ( unsigned int i = 0; i < ioVec.size(); i++ ) {
                len += ioVec[i].len;
            }
        }

        virtual ~BaseInfo() {} 
        std::vector<IoVec> ioVec;
        int     tag;
        int     group;
        size_t  len;
    };

    struct RecvInfo : public BaseInfo {
        RecvInfo( std::vector<IoVec> _ioVec, int _src, int _tag, int _group) : 
            BaseInfo( _ioVec, _tag, _group ),
            src( _src ) 
        {}
        int     src;
    };

    struct SendInfo : public BaseInfo {
        SendInfo( std::vector<IoVec> ioVec, int _dest, int _tag, int _group) : 
            BaseInfo( ioVec, _tag, _group ),
            dest( _dest ) 
        {}
        int     dest;
    };

  public:

    class CommReq {
      public:
        CommReq() : m_info( NULL ) {}
        ~CommReq() {
            if ( m_info ) delete m_info;
        }
        void initRecv( std::vector<IoVec>& ioVec, int src, int tag, int group ) 
        {
            assert( ! m_info );
            m_info = new RecvInfo( ioVec, src, tag, group );
            m_type = CommReq::Recv;
        }
        void initSend( std::vector<IoVec>& ioVec, int src, int tag, int group ) 
        {
            assert( ! m_info );
            m_info = new SendInfo( ioVec, src, tag, group );
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

      private:
        enum { Send, Recv } m_type;
        BaseInfo*           m_info;
    };

  private:

    class SendReq: public Request {
      public:
        Hdr         hdr;
        CommReq*    commReq;
    };

    class RecvReq: public Request {
      public:
        enum { RecvHdr, RecvBody } state;
        Hdr         hdr;
        CommReq*    commReq;
        std::vector<unsigned char> buf;
    };

  public:

    CtrlMsg( SST::Params, Info*, SST::Link* );

    // this group of function is used by hades 
    void setup();
    std::string name() { return "CtrlMsg"; }
    virtual Request* getSendReq( );
    virtual Request* getRecvReq( IO::NodeId src );
    virtual Request* sendIODone( Request* );
    virtual Request* recvIODone( Request* );
    virtual Request* delayDone( Request* );
    virtual bool unblocked();
    void setRetLink(SST::Link* link) { m_retLink = link;}

    // this group of functions is used by the function state machine
    void send( void* buf, size_t len, int dest, int tag, int group );
    void sendv( std::vector<IoVec>&, int dest, int tag, int group );
    void recv( void* buf, size_t len, int src, int tag, int group );
    void irecv( void* buf, size_t len, int src, int tag, int group, CommReq* );
    void irecvv( std::vector<IoVec>&, int src,  int tag, int group, CommReq* );
    void wait( CommReq* );

  private:

    int getCopyDelay( int nbytes ) { return m_copyTime * nbytes; }

    void _recv( void* buf, size_t len, int src, int tag, int group, CommReq* );
    void _recvv( std::vector<IoVec>&, int src, int tag, int group, CommReq* );
    void _send( void* buf, size_t len, int dest, int tag, int group, CommReq* );
    void _sendv( std::vector<IoVec>&, int dest, int tag, int group, CommReq* );

    bool test( CommReq* r, int& delay ); 
    CommReq* findMatch( Hdr&, int& delay );
    RecvReq* searchUnexpected( RecvInfo& info, int& delay );

    Info* m_info;

    std::deque< CommReq* > m_sendQ;
    std::deque< CommReq* > m_postedQ;
    std::deque< RecvReq* > m_unexpectedQ;

    CommReq* m_blockedReq;
    CommReq  m_xxxReq;

    int m_matchTime;
    int m_copyTime;

    SST::Link* m_outLink;
    SST::Link* m_retLink;
};

}
}

#endif
