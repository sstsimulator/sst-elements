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
    uint16_t seq;
    std::string             buf;
    virtual RtrEvent* clone(void)
    {
        return new MerlinFireflyEvent(*this);
    }
    void setNumFlits( size_t len ) {
        size_in_flits = len / 8;
        if ( len % 8 ) 
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

#define MIN(X,Y) ((X) < (Y) ? : (X) : (Y))

void print( Output& dbg, char* buf, int len )
{
    std::string tmp;
    for ( int i = 0; i < len; i++ ) {
        dbg.verbose(CALL_INFO,1,0,"%#03x\n",(unsigned char)buf[i]);
    }
}

size_t copyIn( Output& dbg, MerlinIO::Entry& entry, MerlinFireflyEvent& event )
{
    dbg.verbose(CALL_INFO,1,0,"dest=%d ioVec.size()=%lu\n",
                                            entry.node, entry.ioVec.size() );

    print( dbg, &event.buf[0], event.buf.size());

    for ( ; entry.currentVec < entry.ioVec.size(); 
                entry.currentVec++, entry.currentPos = 0 ) {

        if ( entry.ioVec[entry.currentVec].len ) {
            size_t toLen = entry.ioVec[entry.currentVec].len - entry.currentPos;
            size_t fromLen = event.buf.size();
            size_t len = toLen < fromLen ? toLen : fromLen;

            dbg.verbose(CALL_INFO,1,0,"toBufSpace=%lu fromAvail=%lu, "
                            "memcpy len=%lu\n", toLen,fromLen,len);
            char* toPtr = (char*) entry.ioVec[entry.currentVec].ptr + 
                                                        entry.currentPos;

    
            memcpy( toPtr, &event.buf[0], len );
            event.buf.erase(0,len);

            entry.currentPos += len;
            if ( 0 == event.buf.size() && 
                    entry.currentPos != entry.ioVec[entry.currentVec].len ) 
            {
                dbg.verbose(CALL_INFO,1,0,"event buffer empty\n");
                break;
            }
        }
    }

    dbg.verbose(CALL_INFO,1,0,"currentVec=%lu, currentPos=%lu\n",
                entry.currentVec, entry.currentPos);
    return ( entry.currentVec == entry.ioVec.size() ) ;
}

bool copyOut( Output& dbg, MerlinFireflyEvent& event, MerlinIO::Entry& entry )
{
    dbg.verbose(CALL_INFO,1,0,"dest=%d ioVec.size()=%lu\n",
                                            entry.node, entry.ioVec.size() );
    for ( ; entry.currentVec < entry.ioVec.size() && 
                event.buf.size() <  event.getMaxLength();
                entry.currentVec++, entry.currentPos = 0 ) {

        dbg.verbose(CALL_INFO,1,0,"vec[%lu].len %lu\n",entry.currentVec,
                    entry.ioVec[entry.currentVec].len );

        if ( entry.ioVec[entry.currentVec].len ) {
            size_t toLen = event.getMaxLength() - event.buf.size();
            size_t fromLen = entry.ioVec[entry.currentVec].len - 
                                                        entry.currentPos; 

            size_t len = toLen < fromLen ? toLen : fromLen;

            dbg.verbose(CALL_INFO,1,0,"toBufSpace=%lu fromAvail=%lu, "
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
    dbg.verbose(CALL_INFO,1,0,"currentVec=%lu, currentPos=%lu\n",
                entry.currentVec, entry.currentPos);
    return ( entry.currentVec == entry.ioVec.size() ) ;
}

MerlinIO::MerlinIO( Params& params ) :
    Interface(),
    m_dataReadyFunc( NULL ),
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

void MerlinIO::setDataReadyFunc( IO::Functor2* dataReadyFunc )
{
    m_dbg.verbose(CALL_INFO,1,0,"dataReadyFunc %p\n",dataReadyFunc);
    m_dataReadyFunc = dataReadyFunc; 
}

bool MerlinIO::clockHandler( Cycle_t cycle )
{
    while ( ! m_sendQ.empty() && m_linkControl->spaceToSend(0,8) ) { 

        MerlinFireflyEvent* ev = new MerlinFireflyEvent;
        ev->setDest( m_sendQ.front()->node );
        if ( copyOut( m_dbg, *ev, *m_sendQ.front() ) ){
            m_dbg.verbose(CALL_INFO,1,0,"pop send Entry\n");

            if ( m_sendQ.front()->functor ) {
                IO::Entry* tmp = (*m_sendQ.front()->functor)();
    
                if ( tmp ) {
                    delete tmp;
                }
            }
            delete m_sendQ.front();
            m_sendQ.pop_front();
        }
        ev->setSrc( m_myNodeId );
        ev->setNumFlits( ev->buf.size() );
        //ev->setTraceType( Merlin::RtrEvent::FULL );
        m_dbg.verbose(CALL_INFO,1,0,"sending event with %lu bytes %p\n",
                    ev->buf.size(), ev);
        print(m_dbg,&ev->buf[0],ev->buf.size());
        assert( ev->buf.size() );
        bool sent = m_linkControl->send( ev, 0 );
        assert( sent );
    }

    
    for ( int vc = 0; vc < m_numVC; vc++ ) {
        m_lastVC = (m_lastVC + 1) % m_numVC;
        MerlinFireflyEvent* ev = static_cast<MerlinFireflyEvent*>(m_linkControl->recv( m_lastVC ));
        if ( ev ) {
            
            if ( m_eventMap.find( ev->src ) == m_eventMap.end() ) {
                IN in;
                m_eventMap[ev->src] = in; 
            }
            m_eventMap[ev->src].queue.push_back(ev);
            m_eventMap[ev->src].nbytes += ev->buf.size();
            m_dbg.verbose(CALL_INFO,1,0,"got event from %d with %lu bytes,"
            " %lu total avail\n", ev->src, ev->buf.size(),
                    m_eventMap[ev->src].nbytes);
#if 0
            if ( m_dataReadyFunc ) {
                (*m_dataReadyFunc)( 0 );
                m_dataReadyFunc = NULL;
            }
#endif
        }
    } 

    if ( ! m_eventMap.empty() ) {
        if ( m_dataReadyFunc ) {
            (*m_dataReadyFunc)( 0 );
            m_dataReadyFunc = NULL;
        }
    }

    while ( ! m_recvQ.empty() ) { 
        
        int src = m_recvQ.front()->node; 
        m_dbg.verbose(CALL_INFO,1,0,"have recv entry for src %d, Q size %lu\n",
                            src, m_recvQ.size());
        if ( m_eventMap.find( src ) == m_eventMap.end() )  {
            break;
        } 

        m_dbg.verbose(CALL_INFO,1,0,"have event from src %d\n",src);
        IN& in = m_eventMap[ src ]; 

        while ( ! in.queue.empty() ) {
            size_t tmp = static_cast<MerlinFireflyEvent*>(in.queue.front())->buf.size();
            bool done = copyIn( m_dbg, *m_recvQ.front(),
                    *static_cast<MerlinFireflyEvent*>(in.queue.front() ));

            in.nbytes -= 
                (tmp - static_cast<MerlinFireflyEvent*>(in.queue.front())->buf.size());
            m_dbg.verbose(CALL_INFO,1,0,"%lu bytes avail from src %d\n",
                in.nbytes, src );

            if ( static_cast<MerlinFireflyEvent*>(in.queue.front())->buf.empty() ) { 
                in.queue.pop_front();
                m_dbg.verbose(CALL_INFO,1,0,"pop event\n");
                if ( in.queue.empty() ) {
                
                    m_dbg.verbose(CALL_INFO,1,0,"no more events from src %d\n",
                                                    src);
                    m_eventMap.erase( src );
                }
            }

            if ( done ) {
                m_dbg.verbose(CALL_INFO,1,0,"done with recv Entry\n");
                if ( m_recvQ.front()->functor ) {
                    m_dbg.verbose(CALL_INFO,1,0,"call IO functor\n");
                    IO::Entry* tmp = (*m_recvQ.front()->functor)();
    
                    if ( tmp ) {
                        delete tmp;
                    }
                }
                delete m_recvQ.front();
                m_recvQ.pop_front();
                break;
            }
        }
    }
    
    return false;
}

bool MerlinIO::sendv( IO::NodeId dest, std::vector<IO::IoVec>& ioVec,
                                               IO::Entry::Functor* functor )
{
    m_dbg.verbose(CALL_INFO,2,0,"dest=%d ioVec.size()=%lu\n",
                                                dest, ioVec.size() );
    m_sendQ.push_back( new Entry( dest, ioVec, functor ) );
    return true;
}

bool MerlinIO::recvv( IO::NodeId src, std::vector<IO::IoVec>& ioVec, 
                                                IO::Entry::Functor* functor )
{
    m_dbg.verbose(CALL_INFO,2,0,"src=%d ioVec.size()=%lu\n", src, ioVec.size());
    for ( unsigned int i = 0; i < ioVec.size(); i++ ) {
        m_dbg.verbose(CALL_INFO,2,0,"ptr=%p len=%lu\n", 
                                    ioVec[i].ptr, ioVec[i].len );
    } 
    m_recvQ.push_back( new Entry( src, ioVec, functor ) );
    return true;
}

bool MerlinIO::isReady( IO::NodeId dest )
{
    return false;
}

size_t MerlinIO::peek( IO::NodeId& src )
{
    m_dbg.verbose(CALL_INFO,1,0,"src %d\n",src);
    if ( src == IO::AnyId ) {
        if ( m_eventMap.empty() ) {
            return 0;
        } else {
            std::map<IO::NodeId,IN>::iterator it = m_eventMap.begin();
            src = it->first;
        } 
    } else {
        if ( m_eventMap.find( src ) == m_eventMap.end() ) {
            return 0;
        } 
    }
    m_dbg.verbose(CALL_INFO,1,0,"src %d had %lu bytes in recv buffer\n",
                src,m_eventMap[src].nbytes);
    
    return m_eventMap[src].nbytes;
}
