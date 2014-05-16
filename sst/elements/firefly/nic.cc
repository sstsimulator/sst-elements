// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include "sst_config.h"
#include <sst/core/component.h>
#include <sst/core/params.h>
#include <sst/core/timeLord.h>
#include <sst/core/element.h>

#include <sstream>

#include "nic.h"
#include "merlinEvent.h"

using namespace SST;
using namespace SST::Firefly;


static void print( Output& dbg, char* buf, int len );

Nic::Nic(ComponentId_t id, Params &params) :
    Component( id ),
    m_currentSend( NULL ),
    m_rxMatchDelay( 100 ),
    m_txDelay( 50 ),
    m_pendingMerlinEvent( NULL ),
    m_recvNotifyEnabled( false ),
    m_packetId(0),
    m_ftRadix(0),
	m_bytesPerFlit(8)
{
    m_myNodeId = params.find_integer("nid", -1);
    assert( m_myNodeId != -1 );

    m_dbg.verbose(CALL_INFO,1,0,"\n");
    char buffer[100];
    snprintf(buffer,100,"@t:%d:Nic::@p():@l ",m_myNodeId);

    m_dbg.init(buffer, params.find_integer("verboseLevel",0),0,
       (Output::output_location_t)params.find_integer("debug", 0));

    m_rxMatchDelay = params.find_integer( "rxMatchDelay_ns", 100 );
    m_txDelay =      params.find_integer( "txDelay_ns", 50 );

    std::string topo  = params.find_string( "topology", "" );

    if ( 0 == topo.compare("merlin.fattree") ) {
        m_ftLoading = params.find_integer( "fattree:loading", -1 );
        m_ftRadix = params.find_integer( "fattree:radix", -1);

        if ( -1 == m_ftLoading || -1 == m_ftRadix ) {
            assert(0);
        }
    	m_dbg.verbose(CALL_INFO,1,0,"Fattree\n");

    } else if ( 0 == topo.compare("merlin.torus") ) {
    	m_dbg.verbose(CALL_INFO,1,0,"Torus\n");
    } else {
		assert(0);
	}
    m_bytesPerFlit = params.find_integer("bytesPerFlit",8);

    m_num_vcs = params.find_integer("num_vcs",2);
    std::string link_bw = params.find_string("link_bw","2GHz");
    TimeConverter* tc = Simulation::getSimulation()->
                            getTimeLord()->getTimeConverter(link_bw);
    assert( tc );

    int buffer_size = params.find_integer("buffer_size",100);

    m_dbg.verbose(CALL_INFO,1,0,"id=%d num_vcs=%d buffer_size=%d link_bw=%s\n",
                m_myNodeId, m_num_vcs, buffer_size, link_bw.c_str());

    std::vector<int> buf_size;
    buf_size.resize(m_num_vcs);
    for ( unsigned int i = 0; i < buf_size.size(); i++ ) {
        buf_size[i] = buffer_size;
    }

    m_linkControl = (Merlin::LinkControl*)loadModule(
                    params.find_string("module"), params);
    assert( m_linkControl );

    m_linkControl->configureLink(this, 
						params.find_string("rtrPortName","rtr"),
						tc, m_num_vcs, &buf_size[0], &buf_size[0]);

    m_recvNotifyFunctor =
        new Merlin::LinkControl::Handler<Nic>(this,&Nic::recvNotify );
    assert( m_recvNotifyFunctor );

    m_sendNotifyFunctor =
        new Merlin::LinkControl::Handler<Nic>(this,&Nic::sendNotify );
    assert( m_sendNotifyFunctor );

    m_selfLink = configureSelfLink("Nic::selfLink", "1 ns",
        new Event::Handler<Nic>(this,&Nic::handleSelfEvent));
    assert( m_selfLink );

    m_linkControl->setNotifyOnReceive( m_recvNotifyFunctor );

    m_recvNotifyEnabled = true;

    m_dbg.verbose(CALL_INFO,1,0,"%d\n", IdToNet( m_myNodeId ) );

    m_num_vNics = params.find_integer("num_vNics", 1 );
    for ( int i = 0; i < m_num_vNics; i++ ) {
        m_vNicV.push_back( new VirtNic( *this, i,
			params.find_string("corePortName","core") ) );
    }
    m_recvM.resize( m_vNicV.size() );
}

