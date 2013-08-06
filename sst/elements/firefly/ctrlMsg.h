
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

#include "sst/core/output.h"
#include "protocolAPI.h"

namespace SST {
namespace Firefly {

class Info;

class CtrlMsg : public ProtocolAPI {

    struct Hdr {
        size_t  len;
        int     tag;
        int     src;
        int     group;
    };

    struct BaseInfo {
        BaseInfo( void* _buf, size_t _len, int _tag, int _group) : 
            buf( _buf ),
            len( _len ),
            tag( _tag ),
            group( _group )
        {}

        virtual ~BaseInfo() {} 
        void*   buf;
        size_t  len;
        int     tag;
        int     group;
    };

    struct RecvInfo : public BaseInfo {
        RecvInfo( void* _buf, size_t _len, int _src, int _tag, int _group) : 
            BaseInfo( _buf, _len, _tag, _group ),
            src( _src ) 
        {}
        int     src;
    };

    struct SendInfo : public BaseInfo {
        SendInfo( void* _buf, size_t _len, int _dest, int _tag, int _group) : 
            BaseInfo( _buf, _len, _tag, _group ),
            dest( _dest ) 
        {}
        int     dest;
    };

  public:

    struct CommReq {
        CommReq() : info( NULL ) {}
        BaseInfo*    info;
        bool done;
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

    CtrlMsg( int verboseLevel, Output::output_location_t loc, Info* );

    virtual SendReq* getSendReq( );
    virtual RecvReq* getRecvReq( IO::NodeId src );
    virtual SendReq* sendIODone( Request* );
    virtual RecvReq* recvIODone( Request* );
    virtual Request* delayDone( Request* );

    void recv( void* buf, size_t len, int src,  int tag, int group, CommReq* );
    void send( void* buf, size_t len, int dest, int tag, int group, CommReq* );
    bool test( CommReq*  );


  private:

    CommReq* findMatch( Hdr& );
    RecvReq* searchUnexpected( RecvInfo& info );

    Info* m_info;
    std::deque< CommReq* > m_sendQ;
    std::deque< CommReq* > m_postedQ;
    std::deque< RecvReq* > m_unexpectedQ;
};

}
}

#endif
