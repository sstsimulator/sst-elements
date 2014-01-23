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
    class VirtNic;
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
        VirtNic* vNic;
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
        int    vNicId;
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

    class VirtNic {
      public:
        VirtNic( Nic&  nic, int id ) : m_nic( nic ), m_id( id ) {
        }
 
        int getNodeId() { 
            return m_nic.getNodeId( this );
        }

        bool canDmaSend() { return m_nic.canDmaSend( this ); }
        bool canDmaRecv() { return m_nic.canDmaRecv( this ); }
        bool canPioSend() { return m_nic.canPioSend( this ); }

        void dmaSend( int dest, int tag, std::vector<IoVec>& vec, XXX* key ) {
            m_nic.dmaSend( this, dest, tag, vec, key );
        }
        void dmaRecv( int src, int tag, std::vector<IoVec>& vec, XXX* key ) {
            m_nic.dmaRecv( this, src, tag, vec, key );
        }
        void pioSend( int dest, int tag, std::vector<IoVec>& vec, XXX* key ) {
            m_nic.pioSend( this, dest, tag, vec, key );
        }

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

        void notifySendDmaDone( XXX* key ) {
            (*m_notifySendDmaDone)(key );
        }

        void notifyRecvDmaDone( XXX* key ) {
            (*m_notifyRecvDmaDone)(key );
        }

        void notifySendPioDone( XXX* key ) {
            (*m_notifySendPioDone)(key );
        }

        void notifyNeedRecv( XXX* key ) {
            (*m_notifyNeedRecv)( key );
        }
        int id() { return m_id; }

      private:   
        Nic& m_nic;
        HandlerBase<XXX*>* m_notifySendDmaDone; 
        HandlerBase<XXX*>* m_notifyRecvDmaDone; 
        HandlerBase<XXX*>* m_notifySendPioDone; 
        HandlerBase<XXX*>* m_notifyNeedRecv;
        int m_id;
    };

    Nic(Component*, Params& );
    void init( unsigned int phase );
    VirtNic* virtNicInit( );

    int getNodeId( ) { return m_myNodeId; }
    int getNodeId( VirtNic* vNic ) { return m_myNodeId; }
    bool canDmaSend( VirtNic* vNic );
    bool canDmaRecv( VirtNic* vNic );
    bool canPioSend( VirtNic* vNic );
    void dmaSend( VirtNic*, int dest, int tag, std::vector<IoVec>&, XXX* key );
    void dmaRecv( VirtNic*, int src, int tag, std::vector<IoVec>&,  XXX* key );
    void pioSend( VirtNic*, int dest, int tag, std::vector<IoVec>&, XXX* key );


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
    std::vector<VirtNic*>   m_vNicV;
};

} // namesapce Firefly 
} // namespace SST

#endif