Nic::~Nic()
{
	delete m_linkControl;

	if ( m_recvNotifyFunctor ) delete m_recvNotifyFunctor;
	if ( m_sendNotifyFunctor ) delete m_sendNotifyFunctor;

    for ( int i = 0; i < m_num_vNics; i++ ) {
        delete m_vNicV[i];
    }

    for ( unsigned i = 0; i < m_recvM.size(); i++ ) {
        std::map< int, std::deque<Entry*> >::iterator iter;

        for ( iter = m_recvM[i].begin(); iter != m_recvM[i].end(); ++iter ) {
            while ( ! (*iter).second.empty() ) {
                delete (*iter).second.front();
                (*iter).second.pop_front(); 
            }
        }
    }

    while ( ! m_activeRecvM.empty() ) {
        delete m_activeRecvM.begin()->second;
        m_activeRecvM.erase( m_activeRecvM.begin() );
    }

    while ( ! m_sendQ.empty() ) {
        delete m_sendQ.front();
        m_sendQ.pop_front();
    }

    if ( m_currentSend ) delete m_currentSend;
}

Nic::VirtNic::VirtNic( Nic& nic, int _id, std::string portName ) : 
    m_nic( nic ),
    id( _id )
{
    std::ostringstream tmp;
    tmp <<  id;

    m_toCoreLink = nic.configureLink( portName + tmp.str(), "1 ns", 
        new Event::Handler<Nic::VirtNic>(
                    this, &Nic::VirtNic::handleCoreEvent ) );
	assert( m_toCoreLink );
}

void Nic::VirtNic::init( unsigned int phase )
{
    if ( 0 == phase ) {
        m_toCoreLink->sendInitData( new NicInitEvent( 
                m_nic.getNodeId(), id, m_nic.getNum_vNics() ) );
    }
}

void Nic::VirtNic::handleCoreEvent( Event* ev )
{
    m_nic.handleVnicEvent( ev, id );
}

void Nic::VirtNic::notifyRecvDmaDone( int src_vNic, int src, int tag,
										size_t len, void* key )
{
    m_toCoreLink->send(0, 
        new NicRespEvent( NicRespEvent::DmaRecv, src_vNic, src, tag, len, key ) );
}

void Nic::VirtNic::notifyNeedRecv( int src_vNic, int src, int tag, size_t len )
{
    m_toCoreLink->send(0, 
            new NicRespEvent( NicRespEvent::NeedRecv, src_vNic, src, tag, len ) );
}

void Nic::VirtNic::notifySendDmaDone( void* key )
{
    m_toCoreLink->send(0, new NicRespEvent( NicRespEvent::DmaSend, key ));
}

void Nic::VirtNic::notifySendPioDone( void* key )
{
    m_toCoreLink->send(0, new NicRespEvent( NicRespEvent::PioSend, key ));
}

void Nic::init( unsigned int phase )
{
    m_dbg.verbose(CALL_INFO,1,0,"\n");
    if ( 0 == phase ) {
        for ( unsigned int i = 0; i < m_vNicV.size(); i++ ) {
            m_dbg.verbose(CALL_INFO,1,0,"sendInitdata to core %d\n", i );
            m_vNicV[i]->init( phase );
        } 
    } 
    m_linkControl->init(phase);
}

void Nic::handleVnicEvent( Event* ev, int id )
{
    NicCmdEvent* event = static_cast<NicCmdEvent*>(ev);

    m_dbg.verbose(CALL_INFO,3,0,"%d\n",event->type);

    switch ( event->type ) {
    case NicCmdEvent::DmaSend:
        dmaSend( event, id );
        break;
    case NicCmdEvent::DmaRecv:
        dmaRecv( event, id );
        break;
    case NicCmdEvent::PioSend:
        pioSend( event, id );
        break;
    default:
        assert(0);
    }
}

int Nic::IP_to_fattree_ID(int ip)
{
    union Addr {
        uint8_t x[4];
        int32_t s;
    };

    Addr addr;
    addr.s = ip;

    int id = 0;
    id += addr.x[1] * (m_ftRadix/2) * m_ftLoading;
    id += addr.x[2] * m_ftLoading;
    id += addr.x[3] -2;

    m_dbg.verbose(CALL_INFO,1,0,"ip=%#x -> id=%d\n", ip, id ); 
    return id;
}

