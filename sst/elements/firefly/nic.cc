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


#include "sst_config.h"
#include <sst/core/component.h>
#include <sst/core/params.h>
#include <sst/core/timeLord.h>
#include <sst/core/element.h>

#include "nic.h"
#include "merlinEvent.h"

using namespace SST;
using namespace SST::Firefly;

static size_t
copyIn( Output& dbg, Nic::Entry& entry, MerlinFireflyEvent& event );

static bool copyOut( Output&, MerlinFireflyEvent&, Nic::Entry& );
static void print( Output& dbg, char* buf, int len );

Nic::Nic(Component* comp, Params &params) :
    m_currentSend( NULL ),
    m_txBusDelay( 150 ),
    m_rxBusDelay( 150 ),
    m_rxMatchDelay( 100 ),
    m_txDelay( 50 ),
    m_pendingMerlinEvent( NULL ),
    m_recvNotifyEnabled( false ),
    m_packetId(0)
{
    //params.print_all_params( std::cout );

    m_myNodeId = params.find_integer("nid", -1);
    assert( m_myNodeId != -1 );

    m_dbg.verbose(CALL_INFO,1,0,"\n");
    char buffer[100];
    snprintf(buffer,100,"@t:%d:Nic::@p():@l ",m_myNodeId);

    m_dbg.init(buffer, params.find_integer("verboseLevel",0),0,
       (Output::output_location_t)params.find_integer("debug", 0));


    m_txBusDelay =   params.find_integer( "txBusDelay_ns", 150 );
    m_rxBusDelay =   params.find_integer( "rxBusDelay_ns", 150 );
    m_rxMatchDelay = params.find_integer( "rxMatchDelay_ns", 100 );
    m_txDelay =      params.find_integer( "txDelay_ns", 50 );

    m_num_vcs = params.find_integer("num_vcs",2);
    std::string link_bw = params.find_string("link_bw","2GHz");
    TimeConverter* tc = Simulation::getSimulation()->
                            getTimeLord()->getTimeConverter(link_bw);
    assert( tc );

    int buffer_size = params.find_integer("buffer_size",2048);

    m_dbg.verbose(CALL_INFO,1,0,"id=%d num_vcs=%d buffer_size=%d link_bw=%s\n",
                m_myNodeId, m_num_vcs, buffer_size, link_bw.c_str());

    std::vector<int> buf_size;
    buf_size.resize(m_num_vcs);
    for ( unsigned int i = 0; i < buf_size.size(); i++ ) {
        buf_size[i] = buffer_size;
    }

    m_linkControl = (Merlin::LinkControl*)comp->loadModule(
                    params.find_string("module"), params);
    assert( m_linkControl );

    m_linkControl->configureLink(comp, "rtr", tc, m_num_vcs,
                        &buf_size[0], &buf_size[0]);

    m_recvNotifyFunctor =
        new Merlin::LinkControl::Handler<Nic>(this,&Nic::recvNotify );
    assert( m_recvNotifyFunctor );

    m_sendNotifyFunctor =
        new Merlin::LinkControl::Handler<Nic>(this,&Nic::sendNotify );
    assert( m_sendNotifyFunctor );

    m_selfLink = comp->configureSelfLink("Nic::selfLink", "1 ns",
        new Event::Handler<Nic>(this,&Nic::handleSelfEvent));
    assert( m_selfLink );

    m_linkControl->setNotifyOnReceive( m_recvNotifyFunctor );

    m_recvNotifyEnabled = true;
}

Nic::VirtNic* Nic::virtNicInit()
{
    m_vNicV.push_back( new Nic::VirtNic( *this, m_vNicV.size() ) );
    m_recvM.resize( m_vNicV.size() );
    return m_vNicV.back();
}

void Nic::handleSelfEvent( Event *e )
{
    SelfEvent* event = static_cast<SelfEvent*>(e);
    m_dbg.verbose(CALL_INFO,1,0,"type %d\n", event->type);
    switch ( event->type ) {
    case SelfEvent::DmaSend:
        e = NULL;
        dmaSend(event);
        break;
    case SelfEvent::DmaRecv:
        e = NULL;
        dmaRecv(event);
        break;
    case SelfEvent::PioSend:
        e = NULL;
        pioSend(event);
        break;
    case SelfEvent::DmaSendFini:
        event->vNic->notifySendDmaDone( event->key );
        break;
    case SelfEvent::DmaRecvFini:
        event->vNic->notifyRecvDmaDone( event->node, event->tag, event->len, event->key );
        break;
    case SelfEvent::PioSendFini:
        event->vNic->notifySendPioDone( event->key );
        break;
    case SelfEvent::NeedRecvFini:
        event->vNic->notifyNeedRecv( event->node, event->tag, event->len );
        break;
    case SelfEvent::MatchDelay:

        m_dbg.verbose(CALL_INFO,1,0,"\n");
        if ( event->retval ) {
            moveEvent( event->mEvent );
        } else {
            notifyNeedRecv( event->vNic, event->node, event->tag, event->len );
            m_pendingMerlinEvent = event->mEvent;
        }
        break;
    case SelfEvent::ProcessMerlinEvent:
        processRecvEvent( event->mEvent );
        break;
    case SelfEvent::ProcessSend:
        processSend();
        break;
    }
    if ( e ) {
        delete e;
    }
}

