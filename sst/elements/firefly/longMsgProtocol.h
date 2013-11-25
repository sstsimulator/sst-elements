
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


#ifndef COMPONENTS_FIREFLY_LONGMSGPROTOCOL_H
#define COMPONENTS_FIREFLY_LONGMSGPROTOCOL_H

#include "ctrlMsg.h"
#include "sst/elements/hermes/msgapi.h"
#include "entry.h"

namespace SST {
namespace Firefly {

class SendEntry;
class RecvEntry;

class LongMsgProtocol : public CtrlMsg  {

    struct MsgHdr {
        uint32_t                count;
        Hermes::PayloadDataType dtype;
        uint32_t                tag;
        uint32_t                group;
        uint32_t                srcRank;
    };

    class SendCallbackEntry : public CBF {
      public:
        MsgHdr              hdr;
        std::vector<IoVec>  vec;
        SendEntry*          sendEntry;
        CommReq             commReq;
        IO::NodeId          destNode;
        int                 key;
        CtrlMsg::Response   ctrlResponse;
    };

    class RecvCallbackEntry : public CBF {
      public:
        MsgHdr              hdr;
        std::vector<IoVec>  vec;
        RecvEntry*          recvEntry;
        CommReq             commReq;
        std::vector<unsigned char> buf;
        CtrlMsg::Response   ctrlResponse;
        int                 key;
        CBF::Functor*       doneCallback;
        IO::NodeId          srcNode;
    };

    typedef StaticArg_Functor<LongMsgProtocol, CBF*, CBF*>  CBF_Functor;

    static const uint32_t MaxNumPostedRecvs = 512;
    static const uint32_t MaxNumSends = 512;
    static const uint32_t ShortMsgLength = 4096;

    static const uint32_t ShortMsgTag =   0x80000000;
    static const uint32_t LongMsgTag =    0x40000000;
    static const uint32_t LongMsgRdyTag = 0x20000000;
    static const uint32_t LongMsgBdyTag = 0x10000000;
    static const uint32_t TagMask = 0xf0000000;

  public:
    LongMsgProtocol( Component* owner, Params& );

    virtual void setup();
    std::string name() { return "LongMsgProtocol"; }

    void setRetLink(Link* link);
    void wait( Hermes::MessageRequest, 
                Hermes::MessageResponse* resp = NULL );
    void waitAny( int count, Hermes::MessageRequest req[], int *index, 
                                    Hermes::MessageResponse* resp  );
    void waitAll( int count, Hermes::MessageRequest req[], 
                                    Hermes::MessageResponse* resp[]  );
    void postSendEntry(SendEntry*);
    void postRecvEntry(RecvEntry*);

  private:
    void retHandler(Event*);
    void returnToFunction();
    void finished(CBF*);
    void finishRequest( Hermes::MessageRequest, Hermes::MessageResponse *);

    void block( int count, Hermes::MessageRequest req[], 
                  Hermes::MessageResponse* resp[], int* index = NULL ); 

    void block( Hermes::MessageRequest req[], Hermes::MessageResponse* resp[] );

    void unblock( Hermes::MessageRequest ); 

    CBF* postRecvAny_CB( CBF* );
    CBF* postSendEntry_CB( CBF* );
    CBF* processLongMsg_irecv_ret_CB( CBF* );
    CBF* processLongMsg_irecv_done_CB( CBF* );
    CBF* longMsgSendRdy_CB( CBF* );
    CBF* processLongMsgRdyMsg_CB( CBF* );

    void postRecvAny( );
    void processLongMsgRdyMsg( RecvCallbackEntry* );
    void processLongMsg( RecvCallbackEntry* );
    void processShortMsg( RecvCallbackEntry* );

    bool checkMatch( MsgHdr&, RecvEntry& );

    std::map<int,SendCallbackEntry*>  m_longSendM;

    Component*  m_owner;
    Output      m_my_dbg;
    Link*       m_my_retLink;
    int         m_longMsgKey;
    int         genLongMsgKey() { return m_longMsgKey++ & 0xffff; }

    std::deque<RecvEntry*>  m_postedQ;

    struct Blocked {
        Hermes::MessageRequest     req;     
        Hermes::MessageResponse*   resp;
        int*                       index;
    };
    std::vector<Blocked>    m_blockedList;
};

}
}

#endif
