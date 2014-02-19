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
    typedef uint32_t NodeId;
    static const NodeId AnyId = -1;

    class VirtNic;

  private:

    class SelfEvent : public SST::Event {
      public:
        enum { DmaSend, DmaRecv, PioSend, DmaSendFini,
                DmaRecvFini, PioSendFini, NeedRecvFini,
                MatchDelay, ProcessMerlinEvent,
                ProcessSend } type;
        ~SelfEvent() {}

        VirtNic*            vNic;
        int                 node;
        int                 tag;
        std::vector<IoVec>  iovec;
        void*               key;
        size_t              len;
        MerlinFireflyEvent* mEvent;
        bool                retval;
    };

    struct MsgHdr {
        size_t len;
        int    tag;
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

    template < typename T1, typename T2, typename T3 >
    class HandlerBase3Args {
      public:
        virtual bool operator()( T1, T2, T3 ) = 0;
        virtual ~HandlerBase3Args() {}
    };

    template <typename classT, typename T1, typename T2, typename T3 >
    class Handler3Args : public HandlerBase3Args< T1, T2, T3 > {
      private:
        typedef bool (classT::*PtrMember)( T1, T2, T3 );
        classT* object;
        const PtrMember member;

      public:
        Handler3Args( classT* const object, PtrMember member ) :
            object(object),
            member(member)
        {}

        bool operator()( T1 arg1, T2 arg2, T3 arg3) {
            return (object->*member)( arg1, arg2, arg3);
        }
    };

    template < typename T1, typename T2, typename T3, typename T4 >
    class HandlerBase4Args {
      public:
        virtual bool operator()( T1, T2, T3, T4 ) = 0;
        virtual ~HandlerBase4Args() {}
    };

    template <typename classT, typename T1, typename T2,
                                        typename T3, typename T4 >
    class Handler4Args : public HandlerBase4Args< T1, T2, T3, T4 > {
      private:
        typedef bool (classT::*PtrMember)( T1, T2, T3, T4 );
        classT* object;
        const PtrMember member;

      public:
        Handler4Args( classT* const object, PtrMember member ) :
            object(object),
            member(member)
        {}

        bool operator()( T1 arg1, T2 arg2, T3 arg3, T4 arg4) {
            return (object->*member)( arg1, arg2, arg3, arg4 );
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
        int         src;
        int         tag;
        size_t      len;

      private:
        SelfEvent*  m_event;
    };

    class VirtNic {
      public:
        VirtNic( Nic&  nic, int id ) : 
            m_nic( nic ),
            m_id( id ),
            m_notifySendPioDone(NULL),
            m_notifySendDmaDone(NULL),
            m_notifyRecvDmaDone(NULL),
            m_notifyNeedRecv(NULL)
        {
        }
 
        int getNodeId() { 
            return m_nic.getNodeId( this );
        }

        bool canDmaSend() { return m_nic.canDmaSend( this ); }
        bool canDmaRecv() { return m_nic.canDmaRecv( this ); }

        void dmaSend( int dest, int tag, std::vector<IoVec>& vec, void* key ) {
            m_nic.dmaSend( this, dest, tag, vec, key );
        }

        void dmaRecv( int src, int tag, std::vector<IoVec>& vec, void* key ) {
            m_nic.dmaRecv( this, src, tag, vec, key );
        }

        void pioSend( int dest, int tag, std::vector<IoVec>& vec, void* key ) {
            m_nic.pioSend( this, dest, tag, vec, key );
        }

        inline void setNotifyOnSendDmaDone(HandlerBase<void*>* functor) { 
            m_notifySendDmaDone = functor;
        }

        inline void setNotifyOnRecvDmaDone(
                HandlerBase4Args<int,int,size_t,void*>* functor) { 
            m_notifyRecvDmaDone = functor;
        }

        inline void setNotifyOnSendPioDone(HandlerBase<void*>* functor) { 
            m_notifySendPioDone = functor;
        }

        inline void setNotifyNeedRecv(
                HandlerBase3Args<int,int,size_t>* functor) { 
            m_notifyNeedRecv = functor;
        }

        void notifySendPioDone( void* key ) {
            (*m_notifySendPioDone)(key );
        }

        void notifySendDmaDone( void* key ) {
            (*m_notifySendDmaDone)(key );
        }

        void notifyRecvDmaDone( int src, int tag, size_t len, void* key ) {
            (*m_notifyRecvDmaDone)( src, tag, len, key  );
        }

        void notifyNeedRecv( int src, int tag, size_t length ) {
            (*m_notifyNeedRecv)( src, tag, length );
        }

        int id() { return m_id; }

      private:   
        Nic& m_nic;
        int m_id;
        HandlerBase<void*>* m_notifySendPioDone; 
        HandlerBase<void*>* m_notifySendDmaDone; 
        HandlerBase4Args<int, int, size_t, void*>* m_notifyRecvDmaDone; 
        HandlerBase3Args<int, int, size_t>* m_notifyNeedRecv;
    };

    Nic(Component*, Params& );

    void init( unsigned int phase ) {
        m_linkControl->init(phase);
    }

    VirtNic* virtNicInit( );

    int getNodeId( ) { return m_myNodeId; }
    int getNodeId( VirtNic* vNic ) { return m_myNodeId; }
    bool canDmaSend( VirtNic* vNic ) { return true; }
    bool canDmaRecv( VirtNic* vNic ) { return true; }
    bool canPioSend( VirtNic* vNic ) { return true; } 
    void dmaSend( VirtNic*, int dest, int tag, std::vector<IoVec>&, void* key );
    void dmaRecv( VirtNic*, int src, int tag, std::vector<IoVec>&,  void* key );
    void pioSend( VirtNic*, int dest, int tag, std::vector<IoVec>&, void* key );


  private:
    void handleSelfEvent( Event* );
    void dmaSend( SelfEvent* );
    void pioSend( SelfEvent* );
    void dmaRecv( SelfEvent* );
    void processSend();
    Entry* processSend( Entry* );

    void schedEvent( SelfEvent* event, int delay = 0 ) {
        m_selfLink->send( delay, event );
    }
    
    bool processRecvEvent( MerlinFireflyEvent* );
    bool findRecv( MerlinFireflyEvent*, MsgHdr& );
    void moveEvent( MerlinFireflyEvent* );

    void notifySendPioDone( VirtNic* nic, void* key ) {
        SelfEvent* event = new SelfEvent;
        event->type = SelfEvent::PioSendFini;
        event->vNic = nic;
        event->key = key;
        m_selfLink->send( m_rxBusDelay, event );
    }

    void notifySendDmaDone( VirtNic* nic, void* key ) {
        SelfEvent* event = new SelfEvent;
        event->type = SelfEvent::DmaSendFini;
        event->vNic = nic;
        event->key = key;
        m_selfLink->send( m_rxBusDelay, event );
    }

    void notifyRecvDmaDone( VirtNic* nic, int src, int tag, size_t len, void* key ) {
        SelfEvent* event = new SelfEvent;
        event->type = SelfEvent::DmaRecvFini;
        event->vNic = nic;
        event->key = key;
        event->node = src;
        event->tag = tag;
        event->len = len;
        m_selfLink->send( m_rxBusDelay, event );
    }

    void notifyNeedRecv( VirtNic*nic, int src, int tag, size_t length ) {
        SelfEvent* event = new SelfEvent;
        event->type = SelfEvent::NeedRecvFini;
        event->vNic = nic;
        event->node = src;
        event->tag = tag;
        event->len = length;
        m_selfLink->send( m_rxBusDelay, event );
    }

    std::deque<Entry*>      m_sendQ;
    Entry*                  m_currentSend;
    std::vector< std::map< int, std::deque<Entry*> > > m_recvM;
    std::map< int, Entry* > m_activeRecvM;;
 
    int                     m_txBusDelay;
    int                     m_rxBusDelay;
    int                     m_rxMatchDelay;
    int                     m_txDelay;
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

    bool m_recvNotifyEnabled;
    int  m_packetId;
};

} // namesapce Firefly 
} // namespace SST

#endif
