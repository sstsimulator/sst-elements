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


#ifndef COMPONENTS_FIREFLY_NIC_H 
#define COMPONENTS_FIREFLY_NIC_H 

#include <sst/core/module.h>
#include <sst/core/component.h>
#include <sst/core/output.h>

#include "sst/elements/merlin/linkControl.h"
#include "ioVec.h"

namespace SST {
namespace Firefly {

class MerlinFireflyEvent;

class Nic : public SST::Module  {

  public:
    struct XXX { 
        int     src;
        size_t  len;
        void   *usrData;
    };

  private:

    class SelfEvent : public SST::Event {
      public:
        enum { DmaSend, DmaRecv, PioSend } type;
        ~SelfEvent() {}

        int node;
        int tag;
        XXX *key;
        std::vector<IoVec>   iovec;
    };

    class NicEvent : public SST::Event {
      public:
        enum { DmaRecvFini, DmaSendFini, PioSendFini, Stall } type;
        XXX *key;
        int src;
    };

    struct MsgHdr {
        int    tag;
        size_t len;
    };

  public:

    // Functor classes for handling callbacks
    template < typename argT >
    class HandlerBase {
      public:
        virtual bool operator()(argT) = 0;
        virtual ~HandlerBase() {}
    };

    template <typename classT, typename argT >
    class Handler : public HandlerBase<argT> {
      private:
        typedef bool (classT::*PtrMember)(argT);
        classT* object;
        const PtrMember member;
        argT data;

      public:
        Handler( classT* const object, PtrMember member ) :
            object(object),
            member(member)
        {}

        bool operator()(argT data) {
            return (object->*member)(data);
        }
    };

    class Entry {
      public:
        Entry( SelfEvent* event ) : 
            currentVec(0),
            currentPos(0),
            currentLen(0),
            m_event( event )
        {
        }

        int node() { return m_event->node; }
        std::vector<IoVec>& ioVec() { return m_event->iovec; }
        SelfEvent* event() { return m_event; }
        size_t totalBytes() {
            size_t bytes = 0;
            for ( unsigned int i = 0; i < m_event->iovec.size(); i++ ) {
                bytes += m_event->iovec[i].len; 
            } 
            return bytes;
        }
        size_t      currentVec;
        size_t      currentPos;  
        size_t      currentLen; 

      private:
        SelfEvent*  m_event;
    };


    Nic(Component*, Params& );
    void init( unsigned int phase );

    int getNodeId() { return m_myNodeId; }

    bool canDmaSend();
    bool canDmaRecv();
    bool canPioSend();

    void dmaSend( int dest, int tag, std::vector<IoVec>&, XXX* key );
    void dmaRecv( int src, int tag, std::vector<IoVec>&,  XXX* key );
    void pioSend( int dest, int tag, std::vector<IoVec>&, XXX* key );

    inline void setNotifyOnSendDmaDone(HandlerBase<XXX*>* functor) { 
        m_notifySendDmaDone = functor;
    }

    inline void setNotifyOnRecvDmaDone(HandlerBase<XXX*>* functor) { 
        m_notifyRecvDmaDone = functor;
    }

    inline void setNotifyOnSendPioDone(HandlerBase<XXX*>* functor) { 
        m_notifySendPioDone = functor;
    }

    inline void setNotifyNeedRecv(HandlerBase<XXX*>* functor) { 
        m_notifyNeedRecv = functor;
    }

  private:
    void handleSelfEvent( Event* );
    void dmaSend( SelfEvent* );
    void pioSend( SelfEvent* );
    void dmaRecv( SelfEvent* );
    bool processSend();
    bool processRecvEvent( MerlinFireflyEvent* );

    std::deque<Entry*>          m_sendQ;
    std::map< int, std::deque<Entry*> > m_recvM;
    std::map< int, Entry* > m_activeRecvM;;

    HandlerBase<XXX*>* m_notifySendDmaDone; 
    HandlerBase<XXX*>* m_notifyRecvDmaDone; 
    HandlerBase<XXX*>* m_notifySendPioDone; 
    HandlerBase<XXX*>* m_notifyNeedRecv;
 
    int                     m_txBusDelay;
    MerlinFireflyEvent*     m_pendingMerlinEvent;
    int                     m_myNodeId;
    SST::Link*              m_selfLink;
    int                     m_num_vcs;

    // the interface to to Merlin
    Merlin::LinkControl*                m_linkControl;
    Merlin::LinkControl::Handler<Nic> * m_recvNotifyFunctor;
    Merlin::LinkControl::Handler<Nic> * m_sendNotifyFunctor;
    bool sendNotify(int);
    bool recvNotify(int);

    Output                  m_dbg;
};

} // namesapce Firefly 
} // namespace SST

#endif
