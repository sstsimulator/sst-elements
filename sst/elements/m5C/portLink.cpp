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
#include <sst/core/serialization.h>
#include "portLink.h"

#include <sst/core/component.h>
#include <sst/core/interfaces/stringEvent.h>

#include <debug.h>
#include <m5.h>
#include <dll/gem5dll.hh>
#include <dll/memEvent.hh>
#include <rawEvent.h>
//#include <sst/elements/interfaces/memEvent.h>


namespace SST {
namespace M5 {


PortLink::PortLink( M5& comp, Gem5Object_t& obj, const SST::Params& params ) :
	m_doTranslate( false ),
    m_comp( comp ),
    m_myEndPoint( this, recv, poke )
{
    bool snoopOut = false, snoopIn = false;
    std::string name = params.find_string("name");
    assert( ! name.empty() );
    std::string portName = params.find_string("portName");
    assert( ! portName.empty() );
    int idx = params.find_integer( "portIndex" );
    uint64_t addrStart = 0, addrEnd = -1;
	unsigned blocksize = 0;

    if ( params.find_string("snoopOut").compare("true") == 0 ) {
        snoopOut = true;
    }

    if ( params.find_string("snoopIn").compare("true") == 0 ) {
        snoopIn = true;
    }

    if ( params.find("range.start") != params.end() ) {
        addrStart = params.find_integer("range.start");
    }
    if ( params.find("range.end") != params.end() ) {
        addrEnd = params.find_integer("range.end");
    }
    if ( params.find("blocksize") != params.end() ) {
        blocksize = params.find_integer("blocksize");
    }

    DBGX(2,"%s portName=%s index=%d\n", name.c_str(), portName.c_str(), idx );

    m_link = comp.configureLink( name,
        new SST::Event::Handler<PortLink>( this, &PortLink::eventHandler ) );
    assert( m_link );

    m_gem5EndPoint = GetPort(obj, m_myEndPoint, snoopOut, snoopIn, 
                            addrStart, addrEnd, blocksize, name, portName, idx ); 

    assert( m_gem5EndPoint );
}

void PortLink::setup(void) {
	SST::Event *ev = NULL;
	while ( (ev = m_link->recvInitData()) != NULL ) {
		SST::Interfaces::StringEvent *se = dynamic_cast<SST::Interfaces::StringEvent*>(ev);
		if ( se ) {
			std::string str = se->getString();
			if ( str == "SST::Interfaces::MemEvent" ) {
				m_doTranslate = true;
			}
		}
		delete ev;
	}
}

void PortLink::eventHandler( SST::Event *e )
{
    DBGX(2,"\n");
	RawEvent *event = NULL;

	if ( m_doTranslate ) {
		event = new RawEvent(convertSSTtoGEM5(e), sizeof(MemPkt));
		delete e;
		e = event;
	} else {
		event = static_cast<RawEvent*>( e );
	}

	if ( ! m_deferredQ.empty() ) {
		DBGX(2,"blocked, defer event\n");
		m_deferredQ.push_back( event );
		return;
	}

	if ( ! m_gem5EndPoint->send( event->data(), event->len() ) ) {
		DBGX(2,"busy, defer event\n");
		m_deferredQ.push_back( event );
		return;
	}

    delete e;
}



MemPkt* PortLink::findMatchingEvent(SST::Interfaces::MemEvent *sstev)
{
	MemPkt* g5ev = NULL;
	//fprintf(stderr, "%s:%d %s\n", __FILE__, __LINE__, __FUNCTION__);
	for ( std::list<MemPkt*>::iterator i = m_g5events.begin() ; i != m_g5events.end() ; ++i ) {
		MemPkt *ev = *i;
		/* TODO:  More intelligent matching? */
		if ( ev->addr == sstev->getAddr() ) {
			g5ev = ev;
			m_g5events.erase(i);
			break;
		}
	}
	//fprintf(stderr, "%s:%d %s: Returning patcket %zu\n", __FILE__, __LINE__, __FUNCTION__, g5ev->pktId);
	return g5ev;
}


MemPkt* PortLink::convertSSTtoGEM5( SST::Event *e )
{
	//fprintf(stderr, "%s:%d %s\n", __FILE__, __LINE__, __FUNCTION__);
	SST::Interfaces::MemEvent *sstev = static_cast<SST::Interfaces::MemEvent*>(e);
	MemPkt* pkt = findMatchingEvent(sstev);
	if ( !pkt ) {
		_abort(Gem5Converter, "Trying to convert SST even for which no matching GEM5 event is found.\n");
	}

	switch ( sstev->getCmd() ) {
	case SST::Interfaces::ReadResp:
		pkt->cmd = ::MemCmd::ReadResp;
		pkt->isResponse = true;
		break;
	case SST::Interfaces::WriteResp:
		pkt->cmd = ::MemCmd::WriteResp;
		pkt->isResponse = true;
		break;
	default:
		_abort(Gem5Converter, "Trying to map SST cmd %d to GEM5\n", sstev->getCmd());
		break;
	}

	pkt->size = sstev->getSize();
	assert(pkt->size <= (unsigned)MemPkt::DataSize);
	memcpy(pkt->data, &(sstev->getPayload()[0]), pkt->size);

	/* Swap src/dst */
	int tmp = pkt->src;
	pkt->src = pkt->dest;
	pkt->dest = tmp;

	/* TODO:  finishTime ?   firstWordTime ?  pktId ? */

	return pkt;
}


SST::Interfaces::MemEvent* PortLink::convertGEM5toSST( MemPkt *pkt )
{
	//fprintf(stderr, "%s:%d %s\n", __FILE__, __LINE__, __FUNCTION__);
	SST::Interfaces::MemEvent *ev = new SST::Interfaces::MemEvent(&m_comp, pkt->addr, SST::Interfaces::NULLCMD);
	ev->setPayload(pkt->size, pkt->data);

	/* From ${GEM5}/src/mem/request.hh */
    static const uint32_t LOCKED                      = 0x00100000;
    static const uint32_t UNCACHED                    = 0x00001000;

	if ( pkt->req.flags & LOCKED ) {
		ev->setFlags(SST::Interfaces::MemEvent::F_LOCKED);
		ev->setLockID(((uint64_t)(pkt->req.contextId) <<32) | (uint64_t)(pkt->req.threadId));
	}

    if ( pkt->req.flags & UNCACHED ) {
        ev->setFlags(SST::Interfaces::MemEvent::F_UNCACHED);
    }


	switch ( (::MemCmd::Command)pkt->cmd) {
	case ::MemCmd::ReadReq:
	case ::MemCmd::ReadExReq:
		ev->setCmd(SST::Interfaces::ReadReq);
		break;
	case ::MemCmd::WriteReq:
		ev->setCmd(SST::Interfaces::WriteReq);
		break;
	case ::MemCmd::Writeback:
		ev->setCmd(SST::Interfaces::WriteReq);
		ev->setFlags(SST::Interfaces::MemEvent::F_WRITEBACK);
		break;

	default:
		_abort(Gem5Converter, "Don't know how to conver command %d to SST\n",
				pkt->cmd);
	}
	m_g5events.push_back(pkt);
	//fprintf(stderr, "%s:%d %s: Storing patcket %zu\n", __FILE__, __LINE__, __FUNCTION__, pkt->pktId);
	return ev;
}

}
}

