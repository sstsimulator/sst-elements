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
    DBGX("\n");
    m_recvQ.push_back( static_cast<IOEvent*>(e) );
}

bool SimpleIO::sendv( NodeId dest, std::vector<IoVec>& ioVec, Functor* functor )
{
    size_t len = 0;
    DBGX("dest=%d ioVec.size()=%d\n", dest, ioVec.size() );

    std::string buffer;
    for ( int i = 0; i < ioVec.size(); i++ ) {
        buffer.resize( buffer.size() + ioVec[i].len );
        memcpy( &buffer[ 0 ] + len,  ioVec[i].ptr, ioVec[i].len );
    }

    if ( m_dataReadyFunc ) {
        (*m_dataReadyFunc)( 0 );
    }

    if ( functor ) {
        Entry* tmp = (*functor)();
    
        if ( tmp ) {
            delete tmp;
        }
    }
    
    m_link->send( 0, new IOEvent( dest, buffer ) );

    return true;
}

bool SimpleIO::recvv( NodeId src, std::vector<IoVec>& ioVec, Functor* functor )
{
    size_t len = 0;
    DBGX("src=%d ioVec.size()=%d\n", src, ioVec.size() );

    for ( int i = 0; i < ioVec.size(); i++ ) {
        len += ioVec[i].len;
    }

    if ( 0 == len ) {
        goto done;
    }

    assert( ! m_recvQ.empty() );
    len = 0;

    for ( int i = 0; i < ioVec.size(); i++ ) {
        if ( ioVec[i].len ) {
            memcpy( ioVec[i].ptr, 
                    &m_recvQ.front()->getString()[0] + len, ioVec[i].len );
            len += ioVec[i].len; 
        }
    }

    delete m_recvQ.front();
    m_recvQ.pop_front();

done:

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
    DBGX("\n");
    if ( m_recvQ.empty() ) return 0;
    
    src =  m_recvQ.front()->getNodeId();
    return m_recvQ.front()->getString().length();
}
