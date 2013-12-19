
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

    class SelfEvent : public SST::Event {
      public:
        SelfEvent( VoidArg_FunctorBase<bool>* arg) : Event(), callback(arg) {}
        VoidArg_FunctorBase<bool>* callback; 
    };

    struct MsgHdr {
        uint32_t                count;
        Hermes::PayloadDataType dtype;
        uint32_t                tag;
        uint32_t                group;
        uint32_t                srcRank;
    };

    class SendCallbackEntry {
      public:
        MsgHdr              hdr;
        std::vector<IoVec>  vec;
        SendEntry*          sendEntry;
        int                 key;
    };

    class RecvCallbackEntry {
      public:
        MsgHdr              hdr;
        std::vector<IoVec>  vec;
        RecvEntry*          recvEntry;
        CommReq             commReq;
        std::vector<unsigned char> buf;
        CtrlMsg::Response   ctrlResponse;
        int                 key;
        IO::NodeId          srcNode;
    };

    typedef StaticArg_Functor<LongMsgProtocol, SendCallbackEntry*, bool> 
                                                            SCBE_Functor;
    typedef StaticArg_Functor<LongMsgProtocol, RecvCallbackEntry*, bool> 
                                                            RCBE_Functor;
    typedef StaticArg_Functor<LongMsgProtocol, RecvEntry*, bool> 
                                                            RE_Functor;

    typedef StaticArg_Functor<LongMsgProtocol, void*, bool > XXX_Functor;

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
    void selfHandler(Event*);
    void returnToFunction();
    void finishRecvCBE( RecvEntry&, MsgHdr& );
    void finishSendCBE( SendEntry& );
    void finishRequest( Hermes::MessageRequest, Hermes::MessageResponse *);

    void block( int count, Hermes::MessageRequest req[], 
                  Hermes::MessageResponse* resp[], int* index = NULL ); 

    void block( Hermes::MessageRequest req[], Hermes::MessageResponse* resp[] );

    bool unblock( Hermes::MessageRequest ); 

    void processBlocked( Hermes::MessageRequest );
    bool waitAny_CB( void * );
    bool postRecvAny_irecv_CB( RecvCallbackEntry* );
    void processRecvAny( RecvCallbackEntry* );
    bool processRecvAny_CB( RecvCallbackEntry* );
    bool postSendEntry_CB( SendCallbackEntry* );

    bool postRecvEntry_CB( RecvCallbackEntry* );
    bool postRecvEntry_CB( RecvEntry* );
    bool processLongMsg_irecv_CB( RecvCallbackEntry* );
    bool longMsgSendRdy_CB( SendCallbackEntry* );
    bool processLongMsgRdyMsg_CB( SendCallbackEntry* );

    void postRecvAny( );
    void processLongMsgRdyMsg( int key, int dest );
    void processLongMsg( RecvCallbackEntry* );
    void processShortMsg( RecvCallbackEntry* );
    bool processShortMsg_CB( RecvCallbackEntry* );
    void schedDelay( int, VoidArg_FunctorBase<bool>* );

    bool checkMatch( MsgHdr&, RecvEntry& );
    int calcMatchDelay( int ) { return 0; }
    int calcCopyDelay( int ) { return 0; }

    std::map<int,SendCallbackEntry*>  m_longSendM;

    Component*  m_owner;
    Output      m_my_dbg;
    Link*       m_my_retLink;
    Link*       m_my_selfLink;
    int         m_longMsgKey;
    int         genLongMsgKey() { return m_longMsgKey++ & 0xffff; }

    std::deque<RecvEntry*>  m_postedQ;

    std::deque< RecvCallbackEntry* >      m_unexpectedQ;
    std::deque< RecvCallbackEntry* >      m_longRecvDoneQ;

    struct Blocked {
        Hermes::MessageRequest     req;     
        Hermes::MessageResponse*   resp;
        int*                       index;
    };
    std::vector<Blocked>    m_blockedList;

    std::set<CommReq*>      m_commReqs;
};

}
}

#endif