void Nic::dmaSend( Nic::VirtNic *vNic,int dest, int tag, 
                                std::vector<IoVec>& iovec, void* key )
{
    m_dbg.verbose(CALL_INFO,1,0,"\n");
    SelfEvent* event = new SelfEvent; 
    event->type = SelfEvent::DmaSend;
    event->node = dest;
    event->tag = tag;
    event->iovec = iovec;
    event->key = key;
    event->vNic = vNic;

    schedEvent( event, m_txBusDelay );
}

void Nic::dmaRecv( Nic::VirtNic* vNic, int src, int tag,
                                std::vector<IoVec>& iovec, void* key )
{
    m_dbg.verbose(CALL_INFO,1,0,"\n");
    SelfEvent* event = new SelfEvent; 
    event->type = SelfEvent::DmaRecv;
    event->node = src;
    event->tag = tag;
    event->iovec = iovec;
    event->key = key;
    event->vNic = vNic;

    schedEvent( event, m_txBusDelay );
}

void Nic::pioSend( Nic::VirtNic* vNic, int dest, int tag,
                                std::vector<IoVec>& iovec, void* key )
{
    m_dbg.verbose(CALL_INFO,1,0,"\n");
    SelfEvent* event = new SelfEvent; 
    event->type = SelfEvent::PioSend;
    event->node = dest;
    event->tag = tag;
    event->iovec = iovec;
    event->key = key;
    event->vNic = vNic;

    schedEvent( event, m_txBusDelay );
}

void Nic::processSend( )
{
    m_dbg.verbose(CALL_INFO,1,0,"number pending %lu\n", m_sendQ.size() );

    if ( m_currentSend ) {
        if ( m_linkControl->spaceToSend(0,8) ) { 
            m_currentSend = processSend( m_currentSend );
        }
        if ( m_currentSend && ! m_linkControl->spaceToSend(0,8) ) { 
            m_dbg.verbose(CALL_INFO,1,0,"set send notify\n");
            m_linkControl->setNotifyOnSend( m_sendNotifyFunctor );
        }
    } else if ( ! m_sendQ.empty() ) {
        m_currentSend = m_sendQ.front();
        m_sendQ.pop_front();
        SelfEvent* event = new SelfEvent;
        event->type = SelfEvent::ProcessSend;
        schedEvent( event, m_txDelay );
    } else {
        m_dbg.verbose(CALL_INFO,1,0,"remove send notify\n");
        m_linkControl->setNotifyOnSend( NULL );
    }
}

Nic::Entry* Nic::processSend( Entry* entry )
{
    bool ret = false;
    while ( m_linkControl->spaceToSend(0,8) && entry )  
    {
        SelfEvent* event = entry->event(); 
        MerlinFireflyEvent* ev = new MerlinFireflyEvent;

        if ( 0 == entry->currentVec && 
                0 == entry->currentPos  ) {
            
            MsgHdr hdr;
            hdr.tag = event->tag;
            hdr.len = entry->totalBytes();
            hdr.vNicId = entry->event()->vNic->id(); 

            ev->buf.insert( ev->buf.size(), (const char*) &hdr, 
                                       sizeof(hdr) );
        }
        ret = copyOut( m_dbg, *ev, *entry ); 

        print(m_dbg, &ev->buf[0], ev->buf.size() );
        ev->setDest( entry->node() );
        ev->setSrc( m_myNodeId );
        ev->setNumFlits( ev->buf.size() );

        #if 0 
            ev->setTraceType( Merlin::RtrEvent::ROUTE );
            ev->setTraceID( m_packetId++ );
        #endif
        m_dbg.verbose(CALL_INFO,2,0,"sending event with %lu bytes %p\n",
                                                        ev->buf.size(), ev);
        assert( ev->buf.size() );
        bool sent = m_linkControl->send( ev, 0 );
        assert( sent );

        if ( ret ) {

            m_dbg.verbose(CALL_INFO,1,0,"send entry done\n");

            if ( SelfEvent::DmaSend == event->type )  {
                notifySendDmaDone( event->vNic, event->key );
            } else if ( SelfEvent::PioSend == event->type )  {
                notifySendPioDone( event->vNic, event->key );
            }
            delete event;

            delete entry;
            entry = NULL;
        }
    }
    return entry;
}

void Nic::dmaSend( SelfEvent *e )
{
    Entry* entry = new Entry(e);
    m_dbg.verbose(CALL_INFO,1,0,"dest=%d tag=%d vecLen=%lu length=%lu\n",
                    e->node, e->tag, e->iovec.size(), entry->totalBytes() );
    
    m_sendQ.push_back( entry );
    processSend();
}

void Nic::pioSend( SelfEvent *e )
{
    Entry* entry = new Entry(e);
    m_dbg.verbose(CALL_INFO,1,0,"dest=%d tag=%d vecLen=%lu length=%lu\n",
                    e->node, e->tag, e->iovec.size(), entry->totalBytes() );
    m_sendQ.push_back( entry );
    processSend();
}