int Nic::IdToNet( int v )
{
    if ( m_ftRadix ) {
        return fattree_ID_to_IP( v ); 
    } else {
        return v;
    }
}
int Nic::NetToId( int v )
{
    if ( m_ftRadix ) {
        return IP_to_fattree_ID( v );
    } else {
        return v;
    }
}

int Nic::fattree_ID_to_IP(int id)
{
    union Addr {
        uint8_t x[4];
        int32_t s;
    };

    Addr addr;

    int edge_switch = (id / m_ftLoading);
    int pod = edge_switch / (m_ftRadix/2);
    int subnet = edge_switch % (m_ftRadix/2);

    addr.x[0] = 10;
    addr.x[1] = pod;
    addr.x[2] = subnet;
    addr.x[3] = 2 + (id % m_ftLoading);

#if 1 
    m_dbg.verbose(CALL_INFO,1,0,"NIC id %d to %u.%u.%u.%u.\n",
                        id, addr.x[0], addr.x[1], addr.x[2], addr.x[3]);
#endif

    return addr.s;
}

void Nic::handleSelfEvent( Event *e )
{
    SelfEvent* event = static_cast<SelfEvent*>(e);

    switch ( event->type ) {
    case SelfEvent::MatchDelay:

        m_dbg.verbose(CALL_INFO,2,0,"MatchDelay\n");
        if ( event->retval ) {
            moveEvent( event->mEvent );
        } else {
            notifyNeedRecv( event->hdr.dst_vNicId, event->hdr.src_vNicId,
							 event->node, event->hdr.tag, event->hdr.len);
            m_pendingMerlinEvent = event->mEvent;
        }
        break;

    case SelfEvent::ProcessMerlinEvent:
        m_dbg.verbose(CALL_INFO,2,0,"ProcessMerlinEvent\n");
        processRecvEvent( event->mEvent );
        break;

    case SelfEvent::ProcessSend:
        m_dbg.verbose(CALL_INFO,2,0,"ProcessSend\n");
        processSend();
        break;
    }
    if ( e ) {
        delete e;
    }
}

void Nic::processSend( )
{
    m_dbg.verbose(CALL_INFO,2,0,"number pending %lu\n", m_sendQ.size() );

    if ( m_currentSend ) {
        if ( m_linkControl->spaceToSend(0,8) ) { 
            m_currentSend = processSend( m_currentSend );
        }
        if ( m_currentSend && ! m_linkControl->spaceToSend(0,8) ) { 
            m_dbg.verbose(CALL_INFO,2,0,"set send notify\n");
            m_linkControl->setNotifyOnSend( m_sendNotifyFunctor );
        }
    }
    if ( ! m_currentSend && ! m_sendQ.empty() ) {
        m_currentSend = m_sendQ.front();
        m_sendQ.pop_front();
        m_dbg.verbose(CALL_INFO,2,0,"new Send\n");
        SelfEvent* event = new SelfEvent;
        event->type = SelfEvent::ProcessSend;
        schedEvent( event, m_txDelay );
    }
    if ( ! m_currentSend ) {
        m_dbg.verbose(CALL_INFO,2,0,"remove send notify\n");
        m_linkControl->setNotifyOnSend( NULL );
    }
}

