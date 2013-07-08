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

#include <cxxabi.h>

#define DBGX( fmt, args... ) \
{\
    char* realname = abi::__cxa_demangle(typeid(*this).name(),0,0,NULL);\
    fprintf( stderr, "%d:%s::%s():%d: "fmt, getNodeId(), realname ? realname : "?????????", \
                        __func__, __LINE__, ##args);\
    if ( realname ) free(realname);\
}

using namespace SST::Firefly;
using namespace SST::Firefly::IO;

SimpleIO::SimpleIO( Params& params ) :
    Interface(),
    m_dataReadyFunc( NULL ),
    m_myNodeId( AnyId )
{
    SST::Component* owner = (SST::Component*) params.find_integer( "owner" );

    m_link = owner->configureLink( "link", "50 ps",
            new Event::Handler<SimpleIO>(this, &SimpleIO::handleEvent));
    if ( NULL == m_link ) {
        _abort(SimpleIO,"configureLink failed\n");
    } 
}

void SimpleIO::_componentInit(unsigned int phase )
{
    SST::Interfaces::StringEvent* event = 
        static_cast<SST::Interfaces::StringEvent*>( m_link->recvInitData() );
    if ( event ) {
        m_myNodeId = atoi( event->getString().c_str() );
        DBGX("set node id to %d\n", m_myNodeId );
    } 
}

void SimpleIO::setDataReadyFunc( Functor2* dataReadyFunc )
{
    m_dataReadyFunc = dataReadyFunc; 
}


void SimpleIO::handleEvent(SST::Event* e){
    IOEvent* event = static_cast<IOEvent*>(e);
    
    NodeId srcNode = event->getNodeId(); 

    if ( m_streamMap.find( srcNode ) == m_streamMap.end() ) {
        DBGX("srcNode %d new src\n",srcNode);
        m_streamMap[ srcNode ] = event->getString();
    } else {
        DBGX("srcNode %d append\n",srcNode);
        m_streamMap[ srcNode ].insert( m_streamMap[ srcNode ].size(), 
                                    event->getString() ); 
    }
    DBGX("%lu bytes avail from %d\n", m_streamMap[ srcNode ].size(), srcNode );

    if ( m_dataReadyFunc ) {
        (*m_dataReadyFunc)( 0 );
    }

    delete e;
}

bool SimpleIO::sendv( NodeId dest, std::vector<IoVec>& ioVec,
                                                    Entry::Functor* functor )
{
    size_t len = 0;
    DBGX("dest=%d ioVec.size()=%lu\n", dest, ioVec.size() );
    
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

#if 0
    for ( int j = 0; j < buffer.size(); j++ ) {
        DBGX("%x\n",buffer[j]);
    }
#endif

    if ( functor ) {
        Entry* tmp = (*functor)();
    
        if ( tmp ) {
            delete tmp;
        }
    }

    DBGX(" sending %lu bytes\n", buffer.size() );
    
    m_link->send( 0, new IOEvent( dest, buffer ) );

    return true;
}

bool SimpleIO::recvv( NodeId src, std::vector<IoVec>& ioVec, 
                                                    Entry::Functor* functor )
{
    DBGX("src=%d ioVec.size()=%lu\n", src, ioVec.size() );

    assert( m_streamMap.find( src ) != m_streamMap.end() ); 

    std::string& buf = m_streamMap[src ];

    for ( unsigned int i = 0; i < ioVec.size(); i++ ) {
        assert( buf.size() >= ioVec[i].len );
    //    DBGX("copied %d bytes\n",ioVec[i].len );
        memcpy( ioVec[i].ptr, &buf[0], ioVec[i].len );
        buf.erase(0, ioVec[i].len );
    //    DBGX("%d bytes left\n",buf.size() );
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
    DBGX("src %d\n",src);
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
    DBGX("src OK %d\n",src);
    
    return m_streamMap[src].size();
}
