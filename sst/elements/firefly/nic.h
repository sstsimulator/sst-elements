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

class NicInitEvent : public Event {

  public:
    int node;

    NicInitEvent( int _node ) :
        Event(),
        node( _node )
    {
    }

  private:

    friend class boost::serialization::access;
    template<class Archive>
    void
    serialize(Archive & ar, const unsigned int version )
    {
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Event);
        ar & BOOST_SERIALIZATION_NVP(node);
    }
};

class NicCmdEvent : public Event {

  public:
    enum Type { PioSend, DmaSend, DmaRecv } type;
    int  node;
    int tag;
    std::vector<IoVec> iovec;
    void* key;

    NicCmdEvent( Type _type, int _node, int _tag,
            std::vector<IoVec>& _vec, void* _key ) :
        Event(),
        type( _type ),
        node( _node ),
        tag( _tag ),
        iovec( _vec ),
        key( _key )
    {
    }

  private:

    friend class boost::serialization::access;
    template<class Archive>
    void
    serialize(Archive & ar, const unsigned int version )
    {
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Event);
        ar & BOOST_SERIALIZATION_NVP(type);
        ar & BOOST_SERIALIZATION_NVP(node);
        ar & BOOST_SERIALIZATION_NVP(tag);
        ar & BOOST_SERIALIZATION_NVP(iovec);
        ar & BOOST_SERIALIZATION_NVP(key);
    }
};

class NicRespEvent : public Event {

  public:
    enum Type { PioSend, DmaSend, DmaRecv, NeedRecv } type;
    int  node;
    int tag;
    int len;
    void* key;

    NicRespEvent( Type _type, int _node, int _tag,
            int _len, void* _key ) :
        Event(),
        type( _type ),
        node( _node ),
        tag( _tag ),
        len( _len ),
        key( _key )
    {
    }

    NicRespEvent( Type _type, int _node, int _tag,
            int _len ) :
        Event(),
        type( _type ),
        node( _node ),
        tag( _tag ),
        len( _len )
    {
    }

    NicRespEvent( Type _type, void* _key ) :
        Event(),
        type( _type ),
        key( _key )
    {
    }

  private:

    friend class boost::serialization::access;
    template<class Archive>
    void
    serialize(Archive & ar, const unsigned int version )
    {
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Event);
        ar & BOOST_SERIALIZATION_NVP(type);
        ar & BOOST_SERIALIZATION_NVP(node);
        ar & BOOST_SERIALIZATION_NVP(tag);
        ar & BOOST_SERIALIZATION_NVP(len);
        ar & BOOST_SERIALIZATION_NVP(key);
    }
};

class Nic : public SST::Component  {

  public:
    typedef uint32_t NodeId;
    static const NodeId AnyId = -1;

  private:

    class SelfEvent : public SST::Event {
      public:
        enum { MatchDelay, ProcessMerlinEvent,
                ProcessSend } type;
        ~SelfEvent() {}

        int                 node;
        int                 tag;
        std::vector<IoVec>  iovec;
        void*               key;
        size_t              len;
        MerlinFireflyEvent* mEvent;
        bool                retval;

        int                 vNicNum;
    };

    struct MsgHdr {
        size_t len;
        int    tag;
        int    vNicId;
    };

    class VirtNic {
        Nic& m_nic;
      public:
        VirtNic( Nic&, int id );
        void handleCoreEvent( Event* );
        void init( unsigned int phase );
        Link* m_toCoreLink;
        int id;
        void notifyRecvDmaDone( int src, int tag, size_t len, void* key );
        void notifyNeedRecv( int src, int tag, size_t len );
        void notifySendDmaDone( void* key );
        void notifySendPioDone( void* key );
    };


    class Entry {
      public:
        Entry( int _vNicNum, NicCmdEvent* event ) : 
            currentVec(0),
            currentPos(0),
            currentLen(0),
            m_event( event ),
            m_vNicNum( _vNicNum )
        {
        }

        int node() { return m_event->node; }

        int vNicNum() { return m_vNicNum; }
        std::vector<IoVec>& ioVec() { return m_event->iovec; }

        NicCmdEvent* event() { return m_event; }

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
        NicCmdEvent*  m_event;
        int           m_vNicNum;
    };

public:

    Nic(ComponentId_t, Params& );

    void init( unsigned int phase );
    int getNodeId() { return m_myNodeId; }

  private:
    void handleSelfEvent( Event* );
    void handleVnicEvent( Event*, int );
    void dmaSend( NicCmdEvent*, int );
    void pioSend( NicCmdEvent*, int );
    void dmaRecv( NicCmdEvent*, int );
    void processSend();
    Entry* processSend( Entry* );

    void schedEvent( SelfEvent* event, int delay = 0 ) {
        m_selfLink->send( delay, event );
    }
    
    bool processRecvEvent( MerlinFireflyEvent* );
    bool findRecv( MerlinFireflyEvent*, MsgHdr& );
    void moveEvent( MerlinFireflyEvent* );

    void notifySendDmaDone( int vNicNum, void* key ) {
        m_vNicV[vNicNum]->notifySendDmaDone(  key );
    }
    void notifySendPioDone( int vNicNum, void* key ) {
        m_vNicV[vNicNum]->notifySendPioDone(  key );
    }

    void notifyRecvDmaDone( int vNicNum, int src, int tag, size_t len, void* key ) {
        m_vNicV[vNicNum]->notifyRecvDmaDone( src, tag, len, key );
    }

    void notifyNeedRecv( int vNicNum, int src, int tag, size_t length ) {
        m_vNicV[vNicNum]->notifyNeedRecv( src, tag, length );
    }

    size_t copyIn( Output& dbg, Nic::Entry& entry, MerlinFireflyEvent& event );
    bool  copyOut( Output& dbg, MerlinFireflyEvent& event, Nic::Entry& entry );

    int fattree_ID_to_IP(int id);
    int IP_to_fattree_ID(int ip);
    int NetToId( int );
    int IdToNet( int );

    std::deque<Entry*>      m_sendQ;
    Entry*                  m_currentSend;
    std::vector< std::map< int, std::deque<Entry*> > > m_recvM;
    std::map< int, Entry* > m_activeRecvM;;
 
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
    int  m_ftRadix;
    int  m_ftLoading;

}; 

} // namesapce Firefly 
} // namespace SST

#endif
