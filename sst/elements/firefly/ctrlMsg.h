
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

  public:
    struct MsgReq {
        void* buf;
        size_t len;
        int node;
        int group;
        bool done;
        int type;
    };

    struct Hdr {
        int type;
        int group;
        int srcRank;
        size_t len;
    };

  private:

    class SendReq: public Request {
      public:
        Hdr hdr;
        MsgReq* entry;
    };

    class RecvReq: public Request {
      public:
        enum { RecvHdr, RecvBody } state;
        Hdr hdr;
        MsgReq* entry;
        std::vector<unsigned char> buf;
    };

  public:

    CtrlMsg( int verboseLevel, Output::output_location_t loc, Info* );

    virtual SendReq* getSendReq( );
    virtual RecvReq* getRecvReq( IO::NodeId src );
    virtual SendReq* sendIODone( Request* );
    virtual RecvReq* recvIODone( Request* );
    virtual Request* delayDone( Request* );

    void recv( void* buf, size_t len, int src,  int group, MsgReq* );
    void send( void* buf, size_t len, int dest, int group, MsgReq* );
    bool test( MsgReq * );

  private:

    MsgReq* findMatch( Hdr& );
    RecvReq* searchUnexpected( MsgReq* entry );

    Info* m_info;
    std::deque< MsgReq* > m_sendQ;
    std::deque< MsgReq* > m_postedQ;
    std::deque< RecvReq* > m_unexpectedQ;
};

}
}

#endif
