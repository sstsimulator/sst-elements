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
#include <unistd.h>
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

/* From ${GEM5}/src/mem/request.hh */
static const uint32_t LOCKED                      = 0x00100000;
static const uint32_t UNCACHED                    = 0x00001000;

PortLink::PortLink( M5& comp, Gem5Object_t& obj, const SST::Params& params ) :
	m_doTranslate( false ),
    m_comp( comp ),
    m_myEndPoint( this, recv, poke )
{
    bool snoopOut = false, snoopIn = false;
    std::string name = params.find_string("name");
    m_name = name;
    assert( ! name.empty() );
    std::string portName = params.find_string("portName");
    assert( ! portName.empty() );
    int idx = params.find_integer( "portIndex" );
    uint64_t addrStart = 0, addrEnd = -1;
	unsigned blocksize = 0;

    snoopOut = false;
    snoopIn = true;

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
    if(!m_link) {
        std::cout << "There is an issue connecting link: " << name << std::endl;
    }
    assert( m_link );

    m_gem5EndPoint = GetPort(obj, m_myEndPoint, snoopOut, snoopIn, 
                            addrStart, addrEnd, blocksize, name, portName, idx ); 

    sent = 0, received = 0;
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
    //std::cout << "----Incoming: 0x" << std::hex << sstev->getAddr() << " Cmd: " << sstev->getCmd() << std::endl;
	for ( std::list<MemPkt*>::iterator i = m_g5events.begin() ; i != m_g5events.end() ; ++i ) {
		MemPkt *ev = *i;
        //std::cout << "In Queue: 0x" << std::hex << ev->addr << " Cmd: " << ev->cmd << std::endl;
		/* TODO:  More intelligent matching? */
		if ( ev->addr == sstev->getAddr() ) {
			g5ev = ev;
			m_g5events.erase(i);
         //assert(ev->size == sstev->getSize());
			break;
		}
	}
    //if(g5ev == NULL) _abort(PortLink, "g5ev NULL??!?!");
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
	case SST::Interfaces::GetSResp:
		pkt->cmd = ::MemCmd::ReadResp;
		pkt->isResponse = true;
	    pkt->size = sstev->getSize();
	    memcpy(pkt->data, &(sstev->getPayload()[0]), pkt->size);
		break;
	case SST::Interfaces::GetXResp:
		pkt->cmd = ::MemCmd::WriteResp;
		pkt->isResponse = true;
		break;
	default:
		_abort(Gem5Converter, "Trying to map SST cmd %s to GEM5\n", CommandString[sstev->getCmd()]);
		break;
	}

	assert(pkt->size <= (unsigned)MemPkt::DataSize);
    
    dbg->debug(CALL_INFO,0,0,"---->\n");
    dbg->debug(CALL_INFO,0,0,"Sent Event. Addr = %"PRIu64", Req Size =%u, Src =%s, QueueSize = %lu, Sent = %i\n", pkt->addr, sstev->getSize(), sstev->getSrc().c_str(), m_g5events.size(), ++sent);
    if(m_g5events.size() > 0) dbg->debug(CALL_INFO,0,0," First Addr in Queue = %"PRIu64"\n", m_g5events.front()->addr);

    if(sstev->getCmd() == SST::Interfaces::GetSResp)    dbg->debug(CALL_INFO,0,0," Read Request done. D: ");
    else dbg->debug(CALL_INFO,0,0," Write Request done. W: ");

    for(unsigned int i = 0; i < pkt->size; i++) dbg->debug(CALL_INFO,0,0,"%x", (int)pkt->data[i]);
    dbg->debug(CALL_INFO,0,0,"\n");
    dbg->debug(CALL_INFO,0,0,"<----\n");
    

	/* Swap src/dst */
	int tmp = pkt->src;
	pkt->src = pkt->dest;
	pkt->dest = tmp;

	/* TODO:  finishTime ?   firstWordTime ?  pktId ? */
	return pkt;
}


SST::Interfaces::MemEvent* PortLink::convertGEM5toSST( MemPkt *pkt )
{
    SST::Interfaces::MemEvent *ev = new SST::Interfaces::MemEvent(&m_comp, pkt->addr, SST::Interfaces::NULLCMD);
    ev->setSize(pkt->size);
    bool readEx = false;

    /* From ${GEM5}/src/mem/request.hh */

    if ( pkt->req.flags & UNCACHED ) {
        ev->setFlags(SST::Interfaces::MemEvent::F_UNCACHED);
    }
    else if (UNLIKELY(pkt->req.flags & LOCKED)) {
        ev->setFlags(SST::Interfaces::MemEvent::F_LOCKED);
        ev->setLockID(((uint64_t)(pkt->req.contextId) <<32) | (uint64_t)(pkt->req.threadId));
        readEx = true;
    }

    switch ( (::MemCmd::Command)pkt->cmd) {
    case ::MemCmd::ReadExReq:
        _abort(Gem5Converter, "Not Supported, ReadExReq");
    case ::MemCmd::ReadReq:
        ev->setCmd(SST::Interfaces::GetS);
        if(UNLIKELY(readEx)) ev->setCmd(SST::Interfaces::GetSEx);
        break;
    case ::MemCmd::WriteReq:
        ev->setCmd(SST::Interfaces::GetX);
        ev->setPayload(pkt->size, pkt->data);
        break;
    case ::MemCmd::Writeback:
        _abort(Gem5Converter, "Not Supported, Writeback");
        ev->setCmd(SST::Interfaces::WriteReq);
        ev->setFlags(SST::Interfaces::MemEvent::F_WRITEBACK);
        ev->setPayload(pkt->size, pkt->data);
        break;
    default:
        _abort(Gem5Converter, "Don't know how to convert command %d to SST\n", pkt->cmd);
    }
    m_g5events.push_back(pkt);
    
    dbg->debug(CALL_INFO,0,0,"---->\n");
    dbg->debug(CALL_INFO,0,0,"Received Event. Queue Size: %lu, Addr: %"PRIu64",  ReqSize = %u", m_g5events.size(), pkt->addr, ev->getSize());
    if(ev->getCmd() == SST::Interfaces::GetS) dbg->debug(CALL_INFO,0,0,", R\n");
    else if(ev->getCmd() == SST::Interfaces::GetSEx) dbg->debug(CALL_INFO,0,0,", Read Exclusive\n");
    else {
        dbg->debug(CALL_INFO,0,0," W, D: ");
        for(unsigned int i = 0; i < pkt->size; i++) dbg->debug(CALL_INFO,0,0,"%x", (int)pkt->data[i]);
        dbg->debug(CALL_INFO,0,0,"\n");
    }
    dbg->debug(CALL_INFO,0,0,"<----\n");
    
    ev->setSrc(m_name);
    
	//fprintf(stderr, "%s:%d %s: Storing patcket %zu\n", __FILE__, __LINE__, __FUNCTION__, pkt->pktId);
	return ev;
}

void PortLink::printQueueSize(){
    dbg->debug(CALL_INFO,0,0, "Name: %s",  m_name.c_str());
    if(m_g5events.size() > 0) dbg->debug(CALL_INFO,0,0,"Size: %d. Top Event in Queue = %llx \n",m_g5events.size(), (uint64_t)m_g5events.front()->addr);
    else dbg->debug(CALL_INFO,0,0,"\n");
}

}}

