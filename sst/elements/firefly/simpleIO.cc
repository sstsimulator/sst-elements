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
#include "sst/core/serialization/element.h"
#include "sst/core/component.h"
#include "sst/core/interfaces/stringEvent.h"

#include "simpleIO.h"

using namespace SST::Firefly;
using namespace SST::Firefly::IO;

SimpleIO::SimpleIO( Params& params ) :
    Interface(),
    m_dataReadyFunc( NULL ),
    m_myNodeId( AnyId )
{
    SST::Component* owner = (SST::Component*) params.find_integer( "owner" );

    m_dbg.init("@t:SimpleIO::@p():@l ",
       params.find_integer("verboseLevel",0),0,
       (Output::output_location_t)params.find_integer("debug", 0));

    m_link = owner->configureLink( "link", "50 ps",
            new Event::Handler<SimpleIO>(this, &SimpleIO::handleEvent));
    if ( NULL == m_link ) {
        m_dbg.fatal(CALL_INFO,0,0,0,"configureLink failed\n");
    } 

    m_selfLink = owner->configureSelfLink("SimpleIO", "1 ps",
        new Event::Handler<SimpleIO>(this,&SimpleIO::handleSelfLink));

    if ( NULL == m_selfLink ) {
        m_dbg.fatal(CALL_INFO,0,0,0,"configureLink failed\n");
    } 
}

void SimpleIO::_componentInit(unsigned int phase )
{
    m_dbg.verbose(CALL_INFO,1,0,"phase=%d\n",phase);
    if ( 0 == phase ) return;
    SST::Interfaces::StringEvent* event = 
        static_cast<SST::Interfaces::StringEvent*>( m_link->recvInitData() );
    if ( event ) {
        m_myNodeId = atoi( event->getString().c_str() );
        m_dbg.verbose(CALL_INFO,1,0,"set node id to %d\n", m_myNodeId );
    } 
}

void SimpleIO::setDataReadyFunc( Functor2* dataReadyFunc )
{
    m_dbg.verbose(CALL_INFO,1,0,"dataReadyFunc %p\n",dataReadyFunc);
    m_dataReadyFunc = dataReadyFunc; 
}


void SimpleIO::handleEvent(SST::Event* e){
    IOEvent* event = static_cast<IOEvent*>(e);
    
    NodeId srcNode = event->getNodeId(); 

    if ( m_streamMap.find( srcNode ) == m_streamMap.end() ) {
        m_dbg.verbose(CALL_INFO,2,0,"srcNode %d new src\n",srcNode);
        m_streamMap[ srcNode ] = event->getString();
    } else {
        m_dbg.verbose(CALL_INFO,2,0,"srcNode %d append\n",srcNode);
        m_streamMap[ srcNode ].insert( m_streamMap[ srcNode ].size(), 
                                    event->getString() ); 
    }
    m_dbg.verbose(CALL_INFO,1,0,"%lu bytes avail from %d\n",
                            m_streamMap[ srcNode ].size(), srcNode );

    if ( m_dataReadyFunc ) {
        (*m_dataReadyFunc)( 0 );
    }

    delete e;
}

class XXX : public SST::Event {
  public: 
    enum { Send, Recv } type;
    XXX(): Event() { }
    NodeId node;
    std::vector<IoVec> ioVec;
    Entry::Functor* functor;
    
};

bool SimpleIO::sendv( NodeId dest, std::vector<IoVec>& ioVec,
                                                    Entry::Functor* functor )
{
    m_dbg.verbose(CALL_INFO,2,0,"dest=%d ioVec.size()=%lu\n",
                                                dest, ioVec.size() );
    XXX* xxx = new XXX;
    xxx->node = dest;
    xxx->ioVec = ioVec;
    xxx->functor = functor;
    xxx->type = XXX::Send;
    m_selfLink->send(0,xxx);
    return true;
}


