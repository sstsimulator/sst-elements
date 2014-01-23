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
    m_txBusDelay( 150 ),
    m_pendingMerlinEvent( NULL )
{
    //params.print_all_params( std::cout );

    m_myNodeId = params.find_integer("nid", -1);
    assert( m_myNodeId != -1 );

    m_dbg.verbose(CALL_INFO,1,0,"\n");
    char buffer[100];
    snprintf(buffer,100,"@t:%d:Nic::@p():@l ",m_myNodeId);

    m_dbg.init(buffer, params.find_integer("verboseLevel",0),0,
       (Output::output_location_t)params.find_integer("debug", 0));

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

    m_selfLink = comp->configureSelfLink("Nic::selfLink", "1 ps",
        new Event::Handler<Nic>(this,&Nic::handleSelfEvent));
    assert( m_selfLink );

    m_linkControl->setNotifyOnReceive( m_recvNotifyFunctor );
}

void Nic::init( unsigned int phase )
{
    m_linkControl->init(phase);
}

Nic::VirtNic* Nic::virtNicInit()
{
    m_vNicV.push_back( new Nic::VirtNic( *this, m_vNicV.size() ) );
    return m_vNicV.back();
}

void Nic::handleSelfEvent( Event *e )
{
    SelfEvent* event = static_cast<SelfEvent*>(e);
    m_dbg.verbose(CALL_INFO,1,0,"type %d\n", event->type);
    switch ( event->type ) {
    case SelfEvent::DmaSend:
        dmaSend(event);
        break;
    case SelfEvent::DmaRecv:
        dmaRecv(event);
        break;
    case SelfEvent::PioSend:
        pioSend(event);
        break;
    }
}

void Nic::dmaSend( Nic::VirtNic *vNic,int dest, int tag, 
                                std::vector<IoVec>& iovec, XXX* key )
{
    m_dbg.verbose(CALL_INFO,1,0,"\n");
    SelfEvent* event = new SelfEvent; 
    event->type = SelfEvent::DmaSend;
    event->node = dest;
    event->tag = tag;
    event->iovec = iovec;
    event->key = key;
    event->vNic = vNic;

    m_selfLink->send( m_txBusDelay, event );
}

void Nic::dmaRecv( Nic::VirtNic* vNic, int src, int tag,
                                std::vector<IoVec>& iovec, XXX* key )
{
    m_dbg.verbose(CALL_INFO,1,0,"\n");
    SelfEvent* event = new SelfEvent; 
    event->type = SelfEvent::DmaRecv;
    event->node = src;
    event->tag = tag;
    event->iovec = iovec;
    event->key = key;
    event->vNic = vNic;

    m_selfLink->send( m_txBusDelay, event );
}

void Nic::pioSend( Nic::VirtNic* vNic, int dest, int tag,
                                std::vector<IoVec>& iovec, XXX* key )
{
    m_dbg.verbose(CALL_INFO,1,0,"\n");
    SelfEvent* event = new SelfEvent; 
    event->type = SelfEvent::PioSend;
    event->node = dest;
    event->tag = tag;
    event->iovec = iovec;
    event->key = key;
    event->vNic = vNic;

    m_selfLink->send( m_txBusDelay, event );
}

bool Nic::processSend( )
{
    bool ret = false;
    m_dbg.verbose(CALL_INFO,1,0,"number pending %lu\n", m_sendQ.size() );

    while ( ! m_sendQ.empty() && m_linkControl->spaceToSend(0,8) )  
    {
        SelfEvent* event = m_sendQ.front()->event(); 
        MerlinFireflyEvent* ev = new MerlinFireflyEvent;

        if ( 0 == m_sendQ.front()->currentVec && 
                0 == m_sendQ.front()->currentPos  ) {
            
            MsgHdr hdr;
            hdr.tag = event->tag;
            hdr.len = m_sendQ.front()->totalBytes();
            hdr.vNicId = m_sendQ.front()->event()->vNic->id(); 

            ev->buf.insert( ev->buf.size(), (const char*) &hdr, 
                                       sizeof(hdr) );
        }
        ret = copyOut( m_dbg, *ev, *m_sendQ.front() ); 

        print(m_dbg, &ev->buf[0], ev->buf.size() );
        ev->setDest( m_sendQ.front()->node() );
        ev->setSrc( m_myNodeId );
        ev->setNumFlits( ev->buf.size() );

        #if 0 
            ev->setTraceType( Merlin::RtrEvent::ROUTE );
        #endif
        m_dbg.verbose(CALL_INFO,2,0,"sending event with %lu bytes %p\n",
                                                        ev->buf.size(), ev);
        assert( ev->buf.size() );
        bool sent = m_linkControl->send( ev, 0 );
        assert( sent );

        if ( ret ) {

            m_dbg.verbose(CALL_INFO,1,0,"send entry done\n");
            delete m_sendQ.front();
            m_sendQ.pop_front();

            if ( SelfEvent::DmaSend == event->type )  {
                event->vNic->notifySendDmaDone( event->key );
            } else if ( SelfEvent::PioSend == event->type )  {
                event->vNic->notifySendPioDone(event->key );
            }
            delete event;
        }
    }
    return ! m_sendQ.empty();
}

