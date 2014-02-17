
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
#include "ctrlMsgFunctors.h"

namespace SST {
namespace Firefly {

class SendEntry;
class RecvEntry;

class LongMsgProtocol : public CtrlMsg::API  {

    class SelfEvent : public SST::Event {
      public:
        SelfEvent( CtrlMsg::FunctorBase_0<bool>* arg) : Event(), callback(arg) {}
        CtrlMsg::FunctorBase_0<bool>* callback; 
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
        CtrlMsg::region_t   region;
    };

    class RecvCallbackEntry {
      public:
        MsgHdr              hdr;
        std::vector<IoVec>  vec;
        RecvEntry*          recvEntry;
        CtrlMsg::CommReq    commReq;
        std::vector<unsigned char> buf;
    };

    typedef CtrlMsg::FunctorStatic_0<LongMsgProtocol, SendCallbackEntry*, bool> 
                                                            SCBE_Functor;
    typedef CtrlMsg::FunctorStatic_0<LongMsgProtocol, RecvCallbackEntry*, bool> 
                                                            RCBE_Functor;
    typedef CtrlMsg::FunctorStatic_0<LongMsgProtocol, RecvEntry*, bool> 
                                                            RE_Functor;

    typedef CtrlMsg::Functor_1<LongMsgProtocol, 
                                CtrlMsg::CommReq*, bool > XXX_Functor;

    static const uint32_t MaxNumPostedRecvs = 512;
    static const uint32_t ShortMsgLength = 1024*16; 

    static const uint32_t ShortMsgTag =   0x80000000;
    static const uint32_t LongMsgTag =    0x40000000;
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
    bool waitAny_CB( CtrlMsg::CommReq* );
    bool waitAnyDelay_CB( RecvCallbackEntry* );
    bool postRecvAny_irecv_CB( RecvCallbackEntry* );
    bool postSendEntry_CB( SendCallbackEntry* );
    bool processSendEntryFini_CB( SendCallbackEntry* );

    bool postRecvEntry_CB( RecvCallbackEntry* );
    bool postRecvEntry_CB( RecvEntry* );

    void postRecvAny( );
    void processLongMsg( RecvCallbackEntry* );
    bool processLongMsg_CB( RecvCallbackEntry* );
    void processShortMsg( RecvCallbackEntry* );
    bool processShortMsg_CB( RecvCallbackEntry* );
    void schedDelay( int, CtrlMsg::FunctorBase_0<bool>* );
    
    bool processRegionEvents();

    bool checkMatch( MsgHdr&, RecvEntry& );
    int calcMatchDelay( int ) { return 0; }
    int calcCopyDelay( int ) { return 0; }

    Component*  m_owner;
    Output      m_my_dbg;

    Link*       m_my_retLink;
    Link*       m_my_selfLink;
    int         m_longMsgRegion;
    int         genLongRegion() { return m_longMsgRegion++ & 0x3f; }

    std::deque<RecvEntry*>  m_postedQ;

    std::deque< RecvCallbackEntry* >      m_unexpectedQ;

    struct Blocked {
        Hermes::MessageRequest     req;     
        Hermes::MessageResponse*   resp;
        int*                       index;
    };

    std::vector<Blocked>    m_blockedList;

    CtrlMsg::RegionEventQ*         m_regionEventQ;
    std::map< CtrlMsg::region_t, SendCallbackEntry* > m_regionM;
};

}
}

#endif