Nic::Entry* Nic::processSend( Entry* entry )
{
    bool ret = false;
    while ( m_linkControl->spaceToSend(0,8) && entry )  
    {
        NicCmdEvent* event = entry->event(); 
        MerlinFireflyEvent* ev = new MerlinFireflyEvent;

        if ( 0 == entry->currentVec && 
                0 == entry->currentPos  ) {
            
            MsgHdr hdr;
            hdr.tag = event->tag;
            hdr.len = entry->totalBytes();
            hdr.dst_vNicId = entry->event()->dst_vNic; 
            hdr.src_vNicId = entry->vNicNum(); 

            m_dbg.verbose(CALL_INFO,1,0,"src_vNic=%d, send dstNid=%d "
                    "dst_vNic=%d tag=%#x bytes=%lu\n",
                    hdr.src_vNicId,entry->node(), 
                    hdr.dst_vNicId, hdr.tag, entry->totalBytes()) ;

            ev->buf.insert( ev->buf.size(), (const char*) &hdr, 
                                       sizeof(hdr) );
        }
        ret = copyOut( m_dbg, *ev, *entry ); 

        print(m_dbg, &ev->buf[0], ev->buf.size() );
        ev->setDest( IdToNet( entry->node() ) );
        ev->setSrc( IdToNet( m_myNodeId ) );
        ev->setNumFlits( ev->buf.size(), m_bytesPerFlit );

        #if 0 
            ev->setTraceType( Merlin::RtrEvent::FULL );
            ev->setTraceID( m_packetId++ );
        #endif
        m_dbg.verbose(CALL_INFO,2,0,"sending event with %lu bytes\n",
                                                        ev->buf.size());
        assert( ev->buf.size() );
        bool sent = m_linkControl->send( ev, 0 );
        assert( sent );

        if ( ret ) {

            m_dbg.verbose(CALL_INFO,1,0,"send entry done\n");

            if ( NicCmdEvent::DmaSend == event->type )  {
                notifySendDmaDone( entry->vNicNum(), event->key );
            } else if ( NicCmdEvent::PioSend == event->type )  {
                notifySendPioDone( entry->vNicNum(), event->key );
            } else {
                assert(0);
            }
            delete event;

            delete entry;
            entry = NULL;
        }
    }
    return entry;
}

void Nic::dmaSend( NicCmdEvent *e, int vNicNum )
{
    Entry* entry = new Entry( vNicNum, e );
    m_dbg.verbose(CALL_INFO,1,0,"dest=%#x tag=%#x vecLen=%lu totalBytes=%lu\n",
                    e->node, e->tag, e->iovec.size(), entry->totalBytes() );
    
    m_sendQ.push_back( entry );
    processSend();
}

void Nic::pioSend( NicCmdEvent *e, int vNicNum )
{
    Entry* entry = new Entry( vNicNum, e );
    m_dbg.verbose(CALL_INFO,1,0,"src_vNic=%d dest=%#x dst_vNic=%d tag=%#x vecLen=%lu totalBytes=%lu\n", vNicNum,
             e->node, e->dst_vNic, e->tag, e->iovec.size(), entry->totalBytes() );
    m_sendQ.push_back( entry );
    processSend();
}

void Nic::dmaRecv( NicCmdEvent *e, int vNicNum )
{
    Entry* entry = new Entry( vNicNum, e );

    m_dbg.verbose(CALL_INFO,1,0,"vNicNum=%d src=%#x tag=%#x length=%lu\n",
                   vNicNum, e->node, e->tag, entry->totalBytes());

    m_recvM[ entry->vNicNum() ][ e->tag ].push_back( entry );

    if ( m_pendingMerlinEvent ) {
        SelfEvent* tmp = new SelfEvent;
        tmp->type = SelfEvent::ProcessMerlinEvent;
        tmp->mEvent = m_pendingMerlinEvent;
        m_pendingMerlinEvent = NULL;
        schedEvent( tmp );
    }
}

// Merlin stuff
bool Nic::sendNotify(int)
{
    m_dbg.verbose(CALL_INFO,2,0,"\n");
    
    SelfEvent* event = new SelfEvent;
    event->type = SelfEvent::ProcessSend;
    schedEvent( event );

    // remove notifier
    return false;
}

bool Nic::recvNotify(int vc)
{
    m_dbg.verbose(CALL_INFO,2,0,"network event available\n");

    MerlinFireflyEvent* event = 
            static_cast<MerlinFireflyEvent*>( m_linkControl->recv(vc) );

    event->src = NetToId( event->src ); 

    for ( int i = 0; i < m_num_vcs; i++ ) {
        assert( ! m_linkControl->eventToReceive( i ) );
    }
     
    m_recvNotifyEnabled = processRecvEvent( event );
    if ( ! m_recvNotifyEnabled ) {
        m_dbg.verbose(CALL_INFO,2,0,"remove recv notify\n");
    }
    return m_recvNotifyEnabled;
}

