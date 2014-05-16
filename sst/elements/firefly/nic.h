// Copyright 2013-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2014, Sandia Corporation
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
    int vNic;
    int num_vNics;

	NicInitEvent() :
		Event() {}

    NicInitEvent( int _node, int _vNic, int _num_vNics ) :
        Event(),
        node( _node ),
        vNic( _vNic ),
        num_vNics( _num_vNics )
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
        ar & BOOST_SERIALIZATION_NVP(vNic);
        ar & BOOST_SERIALIZATION_NVP(num_vNics);
    }
};

class NicCmdEvent : public Event {

  public:
    enum Type { PioSend, DmaSend, DmaRecv } type;
    int  node;
	int dst_vNic;
    int tag;
    std::vector<IoVec> iovec;
    void* key;

    NicCmdEvent( Type _type, int _vNic, int _node, int _tag,
            std::vector<IoVec>& _vec, void* _key ) :
        Event(),
        type( _type ),
        node( _node ),
		dst_vNic( _vNic ),  
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
        ar & BOOST_SERIALIZATION_NVP(dst_vNic);
        ar & BOOST_SERIALIZATION_NVP(tag);
        ar & BOOST_SERIALIZATION_NVP(iovec);
        ar & BOOST_SERIALIZATION_NVP(key);
    }
};

class NicRespEvent : public Event {

  public:
    enum Type { PioSend, DmaSend, DmaRecv, NeedRecv } type;
    int src_vNic;
    int node;
    int tag;
    int len;
    void* key;

    NicRespEvent( Type _type, int _vNic, int _node, int _tag,
            int _len, void* _key ) :
        Event(),
        type( _type ),
        src_vNic( _vNic ),
        node( _node ),
        tag( _tag ),
        len( _len ),
        key( _key )
    {
    }

    NicRespEvent( Type _type, int _vNic, int _node, int _tag,
            int _len ) :
        Event(),
        type( _type ),
        src_vNic( _vNic ),
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
        ar & BOOST_SERIALIZATION_NVP(src_vNic);
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

    struct MsgHdr {
        size_t len;
        int    tag;
        unsigned short    dst_vNicId;
        unsigned short    src_vNicId;
    };

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

        MsgHdr				hdr;
    };

    class VirtNic {
        Nic& m_nic;
      public:
        VirtNic( Nic&, int id, std::string portName );
		~VirtNic() {}
        void handleCoreEvent( Event* );
        void init( unsigned int phase );
        Link* m_toCoreLink;
        int id;
        void notifyRecvDmaDone( int src_vNic, int src, int tag, size_t len, void* key );
        void notifyNeedRecv( int src_vNic, int src, int tag, size_t len );
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
        int src_vNicNum() { return m_src_vNicNum; }
        void set_src_vNicNum( int val ) { m_src_vNicNum = val; }

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
        int           m_src_vNicNum;
    };

public:

    Nic(ComponentId_t, Params& );
    ~Nic();

    void init( unsigned int phase );
    int getNodeId() { return m_myNodeId; }
    int getNum_vNics() { return m_num_vNics; }

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

    void notifyRecvDmaDone(int vNic, int src_vNic, int src, int tag, size_t len, void* key) {
        m_vNicV[vNic]->notifyRecvDmaDone( src_vNic, src, tag, len, key );
    }

    void notifyNeedRecv( int vNic, int src_vNic, int src, int tag, size_t length ) {
    	m_dbg.verbose(CALL_INFO,2,0,"src_vNic=%d src=%d tag=%#x len=%lu\n",
                                            src_vNic,src,tag,length);

        m_vNicV[vNic]->notifyNeedRecv( src_vNic, src, tag, length );
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
    int                     m_num_vNics;
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
	int  m_bytesPerFlit;
}; 

} // namesapce Firefly 
} // namespace SST

#endif
