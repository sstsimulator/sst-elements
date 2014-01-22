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

#include <sst_config.h>
#include <sst/core/component.h>
#include <sst/core/timeLord.h>
#include <sst/core/module.h>

#include "sst/elements/merlin/linkControl.h"

#include "merlinIO.h"
#include "merlinEvent.h"

using namespace SST::Firefly;

static size_t 
copyIn( Output& dbg, MerlinIO::Entry& entry, MerlinFireflyEvent& event );

static bool
copyOut( Output& dbg, MerlinFireflyEvent& event, MerlinIO::Entry& entry );

MerlinIO::MerlinIO( Component* owner, Params& params ) :
    Interface(),
    m_leaveLink( NULL ),
    m_sendEntry( NULL ),
    m_myNodeId( IO::AnyId ),
    m_numVC( 2 ),
    m_lastVC( 0 ),
    m_haveCtrl( false )
{
    m_myNodeId = params.find_integer("nid", IO::AnyId);
    assert( m_myNodeId != IO::AnyId );

    char buffer[100];
    snprintf(buffer,100,"@t:%d:MerlinIO::@p():@l ",m_myNodeId);

    m_dbg.init(buffer, params.find_integer("verboseLevel",0),0,
       (Output::output_location_t)params.find_integer("debug", 0));

    int num_vcs = params.find_integer("num_vcs",2);
    std::string link_bw = params.find_string("link_bw","2GHz");
    TimeConverter* tc = Simulation::getSimulation()->
                            getTimeLord()->getTimeConverter(link_bw);
    assert( tc );

    m_dbg.verbose(CALL_INFO,1,0,"id=%d num_vcs=%d link_bw=%s\n",
                m_myNodeId, num_vcs, link_bw.c_str());

    int buffer_size = params.find_integer("buffer_size",2048);

    std::vector<int> buf_size;
    buf_size.resize(m_numVC);
    for ( unsigned int i = 0; i < buf_size.size(); i++ ) {
        buf_size[i] = buffer_size;
    }

    m_linkControl = (Merlin::LinkControl*)owner->loadModule( 
                    params.find_string("module"), params);

    m_dbg.verbose(CALL_INFO,1,0,"%d\n",buffer_size);
    
    m_linkControl->configureLink(owner, "rtr", tc, num_vcs,
                        &buf_size[0], &buf_size[0]);

    m_recvNotifyFunctor = 
        new Merlin::LinkControl::Handler<MerlinIO>(this,&MerlinIO::recvNotify );

    m_sendNotifyFunctor = 
        new Merlin::LinkControl::Handler<MerlinIO>(this,&MerlinIO::sendNotify );

    m_selfLink = owner->configureSelfLink("MerlinIOselfLink", "1 ps",
        new Event::Handler<MerlinIO>(this,&MerlinIO::selfHandler));
}

static inline  MerlinFireflyEvent* cast( Event* e )
{
    return static_cast<MerlinFireflyEvent*>(e);
}

bool MerlinIO::recvNotify( int vc )
{
    m_dbg.verbose(CALL_INFO,2,0,"vc=%d\n", vc );

    m_recvEventQ.push_back( m_linkControl->recv(vc) );

    if ( processRecv() ) {
        m_dbg.verbose(CALL_INFO,2,0,"clear Notify Functors\n");
        m_linkControl->setNotifyOnSend( NULL );
        return false;
    }

    return true;
}

bool MerlinIO::sendNotify( int vc )
{
    m_dbg.verbose(CALL_INFO,2,0,"vc=%d\n", vc );

    if ( processSend() ) {
        m_dbg.verbose(CALL_INFO,2,0,"clear Notify Functors\n");
        m_linkControl->setNotifyOnReceive( NULL );
        return false;
    }
    return true;
}

void MerlinIO::_componentInit(unsigned int phase )
{
    m_linkControl->init(phase);
}

void MerlinIO::enter( )
{
    m_dbg.verbose(CALL_INFO,1,0,"\n");

    drainInput();

    if( processRecv() ) {
        return;
    }

    if ( processSend() ) {
        return;
    }

    m_dbg.verbose(CALL_INFO,1,0,"set Notify Functors\n");
    m_linkControl->setNotifyOnSend( m_sendNotifyFunctor );
    m_linkControl->setNotifyOnReceive( m_recvNotifyFunctor );
}