void Nic::dmaRecv( SelfEvent *e )
{
    
    Entry* entry = new Entry( e );

    m_recvM[ e->vNic->id() ][ e->tag ].push_back( entry );

    m_dbg.verbose(CALL_INFO,1,0,"src=%d tag=%#x length=%lu\n",
                            e->node, e->tag, entry->totalBytes());

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
    m_dbg.verbose(CALL_INFO,1,0,"\n");
    
    SelfEvent* event = new SelfEvent;
    event->type = SelfEvent::ProcessSend;
    schedEvent( event );

    // remove notifier
    return false;
}

bool Nic::recvNotify(int vc)
{
    m_dbg.verbose(CALL_INFO,1,0,"\n");

    MerlinFireflyEvent* event = 
            static_cast<MerlinFireflyEvent*>( m_linkControl->recv(vc) );

    for ( int i = 0; i < m_num_vcs; i++ ) {
        assert( ! m_linkControl->eventToReceive( i ) );
    }
     
    m_recvNotifyEnabled = processRecvEvent( event );
    if ( ! m_recvNotifyEnabled ) {
        m_dbg.verbose(CALL_INFO,1,0,"remove recv notify\n");
    }
    return m_recvNotifyEnabled;
}

bool Nic::processRecvEvent( MerlinFireflyEvent* event )
{
    if ( m_activeRecvM.find( event->src ) == m_activeRecvM.end() ) {
        SelfEvent* selfEvent = new SelfEvent;
        selfEvent->type = SelfEvent::MatchDelay;
        selfEvent->mEvent = event;
        selfEvent->retval =findRecv( event ); 

        MsgHdr hdr;
        memcpy( &hdr, &event->buf[0], sizeof(hdr) );
        selfEvent->node = event->src;
        selfEvent->tag = hdr.tag;
        selfEvent->len = hdr.len;
        selfEvent->vNic = m_vNicV[hdr.vNicId];  

        schedEvent( selfEvent, m_rxMatchDelay );

        // remove recvNotifier until delay is done 
        return false;
    } else {
        moveEvent( event );
        return true;
    }
}

bool Nic::findRecv( MerlinFireflyEvent* event )
{
    MsgHdr hdr;
    int src = event->src;
    memcpy( &hdr, &event->buf[0], sizeof(hdr) );

    m_dbg.verbose(CALL_INFO,1,0,"need a recv entry, src=%d tag=%#x "
                                "len=%lu\n", src, hdr.tag, hdr.len);

    if ( m_recvM[hdr.vNicId].find( hdr.tag ) == m_recvM[hdr.vNicId].end() ) {
        return false;
    }

    Entry* entry = m_recvM[ hdr.vNicId][ hdr.tag ].front();
    if ( entry->event()->node != -1 && entry->event()->node != src ) {
        return false;
    } 
    m_dbg.verbose(CALL_INFO,1,0,"entry nbytes %lu\n",entry->totalBytes());

    if ( entry->totalBytes() < hdr.len ) {
        assert(0);
    }

    m_dbg.verbose(CALL_INFO,1,0,"found a receive entry\n");

    event->buf.erase(0, sizeof(hdr) );

    m_activeRecvM[src] = m_recvM[ hdr.vNicId ][ hdr.tag ].front();
    m_recvM[ hdr.vNicId ][ hdr.tag ].pop_front();

    if ( m_recvM[ hdr.vNicId ][ hdr.tag ].empty() ) {
        m_recvM[ hdr.vNicId ].erase( hdr.tag );
    }
    m_activeRecvM[src]->len    = hdr.len;
    m_activeRecvM[src]->src    = src;
    m_activeRecvM[src]->tag    = hdr.tag;
    return true;
}

void Nic::moveEvent( MerlinFireflyEvent* event )
{
    m_dbg.verbose(CALL_INFO,1,0,"\n" );
    int src = event->src;
    if ( copyIn( m_dbg, *m_activeRecvM[ src ], *event ) || 
        m_activeRecvM[src]->len == 
                        m_activeRecvM[src]->currentLen ) {
        m_dbg.verbose(CALL_INFO,1,0,"recv entry done, src=%d\n",src );
        notifyRecvDmaDone( m_activeRecvM[src]->event()->vNic,
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
        delete event;
        if ( ! m_recvNotifyEnabled ) {
            MerlinFireflyEvent* event = NULL;
            // this is not fair
            for ( int i = 0; i < m_num_vcs; i++ ) {
                event = static_cast<MerlinFireflyEvent*>(m_linkControl->recv(i));
                if ( event ) break;
            }
            if ( event ) {
                SelfEvent* tmp = new SelfEvent;
                tmp->type = SelfEvent::ProcessMerlinEvent;
                tmp->mEvent = event;
                schedEvent( tmp );
            } else {
                m_dbg.verbose(CALL_INFO,1,0,"\n");
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

static size_t
copyIn( Output& dbg, Nic::Entry& entry, MerlinFireflyEvent& event )
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

static bool
copyOut( Output& dbg, MerlinFireflyEvent& event, Nic::Entry& entry )
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

