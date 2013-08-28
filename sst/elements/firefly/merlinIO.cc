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
#include "sst/core/serialization.h"
#include "sst/core/component.h"
#include "sst/core/timeLord.h"
#include "sst/core/module.h"
#include "sst/elements/merlin/linkControl.h"

#include "merlinIO.h"

using namespace SST::Firefly;

class MerlinFireflyEvent : public Merlin::RtrEvent {

    static const int BufLen = 56;

  public:
    uint16_t        seq;
    std::string     buf;

    MerlinFireflyEvent() {} 

    MerlinFireflyEvent(const MerlinFireflyEvent *me) : 
        Merlin::RtrEvent() 
    {
        buf = me->buf;
        seq = me->seq;
    }

    MerlinFireflyEvent(const MerlinFireflyEvent &me) : 
        Merlin::RtrEvent() 
    {
        buf = me.buf;
        seq = me.seq;
    }

    virtual RtrEvent* clone(void)
    {
        return new MerlinFireflyEvent(*this);
    }

    void setNumFlits( size_t len ) {
        size_in_flits = len / 8;
        if ( len % 8 ) 
        ++size_in_flits;

        // add flit for 8 bytes of packet header info 
        ++size_in_flits;
    }

    void setDest( int _dest ) {
        dest = _dest;
    }

    void setSrc( int _src ) {
        src = _src;
    }

    size_t getMaxLength(){
        return BufLen;
    }

  private:

    friend class boost::serialization::access;
    template<class Archive>
    void
    serialize(Archive & ar, const unsigned int version )
    {
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(RtrEvent);
        ar & BOOST_SERIALIZATION_NVP(seq);
        ar & BOOST_SERIALIZATION_NVP(buf);
    }
};

BOOST_CLASS_EXPORT(MerlinFireflyEvent)

static size_t 
copyIn( Output& dbg, MerlinIO::Entry& entry, MerlinFireflyEvent& event );

static bool
copyOut( Output& dbg, MerlinFireflyEvent& event, MerlinIO::Entry& entry );

MerlinIO::MerlinIO( Params& params ) :
    Interface(),
    m_leaveLink( NULL ),
    m_sendEntry( NULL ),
    m_myNodeId( IO::AnyId ),
    m_numVC( 2 ),
    m_lastVC( 0 )
{
    SST::Component* owner = (SST::Component*) params.find_integer( "owner" );

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

    int buffer_size = params.find_integer("buffer_size",128);

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

    owner->registerClock( "1GHz", 
        new Clock::Handler<MerlinIO>(this,&MerlinIO::clockHandler), false);
}

void MerlinIO::_componentInit(unsigned int phase )
{
    m_linkControl->init(phase);
}

bool MerlinIO::clockHandler( Cycle_t cycle )
{
    if ( m_eventQ.size() < MaxPendingEvents ) {
        for ( int vc = 0; vc < m_numVC; vc++ ) {
            m_lastVC = (m_lastVC + 1) % m_numVC;
            MerlinFireflyEvent* ev = 
                static_cast<MerlinFireflyEvent*>(m_linkControl->recv(m_lastVC));
            if ( ev ) {
                m_dbg.verbose(CALL_INFO,2,0,"got event from %d\n", ev->src );
                m_eventQ.push_back( ev );
            }
        } 
    }

    // if m_leaveLink is not NULL it means we are in control
    if ( m_leaveLink ) {
        if ( processSend() ) {
            leave(); 
        } else if ( processRecv() ) {
            leave();
        }
    } 
    return false; 
}

bool MerlinIO::pending( )
{
    m_dbg.verbose(CALL_INFO,2,0,"%s\n",
    ( ( NULL != m_sendEntry ) || ! m_recvEntryM.empty() ) ? "true" : "false" );

    return ( ( NULL != m_sendEntry ) || ! m_recvEntryM.empty() );
}

void MerlinIO::enter( SST::Link* link  )
{
    m_dbg.verbose(CALL_INFO,2,0,"\n");
    assert( ! m_leaveLink );
    // the clockHandler will use m_leaveLink as a flag to start processing
    m_leaveLink = link;
}