void MerlinIO::drainInput()
{
    for ( int vc = 0; vc < m_numVC; vc++ ) {
        m_lastVC = (m_lastVC + 1) % m_numVC;
        m_dbg.verbose(CALL_INFO,2,0,"check VC %d\n", m_lastVC);
        while ( m_linkControl->eventToReceive( m_lastVC ) ) {
            
            m_dbg.verbose(CALL_INFO,2,0,"got event from VC %d\n", m_lastVC);
            m_recvEventQ.push_back( m_linkControl->recv(m_lastVC) );
        }
    }
}

bool MerlinIO::processSend()
{
    bool ret = false;
    while ( m_sendEntry && m_linkControl->spaceToSend(0,8) ) { 

        MerlinFireflyEvent* ev = new MerlinFireflyEvent;
        ev->setDest( m_sendEntry->node );
        ret = copyOut( m_dbg, *ev, *m_sendEntry );

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
            m_dbg.verbose(CALL_INFO,1,0,"send Entry done\n");
            m_selfLink->send( new SelfEvent( m_sendEntry ) );
            m_sendEntry = NULL;
        }
    }
    return ret;
}

void MerlinIO::selfHandler( Event* e )
{
    SelfEvent* event = static_cast<SelfEvent*>(e);

    if ( event ) {
        if ( event->entry ) {
            if ( event->entry->functor ) {
                m_dbg.verbose(CALL_INFO,2,0,"call functor\n");
                IO::Entry* tmp = (*event->entry->functor)();
                if ( tmp ) {
                    delete tmp;
                }
            } else {
                m_leaveLink->send(0,NULL);
            }
            delete event->entry;
        } else {
            m_leaveLink->send(0,NULL);
        }
        assert( m_haveCtrl );
        m_haveCtrl = false;
        delete e;
    } else {
        assert( ! m_haveCtrl );
        m_haveCtrl = true;
        enter();
    }
}

bool MerlinIO::processRecv(  )
{
    bool leaveFlag = false;
    while ( ! m_recvEventQ.empty() ) {
        m_dbg.verbose(CALL_INFO,2,0,"processRecv\n");
        leaveFlag = processRecvEvent( m_recvEventQ.front() );

        if ( cast(m_recvEventQ.front())->buf.empty() ) {
            delete m_recvEventQ.front();
            m_recvEventQ.pop_front();
        }

        if ( leaveFlag ) {
            break;
        }
    }
    return leaveFlag; 
}

bool MerlinIO::processRecvEvent( Event* e )
{
    MerlinFireflyEvent* event = static_cast<MerlinFireflyEvent*>( e );

    IO::NodeId src = event->src;

    if ( m_recvEntryM.find( src ) == m_recvEntryM.end() ) {
        m_dbg.verbose(CALL_INFO,1,0,"need a recv entry, src=%d\n", src);
        m_selfLink->send( new SelfEvent( NULL ) );
        return true;
    }

    if ( copyIn( m_dbg, *m_recvEntryM[ src ], *event ) ) {
        m_dbg.verbose(CALL_INFO,1,0,"recv entry done, src=%d\n",src );
        m_selfLink->send( new SelfEvent( m_recvEntryM[ src ] ) );
        m_recvEntryM.erase( src );
        return true;
    }

    return false;
}

bool MerlinIO::sendv( IO::NodeId dest, std::vector<IoVec>& ioVec,
                                               IO::Entry::Functor* functor )
{
    m_dbg.verbose(CALL_INFO,1,0,"dest=%d ioVec.size()=%lu functor=%p\n",
                                                dest, ioVec.size(), functor );

    for ( unsigned int i = 0; i < ioVec.size(); i++ ) {
        m_dbg.verbose(CALL_INFO,1,0,"ptr=%p len=%lu\n", 
                                    ioVec[i].ptr, ioVec[i].len );
    } 

    assert( ! m_sendEntry );

    m_sendEntry = new Entry( dest, ioVec, functor );
    m_selfLink->send( NULL );
    return true;
}

bool MerlinIO::recvv( IO::NodeId src, std::vector<IoVec>& ioVec, 
                                                IO::Entry::Functor* functor )
{
    m_dbg.verbose(CALL_INFO,1,0,"src=%d ioVec.size()=%lu\n", src, ioVec.size());


    for ( unsigned int i = 0; i < ioVec.size(); i++ ) {
        m_dbg.verbose(CALL_INFO,1,0,"ptr=%p len=%lu\n", 
                                    ioVec[i].ptr, ioVec[i].len );
    } 

    assert( m_recvEntryM.find(src) == m_recvEntryM.end() );

    m_recvEntryM[src] = new Entry( src, ioVec, functor );
    m_selfLink->send( NULL );
    return true;
}

