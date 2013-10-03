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
#include "sst/elements/hermes/msgapi.h"

namespace SST {
namespace Firefly {

class Info;
class SendEntry;
class RecvEntry;
class RecvReq;


struct MatchHdr {
    uint32_t                count;
    Hermes::PayloadDataType dtype;
    uint32_t                tag;
    uint32_t                group;
    uint32_t                srcRank;
};

struct Hdr {
    enum { MatchMsg, LongMsgReq, LongMsgResp }    msgType;
    unsigned char           sendKey;
    unsigned char           recvKey;
    MatchHdr mHdr;
};

class MsgEntry {
  public:
    uint32_t                    srcNodeId;
    unsigned char               sendKey;
    MatchHdr                    hdr;
    std::vector<unsigned char>  buffer;
};

class DataMovement : public ProtocolAPI 
{

    static const uint32_t MaxNumPostedRecvs = 512;
    static const uint32_t MaxNumSends = 512;
    static const uint32_t ShortMsgLength = 4096;

    class SendReq: public Request {
      public:
        SendReq() : entry(NULL) {}
        Hdr    hdr;
        SendEntry*  entry;
    };

    class RecvReq: public Request {
      public:
        RecvReq() : recvEntry(NULL), msgEntry(NULL) {}
        enum { RecvHdr, RecvBody } state; 
        Hdr         hdr;
        RecvEntry*  recvEntry;
        MsgEntry*   msgEntry;
    };

    unsigned char saveSendReq( SendReq* req) {
        m_dbg.verbose(CALL_INFO,1,0,"key=%d\n",m_sendReqKey);
        m_sendReqM[ m_sendReqKey ] = req;
        return m_sendReqKey++;
    }

    SendReq* findSendReq( unsigned char key ) {
        m_dbg.verbose(CALL_INFO,1,0,"key=%d\n",key);
        SendReq* tmp = m_sendReqM[key];
        m_sendReqM.erase(key);
        return tmp;
    }

    unsigned char saveRecvReq( RecvReq* req) {
        m_dbg.verbose(CALL_INFO,1,0,"key=%d\n",m_recvReqKey);
        m_recvReqM[ m_recvReqKey ] = req;
        return m_recvReqKey++;
    }

    RecvReq* findRecvReq( unsigned char key ) {
        m_dbg.verbose(CALL_INFO,1,0,"key=%d\n",key);
        RecvReq* tmp = m_recvReqM[key];
        m_recvReqM.erase(key);
        return tmp;
    }


  public:
    DataMovement( SST::Params, Info* info, SST::Link* );
 
    virtual Request* getSendReq( );
    virtual Request* getRecvReq( IO::NodeId src );
    virtual Request* sendIODone( Request* );
    virtual Request* recvIODone( Request* );
    virtual Request* delayDone( Request* );
    virtual bool blocked();
    virtual void setup();

    MsgEntry* searchUnexpected(RecvEntry*,int& delay);
    bool canPostSend();
    void postSendEntry(SendEntry);
    bool canPostRecv();
    void postRecvEntry(RecvEntry);
    void completeLongMsg( MsgEntry*, RecvEntry* );
    void sleep();
    void enter();

    int getCopyDelay( int nbytes ) {
        return m_copyTime * nbytes;
    }

  private:
    RecvEntry* findMatch( MatchHdr&, int& delay );
    void handleMatchDelay( SST::Event* );
    void finishRecv( Request* );

    Info*                   m_info;
    std::deque<MsgEntry*>   m_unexpectedMsgQ;
    std::deque<RecvEntry*>  m_postedQ;
    std::deque<SendEntry*>  m_sendQ;
    std::deque<RecvReq*>    m_longMsgReqQ;
    std::deque<RecvReq*>    m_longMsgRespQ;
    int                     m_matchTime;
    int                     m_copyTime;
    bool                    m_sleep;

    std::map<unsigned char,SendReq*>    m_sendReqM;
    unsigned char                       m_sendReqKey;

    std::map<unsigned char,RecvReq*>    m_recvReqM;
    unsigned char                       m_recvReqKey;

    SST::Link*              m_link;
};

}
}
#endif