void MerlinIO::leave()
{
    m_dbg.verbose(CALL_INFO,2,0,"\n");
    m_leaveLink->send(0,NULL);
    m_leaveLink = NULL;
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
        ev->setTraceType( Merlin::RtrEvent::FULL );
#endif
        m_dbg.verbose(CALL_INFO,2,0,"sending event with %lu bytes %p\n",
                                                        ev->buf.size(), ev);
        assert( ev->buf.size() );
        bool sent = m_linkControl->send( ev, 0 );
        assert( sent );

        if ( ret ) {
            m_dbg.verbose(CALL_INFO,1,0,"send Entry done\n");
            delete m_sendEntry;
            m_sendEntry = NULL;
        }
    }
    return ret;
}

bool MerlinIO::processRecv()
{
    while( ! m_eventQ.empty() ) 
    {
        MerlinFireflyEvent* event = static_cast<MerlinFireflyEvent*>
                    (m_eventQ.front() ); 

        IO::NodeId src = event->src;

        if ( m_recvEntryM.find( src ) == m_recvEntryM.end() ) {
            m_dbg.verbose(CALL_INFO,1,0,"need a recv entry, src=%d\n", src);
            return true;
        }

        if ( copyIn( m_dbg, *m_recvEntryM[ src ], *event ) ) {
            delete m_recvEntryM[ src ];
            m_recvEntryM[ src ] = NULL;
        }

        if ( event->buf.empty() ) {
            m_dbg.verbose(CALL_INFO,1,0,"done with event, src=%d\n", src);
            delete m_eventQ.front();
            m_eventQ.pop_front();
        } 

        if ( ! m_recvEntryM[ src ] ) {
            m_dbg.verbose(CALL_INFO,1,0,"recv entry done, src=%d\n",src );
            m_recvEntryM.erase( src );
            return true;
        }
    }
    return false;
}

bool MerlinIO::sendv( IO::NodeId dest, std::vector<IO::IoVec>& ioVec,
                                               IO::Entry::Functor* functor )
{
    m_dbg.verbose(CALL_INFO,1,0,"dest=%d ioVec.size()=%lu\n",
                                                dest, ioVec.size() );
    for ( unsigned int i = 0; i < ioVec.size(); i++ ) {
        m_dbg.verbose(CALL_INFO,1,0,"ptr=%p len=%lu\n", 
                                    ioVec[i].ptr, ioVec[i].len );
    } 

    assert( ! m_sendEntry );

    m_sendEntry = new Entry( dest, ioVec, functor );
    return true;
}

bool MerlinIO::recvv( IO::NodeId src, std::vector<IO::IoVec>& ioVec, 
                                                IO::Entry::Functor* functor )
{
    m_dbg.verbose(CALL_INFO,1,0,"src=%d ioVec.size()=%lu\n", src, ioVec.size());

    for ( unsigned int i = 0; i < ioVec.size(); i++ ) {
        m_dbg.verbose(CALL_INFO,1,0,"ptr=%p len=%lu\n", 
                                    ioVec[i].ptr, ioVec[i].len );
    } 

    assert( m_recvEntryM.find(src) == m_recvEntryM.end() );

    m_recvEntryM[src] = new Entry( src, ioVec, functor );
    return true;
}

IO::NodeId MerlinIO::peek( )
{
    IO::NodeId src = IO::AnyId;
    if ( ! m_eventQ.empty() ) {
        
        IO::NodeId tmp = static_cast<Merlin::RtrEvent*>(m_eventQ.front())->src;
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

    print( dbg, &event.buf[0], event.buf.size());

    for ( ; entry.currentVec < entry.ioVec.size(); 
                entry.currentVec++, entry.currentPos = 0 ) {

        if ( entry.ioVec[entry.currentVec].len ) {
            size_t toLen = entry.ioVec[entry.currentVec].len - entry.currentPos;
            size_t fromLen = event.buf.size();
            size_t len = toLen < fromLen ? toLen : fromLen;

            dbg.verbose(CALL_INFO,2,0,"toBufSpace=%lu fromAvail=%lu, "
                            "memcpy len=%lu\n", toLen,fromLen,len);
            char* toPtr = (char*) entry.ioVec[entry.currentVec].ptr + 
                                                        entry.currentPos;

    
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