void Nic::dmaSend( SelfEvent *e )
{
    Entry* entry = new Entry(e);
    m_dbg.verbose(CALL_INFO,1,0,"dest=%d tag=%d vecLen=%lu length=%lu\n",
                    e->node, e->tag, e->iovec.size(), entry->totalBytes() );
    
    m_sendQ.push_back( entry );
    if ( processSend() ) {
        m_dbg.verbose(CALL_INFO,1,0,"set send notify\n");
        m_linkControl->setNotifyOnSend( m_sendNotifyFunctor );
    }
}

void Nic::pioSend( SelfEvent *e )
{
    Entry* entry = new Entry(e);
    m_dbg.verbose(CALL_INFO,1,0,"dest=%d tag=%d vecLen=%lu length=%lu\n",
                    e->node, e->tag, e->iovec.size(), entry->totalBytes() );
    m_sendQ.push_back( entry );
    if ( processSend() ) {
        m_dbg.verbose(CALL_INFO,1,0,"set send notify\n");
        m_linkControl->setNotifyOnSend( m_sendNotifyFunctor );
    }
}

void Nic::dmaRecv( SelfEvent *e )
{
    m_dbg.verbose(CALL_INFO,1,0,"src=%d tag=%#x\n",e->node, e->tag);
    
    m_recvM[ e->tag ].push_back( new Entry( e ) );

    if ( m_pendingMerlinEvent ) {
        MerlinFireflyEvent* tmp = m_pendingMerlinEvent;
        m_pendingMerlinEvent = NULL;
        if ( processRecvEvent( tmp ) ) {
            m_linkControl->setNotifyOnSend( m_recvNotifyFunctor );
        }
    }
}

// Merlin stuff
bool Nic::sendNotify(int)
{
    m_dbg.verbose(CALL_INFO,1,0,"\n");
    return processSend();
}

bool Nic::recvNotify(int vc)
{
    m_dbg.verbose(CALL_INFO,1,0,"\n");

    MerlinFireflyEvent* event = 
            static_cast<MerlinFireflyEvent*>( m_linkControl->recv(vc) );

    for ( int i = 0; i < m_num_vcs; i++ ) {
        assert( ! m_linkControl->eventToReceive( i ) );
    }
    
    return processRecvEvent( event );
}

bool Nic::processRecvEvent( MerlinFireflyEvent* event )
{
    int src = event->src;

    if ( m_activeRecvM.find( src ) == m_activeRecvM.end() ) {

        MsgHdr hdr;
        memcpy( &hdr, &event->buf[0], sizeof(hdr) );

        m_dbg.verbose(CALL_INFO,1,0,"need a recv entry, src=%d tag=%#x "
                                    "len=%lu\n", src, hdr.tag, hdr.len);

        if ( m_recvM.find( hdr.tag ) == m_recvM.end() ) {
            m_vNicV[ hdr.vNicId ]->notifyNeedRecv( NULL );
            m_pendingMerlinEvent = event;
            // unregister notifier
            return false;
        }

        Entry* entry = m_recvM[ hdr.tag ].front();
        if ( entry->event()->node != -1 && entry->event()->node != src ) {
            m_vNicV[ hdr.vNicId ]->notifyNeedRecv( NULL );
            m_pendingMerlinEvent = event;
            // unregister notifier
        } 

        if ( entry->totalBytes() < hdr.len ) {
            assert(0);
        }

        event->buf.erase(0, sizeof(hdr) );

        m_activeRecvM[src] = m_recvM[ hdr.tag ].front();
        m_recvM[ hdr.tag ].pop_front();

        m_dbg.verbose(CALL_INFO,1,0,"found a receive entry\n");
        if ( m_recvM[ hdr.tag ].empty() ) {
            m_recvM.erase( hdr.tag );
        }
        m_activeRecvM[src]->event()->key->len = hdr.len;
        m_activeRecvM[src]->event()->key->src = src;
    }

    if ( copyIn( m_dbg, *m_activeRecvM[ src ], *event ) || 
        m_activeRecvM[src]->event()->key->len == 
                        m_activeRecvM[src]->currentLen ) {
        m_dbg.verbose(CALL_INFO,1,0,"recv entry done, src=%d\n",src );
        m_activeRecvM[src]->event()->vNic->notifyRecvDmaDone( 
                            m_activeRecvM[src]->event()->key );
        
        delete m_activeRecvM[src]->event();
        delete m_activeRecvM[src];
        m_activeRecvM.erase(src);

        assert( event->buf.empty() );
    }

    if ( event->buf.empty() ) {
        delete event;
    }

    return true;
}


static void print( Output& dbg, char* buf, int len )
{
    std::string tmp;
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

            print( dbg, &event.buf[0], len );

            entry.currentLen += len;
            memcpy( toPtr, &event.buf[0], len );
            event.buf.erase(0,len);

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