bool Nic::processRecvEvent( MerlinFireflyEvent* event )
{
    if ( m_activeRecvM.find( event->src ) == m_activeRecvM.end() ) {
        SelfEvent* selfEvent = new SelfEvent;
        selfEvent->type = SelfEvent::MatchDelay;
        selfEvent->mEvent = event;

        selfEvent->retval = findRecv( event, selfEvent->hdr ); 

        selfEvent->node = event->src;

        schedEvent( selfEvent, m_rxMatchDelay );

        // remove recvNotifier until delay is done 
        return false;
    } else {
        moveEvent( event );
        return true;
    }
}

bool Nic::findRecv( MerlinFireflyEvent* event, MsgHdr& hdr )
{
    int src = event->src;
    memcpy( &hdr, &event->buf[0], sizeof(hdr) );

    m_dbg.verbose(CALL_INFO,2,0,"need a recv entry, src=%d src_vNic=%d "
                "dst_vNic=%d tag=%#x len=%lu\n", src, hdr.src_vNicId,
						hdr.dst_vNicId, hdr.tag, hdr.len);

    if ( m_recvM[hdr.dst_vNicId].find( hdr.tag ) == m_recvM[hdr.dst_vNicId].end() ) {
		m_dbg.verbose(CALL_INFO,2,0,"did't match tag\n");
        return false;
    }

    Entry* entry = m_recvM[ hdr.dst_vNicId][ hdr.tag ].front();
    if ( entry->event()->node != -1 && entry->event()->node != src ) {
		m_dbg.verbose(CALL_INFO,2,0,"didn't match node  want=%#x src=%#x\n",
											entry->event()->node, src );
        return false;
    } 
    m_dbg.verbose(CALL_INFO,2,0,"recv entry size %lu\n",entry->totalBytes());

    if ( entry->totalBytes() < hdr.len ) {
        assert(0);
    }

    m_dbg.verbose(CALL_INFO,2,0,"found a receive entry\n");

    event->buf.erase(0, sizeof(hdr) );

    m_activeRecvM[src] = m_recvM[ hdr.dst_vNicId ][ hdr.tag ].front();
    m_recvM[ hdr.dst_vNicId ][ hdr.tag ].pop_front();

    if ( m_recvM[ hdr.dst_vNicId ][ hdr.tag ].empty() ) {
        m_recvM[ hdr.dst_vNicId ].erase( hdr.tag );
    }
    m_activeRecvM[src]->len    = hdr.len;
    m_activeRecvM[src]->src    = src;
    m_activeRecvM[src]->tag    = hdr.tag;
    m_activeRecvM[src]->set_src_vNicNum( hdr.src_vNicId );
    return true;
}

void Nic::moveEvent( MerlinFireflyEvent* event )
{
    int src = event->src;

    if ( 0 == m_activeRecvM[ src ]->currentVec && 
             0 == m_activeRecvM[ src ]->currentPos  ) {
        m_dbg.verbose(CALL_INFO,1,0,"dst_vNic %d, recv srcNid=%d "
                    "src_vNic=%d tag=%x bytes=%lu\n", 
                        m_activeRecvM[ src ]->vNicNum(),
                        src,
                        m_activeRecvM[ src ]->src_vNicNum(),
                        m_activeRecvM[ src ]->tag,
                        m_activeRecvM[ src ]->len );
    }
    if ( copyIn( m_dbg, *m_activeRecvM[ src ], *event ) || 
        m_activeRecvM[src]->len == 
                        m_activeRecvM[src]->currentLen ) {
        m_dbg.verbose(CALL_INFO,1,0,"recv entry done, srcNid=%d\n",src );
        notifyRecvDmaDone( m_activeRecvM[src]->vNicNum(),
                            m_activeRecvM[src]->src_vNicNum(),
                            m_activeRecvM[src]->src,
                            m_activeRecvM[src]->tag,
                            m_activeRecvM[src]->len,
                            m_activeRecvM[src]->event()->key );
        
        delete m_activeRecvM[src]->event();
        delete m_activeRecvM[src];
        m_activeRecvM.erase(src);

        assert( event->buf.empty() );
    }

    if ( event->buf.empty() ) {
        m_dbg.verbose(CALL_INFO,2,0,"done with network event\n" );
        delete event;
        if ( ! m_recvNotifyEnabled ) {
            MerlinFireflyEvent* event = NULL;
            // this is not fair
            for ( int i = 0; i < m_num_vcs; i++ ) {
                event = static_cast<MerlinFireflyEvent*>(m_linkControl->recv(i));
                if ( event ) break;
            }
            if ( event ) {
                event->src = NetToId( event->src ); 
                SelfEvent* tmp = new SelfEvent;
                tmp->type = SelfEvent::ProcessMerlinEvent;
                tmp->mEvent = event;
                schedEvent( tmp );
            } else {
                m_linkControl->setNotifyOnReceive( m_recvNotifyFunctor );
            }
        }
    }
}