void SimpleIO::handleSelfLink(SST::Event* e)
{
    XXX* xxx = static_cast<XXX*>(e);
    switch ( xxx->type)  {
    case XXX::Send:
        _sendv( xxx->node, xxx->ioVec, xxx->functor );
        break;
    case XXX::Recv:
        _recvv( xxx->node, xxx->ioVec, xxx->functor );
        break;
    }  
}

bool SimpleIO::_sendv( NodeId dest, std::vector<IoVec>& ioVec,
                                                    Entry::Functor* functor )
{
    size_t len = 0;
    m_dbg.verbose(CALL_INFO,1,0,"dest=%d ioVec.size()=%lu\n",
                                                    dest, ioVec.size() );
    
    for ( unsigned int i = 0; i < ioVec.size(); i++ ) {
        len += ioVec[i].len;
    }

    std::string buffer;
    buffer.resize( len );

    len = 0;
    for ( unsigned int i = 0; i < ioVec.size(); i++ ) {
        memcpy( &buffer[ len ],  ioVec[i].ptr, ioVec[i].len );
        len += ioVec[i].len; 
    }

    for ( unsigned int j = 0; j < buffer.size(); j++ ) {
        m_dbg.verbose(CALL_INFO,2,0,"%x\n",(unsigned char)buffer[j]);
    }

    if ( functor ) {
        Entry* tmp = (*functor)();
    
        if ( tmp ) {
            delete tmp;
        }
    }

    m_dbg.verbose(CALL_INFO,2,0," sending %lu bytes\n", buffer.size() );
    
    m_link->send( 0, new IOEvent( dest, buffer ) );

    return true;
}

bool SimpleIO::recvv( NodeId src, std::vector<IoVec>& ioVec, 
                                                    Entry::Functor* functor )
{
    m_dbg.verbose(CALL_INFO,2,0,"src=%d ioVec.size()=%lu\n", src, ioVec.size());
    XXX* xxx = new XXX;
    xxx->node = src;
    xxx->ioVec = ioVec;
    xxx->functor = functor;
    xxx->type = XXX::Recv;
    m_selfLink->send(0,xxx);
    return true;
}


bool SimpleIO::_recvv( NodeId src, std::vector<IoVec>& ioVec, 
                                                    Entry::Functor* functor )
{
    m_dbg.verbose(CALL_INFO,1,0,"src=%d ioVec.size()=%lu\n", src, ioVec.size());
    assert( m_streamMap.find( src ) != m_streamMap.end() ); 

    std::string& buf = m_streamMap[src ];

    for ( unsigned int i = 0; i < ioVec.size(); i++ ) {
        assert( buf.size() >= ioVec[i].len );
        m_dbg.verbose(CALL_INFO,1,0,"copied %lu bytes\n",ioVec[i].len );
        memcpy( ioVec[i].ptr, &buf[0], ioVec[i].len );
        buf.erase(0, ioVec[i].len );
        m_dbg.verbose(CALL_INFO,1,0,"%lu bytes left\n",buf.size() );
    }

    if ( buf.empty() ) {
        m_streamMap.erase( src );
    }

    if ( functor ) {
        Entry* tmp = (*functor)();
    
        if ( tmp ) {
            delete tmp;
        }
    }
    return true;
}

bool SimpleIO::isReady( NodeId dest )
{
    return false;
}

size_t SimpleIO::peek( NodeId& src )
{
    m_dbg.verbose(CALL_INFO,1,0,"src %d\n",src);
    if ( src == IO::AnyId ) {
        if ( m_streamMap.empty() ) {
            return 0;
        } else {
            std::map<NodeId,std::string>::iterator it = m_streamMap.begin();
            src = it->first;
        } 
    } else {
        if ( m_streamMap.find( src ) == m_streamMap.end() ) {
            return 0;
        } 
    }
    m_dbg.verbose(CALL_INFO,1,0,"src OK %d\n",src);
    
    return m_streamMap[src].size();
}
