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

#include <sst_config.h>
#include <sst/core/serialization.h>

#include <memLink.h>
#include <debug.h>
#ifdef fatal
#undef fatal
#endif
#include <m5.h>
#include <memEvent.h>
#include <paramHelp.h>
#include <sst/core/link.h>
#include <sst/core/params.h>

using namespace SST::M5;


MemLink::MemLink( const MemLinkParams *p ) :
    MemObject( p ),
    m_comp( p->m5Comp ),
    m_port( NULL ),
    m_range( p->range ),
    m_snoop( p->snoop ),
    m_delay( p->delay )
{
    DBGX_M5(2,"linkName=`%s`\n", p->linkName.c_str());

    m_link = p->m5Comp->configureLink( p->linkName,
        new SST::Event::Handler<MemLink>( this, &MemLink::eventHandler ) );

	assert( !m_port);
    assert( m_link );
}

Port *MemLink::getPort(const std::string &if_name, int idx )
{
    DBGX_M5(2,"if_name=`%s` idx=%d\n",if_name.c_str(), idx );
	fprintf(stderr, "in %s:  m_port = %p, name = %s\n", __FUNCTION__, m_port, if_name.c_str());
    assert ( ! m_port );
    return m_port = new LinkPort( name() + ".linkPort", this );
}

void MemLink::init( void )
{
    DBGX_M5(2,"\n" );
    m_port->sendStatusChange( Port::RangeChange );
}

MemLink* MemLink::create( std::string name, M5* comp, Port* port, const SST::Params& params )
{
    MemLinkParams& memLinkParams = *new MemLinkParams;

    DBGC(2,"object name `%s` link name `%s`\n",
                name.c_str(), params.find_string("name").c_str());

    memLinkParams.name = name + ".link-" + params.find_string("name");
    memLinkParams.m5Comp = comp;
    memLinkParams.linkName = params.find_string("name");

    INIT_HEX( memLinkParams, params, range.start );
    INIT_HEX( memLinkParams, params, range.end );
    INIT_BOOL( memLinkParams, params, snoop );
    memLinkParams.delay = params.find_integer( "delay", 0 );

    MemLink* memLink = new MemLink( &memLinkParams );

    Port* linkPort = memLink->getPort( "" );
    assert( linkPort );

    linkPort->setPeer( port ); 
    port->setPeer( linkPort );

    return memLink;
}


void MemLink::getAddressRanges(AddrRangeList &resp, bool &snoop ) {
    DBGX_M5(2,"\n");
    resp.clear();
    snoop = m_snoop;
    if ( ! snoop ) { 
        DBGX_M5(2,"start=%#lx stop=%#lx\n", m_range.start, m_range.end );
        resp.push_back( RangeSize(m_range.start, m_range.size()));
    }
}

bool MemLink::send( SST::Event* event )
{
    // Note that we are not throttling data on this link
    DBGX_M5(3, "enter\n");
    m_link->send( m_delay, event ); 
    DBGX_M5(3, "return\n");
    return true;
}

void MemLink::eventHandler( SST::Event* e )
{
    MemEvent* event = static_cast<MemEvent*>(e);
    PacketPtr pkt = event->M5_Packet();

    DBGX_M5(3, "`%s` %#lx\n", pkt->cmdString().c_str(), (long)pkt->getAddr());

    switch ( event->type() ) {
        case MemEvent::Functional:
            m_port->sendFunctional( pkt );
            break;

        case MemEvent::Timing:
            m_port->sendTiming( pkt );
        break;
    }

    delete event;
}