static void print( Output& dbg, char* buf, int len )
{
    std::string tmp;
    dbg.verbose(CALL_INFO,3,0,"addr=%p len=%d\n",buf,len);
    for ( int i = 0; i < len; i++ ) {
        dbg.verbose(CALL_INFO,3,0,"%#03x\n",(unsigned char)buf[i]);
    }
}

size_t Nic::copyIn( Output& dbg, Nic::Entry& entry, MerlinFireflyEvent& event )
{
    dbg.verbose(CALL_INFO,2,0,"ioVec.size()=%lu\n", entry.ioVec().size() );


    for ( ; entry.currentVec < entry.ioVec().size();
                entry.currentVec++, entry.currentPos = 0 ) {

        if ( entry.ioVec()[entry.currentVec].len ) {
            size_t toLen = entry.ioVec()[entry.currentVec].len - entry.currentPos;
            size_t fromLen = event.buf.size();
            size_t len = toLen < fromLen ? toLen : fromLen;

            char* toPtr = (char*) entry.ioVec()[entry.currentVec].ptr +
                                                        entry.currentPos;
            dbg.verbose(CALL_INFO,2,0,"toBufSpace=%lu fromAvail=%lu, "
                            "memcpy len=%lu\n", toLen,fromLen,len);


            entry.currentLen += len;
            memcpy( toPtr, &event.buf[0], len );
            event.buf.erase(0,len);
            print( dbg, toPtr, len );

            entry.currentPos += len;
            if ( 0 == event.buf.size() &&
                    entry.currentPos != entry.ioVec()[entry.currentVec].len )
            {
                dbg.verbose(CALL_INFO,2,0,"event buffer empty\n");
                break;
            }
        }
    }

    dbg.verbose(CALL_INFO,2,0,"currentVec=%lu, currentPos=%lu\n",
                entry.currentVec, entry.currentPos);
    return ( entry.currentVec == entry.ioVec().size() ) ;
}

bool  Nic::copyOut( Output& dbg, MerlinFireflyEvent& event, Nic::Entry& entry )
{
    dbg.verbose(CALL_INFO,2,0,"ioVec.size()=%lu\n", entry.ioVec().size() );

    for ( ; entry.currentVec < entry.ioVec().size() &&
                event.buf.size() <  event.getMaxLength();
                entry.currentVec++, entry.currentPos = 0 ) {

        dbg.verbose(CALL_INFO,2,0,"vec[%lu].len %lu\n",entry.currentVec,
                    entry.ioVec()[entry.currentVec].len );

        if ( entry.ioVec()[entry.currentVec].len ) {
            size_t toLen = event.getMaxLength() - event.buf.size();
            size_t fromLen = entry.ioVec()[entry.currentVec].len -
                                                        entry.currentPos;

            size_t len = toLen < fromLen ? toLen : fromLen;

            dbg.verbose(CALL_INFO,2,0,"toBufSpace=%lu fromAvail=%lu, "
                            "memcpy len=%lu\n", toLen,fromLen,len);

            const char* from = 
                    (const char*) entry.ioVec()[entry.currentVec].ptr + 
                                                        entry.currentPos;

            event.buf.insert( event.buf.size(), from, len );
            entry.currentPos += len;
            if ( event.buf.size() == event.getMaxLength() &&
                    entry.currentPos != entry.ioVec()[entry.currentVec].len ) {
                break;
            }
        }
    }
    dbg.verbose(CALL_INFO,2,0,"currentVec=%lu, currentPos=%lu\n",
                entry.currentVec, entry.currentPos);
    return ( entry.currentVec == entry.ioVec().size() ) ;
}

BOOST_CLASS_EXPORT( NicInitEvent )
