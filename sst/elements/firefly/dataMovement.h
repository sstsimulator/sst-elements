// Copyright 2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef COMPONENTS_FIREFLY_DATAMOVEMENT_H
#define COMPONENTS_FIREFLY_DATAMOVEMENT_H

#include <sst/core/output.h>
#include "protocolAPI.h"
#include "msgHdr.h"

namespace SST {
namespace Firefly {

class Info;
class SendEntry;
class RecvEntry;
class MsgEntry;

class DataMovement : public ProtocolAPI 
{
    static const uint32_t MaxNumPostedRecvs = 32;
    static const uint32_t MaxNumSends = 32;

    class SendReq: public Request {
      public:
        Hdr hdr;
        SendEntry* entry;
    };

    class RecvReq: public Request {
      public:
        enum { RecvHdr, RecvBody } state; 
        Hdr hdr;
        RecvEntry*  recvEntry;
        MsgEntry*   msgEntry;
    };

  public:
    DataMovement( int verboseLevel, Output::output_location_t loc, Info* info );
 
    virtual SendReq* getSendReq( );
    virtual RecvReq* getRecvReq( IO::NodeId src );
    virtual SendReq* sendIODone( Request* );
    virtual RecvReq* recvIODone( Request* );
    virtual Request* delayDone( Request* );

    MsgEntry* searchUnexpected(RecvEntry*);
    bool canPostSend();
    void postSendEntry(SendEntry);
    bool canPostRecv();
    void postRecvEntry(RecvEntry);

  private:
    RecvEntry* findMatch( Hdr& );
    void handleMatchDelay( SST::Event* );
    void finishRecv( Request* );

    Info*                   m_info;
    std::deque<MsgEntry*>   m_unexpectedMsgQ;
    std::deque<RecvEntry*>  m_postedQ;
    std::deque<SendEntry*>  m_sendQ;
};

}
}
#endif