void MerlinIO::wait()
{
    m_dbg.verbose(CALL_INFO,2,0,"\n");
    m_selfLink->send( NULL );
}

IO::NodeId MerlinIO::peek( )
{
    IO::NodeId src = IO::AnyId;
    if ( !m_recvEventQ.empty() ) {
        IO::NodeId tmp = 
                static_cast<Merlin::RtrEvent*>(m_recvEventQ.front())->src;
        if ( m_recvEntryM.find( tmp ) == m_recvEntryM.end() ) {
            src = tmp;
        }
    } 
    m_dbg.verbose(CALL_INFO,2,0,"src %d\n",src);
    return src;
}

static void print( Output& dbg, char* buf, int len )
{
    std::string tmp;
    for ( int i = 0; i < len; i++ ) {
        dbg.verbose(CALL_INFO,3,0,"%#03x\n",(unsigned char)buf[i]);
    }
}

static size_t 
copyIn( Output& dbg, MerlinIO::Entry& entry, MerlinFireflyEvent& event )
{
    dbg.verbose(CALL_INFO,2,0,"dest=%d ioVec.size()=%lu\n",
                                            entry.node, entry.ioVec.size() );


    for ( ; entry.currentVec < entry.ioVec.size(); 
                entry.currentVec++, entry.currentPos = 0 ) {

        if ( entry.ioVec[entry.currentVec].len ) {
            size_t toLen = entry.ioVec[entry.currentVec].len - entry.currentPos;
            size_t fromLen = event.buf.size();
            size_t len = toLen < fromLen ? toLen : fromLen;

            char* toPtr = (char*) entry.ioVec[entry.currentVec].ptr + 
                                                        entry.currentPos;
            dbg.verbose(CALL_INFO,2,0,"toBufSpace=%lu fromAvail=%lu, "
                            "memcpy len=%lu\n", toLen,fromLen,len);

            print( dbg, &event.buf[0], len );

            memcpy( toPtr, &event.buf[0], len );
            event.buf.erase(0,len);

            entry.currentPos += len;
            if ( 0 == event.buf.size() && 
                    entry.currentPos != entry.ioVec[entry.currentVec].len ) 
            {
                dbg.verbose(CALL_INFO,2,0,"event buffer empty\n");
                break;
            }
        }
    }

    dbg.verbose(CALL_INFO,2,0,"currentVec=%lu, currentPos=%lu\n",
                entry.currentVec, entry.currentPos);
    return ( entry.currentVec == entry.ioVec.size() ) ;
}

static bool
copyOut( Output& dbg, MerlinFireflyEvent& event, MerlinIO::Entry& entry )
{
    dbg.verbose(CALL_INFO,2,0,"dest=%d ioVec.size()=%lu\n",
                                            entry.node, entry.ioVec.size() );
    for ( ; entry.currentVec < entry.ioVec.size() && 
                event.buf.size() <  event.getMaxLength();
                entry.currentVec++, entry.currentPos = 0 ) {

        dbg.verbose(CALL_INFO,2,0,"vec[%lu].len %lu\n",entry.currentVec,
                    entry.ioVec[entry.currentVec].len );

        if ( entry.ioVec[entry.currentVec].len ) {
            size_t toLen = event.getMaxLength() - event.buf.size();
            size_t fromLen = entry.ioVec[entry.currentVec].len - 
                                                        entry.currentPos; 

            size_t len = toLen < fromLen ? toLen : fromLen;

            dbg.verbose(CALL_INFO,2,0,"toBufSpace=%lu fromAvail=%lu, "
                            "memcpy len=%lu\n", toLen,fromLen,len);

            const char* from = (const char*) entry.ioVec[entry.currentVec].ptr + 
                                                        entry.currentPos;

            event.buf.insert( event.buf.size(), from, len );
            entry.currentPos += len;
            if ( event.buf.size() == event.getMaxLength() &&
                    entry.currentPos != entry.ioVec[entry.currentVec].len ) {
                break;
            }
        }
    }
    dbg.verbose(CALL_INFO,2,0,"currentVec=%lu, currentPos=%lu\n",
                entry.currentVec, entry.currentPos);
    print( dbg, &event.buf[0], event.buf.size());
    return ( entry.currentVec == entry.ioVec.size() ) ;
}
