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
#include "portLink.h"
#include <unistd.h>
#include <sst/core/component.h>
#include <sst/core/interfaces/simpleMem.h>
#include <sst/core/interfaces/stringEvent.h>

#include <debug.h>
#include <m5.h>
#include <dll/gem5dll.hh>
#include <dll/memEvent.hh>
#include <rawEvent.h>

using namespace SST::Interfaces;

namespace SST {
namespace M5 {

/* From ${GEM5}/src/mem/request.hh */
static const uint32_t LOCKED                      = 0x00100000;
static const uint32_t UNCACHED                    = 0x00001000;
static const uint32_t LLSC                        = 0x00200000;

PortLink::PortLink( M5& comp, Gem5Object_t& obj, const SST::Params& params ) :
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

    m_sstInterface = dynamic_cast<SimpleMem*>(comp.loadModuleWithComponent("memHierarchy.memInterface", &m_comp, const_cast<SST::Params&>(params)));
    m_sstInterface->initialize(name, new SimpleMem::Handler<PortLink>( this, &PortLink::eventHandler ) );
    comp.registerPortLink(name, this);

    m_gem5EndPoint = GetPort(obj, m_myEndPoint, snoopOut, snoopIn,
                            addrStart, addrEnd, blocksize, name, portName, idx );

    sent = 0, received = 0;
    assert( m_gem5EndPoint );
}


void PortLink::setup(void) {
	SST::Event *ev = NULL;
	while ( (ev = m_sstInterface->recvInitData()) != NULL ) {
		delete ev;
	}
}


void PortLink::eventHandler( SimpleMem::Request *e )
{

    DBGX(2,"PortLink::eventHandler()\n");
	RawEvent *event = new RawEvent(convertSSTtoGEM5(e), sizeof(MemPkt));
    delete e;

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

    delete event;
}



MemPkt* PortLink::findMatchingEvent(SimpleMem::Request *e)
{
	MemPkt* g5ev = NULL;

    std::map<uint64_t, MemPkt*>::iterator i = m_g5events.find(e->id);
    if ( i != m_g5events.end() ) {
        g5ev = i->second;
        m_g5events.erase(i);
    }

	return g5ev;
}


MemPkt* PortLink::convertSSTtoGEM5( SimpleMem::Request *e )
{
	MemPkt* pkt = findMatchingEvent(e);
	if ( !pkt ) {
		_abort(Gem5Converter, "Trying to convert SST even for which no matching GEM5 event is found.\n");
	}

	switch ( e->cmd ) {
        case SimpleMem::Request::ReadResp:
            pkt->cmd = ::MemCmd::ReadResp;
            pkt->isResponse = true;
            pkt->size = e->size;
            memcpy(pkt->data, &(e->data[0]), pkt->size);
            break;
        case SimpleMem::Request::WriteResp:
            pkt->cmd = ::MemCmd::WriteResp;
            pkt->isResponse = true;
            pkt->req.flags |= LLSC;
            if(e)
            break;
        default:
            _abort(Gem5Converter, "Trying to map SST cmd %d to GEM5\n", e->cmd);
            break;
	}

	assert(pkt->size <= (unsigned)MemPkt::DataSize);

    dbg->debug(CALL_INFO,0,0,"---->\n");
    dbg->debug(CALL_INFO,0,0,"Sent Event. Addr = %"PRIx64", Req Size =%zu, QueueSize = %lu, Sent = %i\n", pkt->addr, e->size, m_g5events.size(), ++sent);
    for(std::map<uint64_t, MemPkt*>::iterator it = m_g5events.begin(); it != m_g5events.end(); it++){
        dbg->debug(CALL_INFO,0,0,"Queue Addr = %"PRIx64"\n", (*it).second->addr);
    }
    dbg->debug(CALL_INFO,0,0,"<----\n");


	/* Swap src/dst */
	int tmp = pkt->src;
	pkt->src = pkt->dest;
	pkt->dest = tmp;
	return pkt;
}


SimpleMem::Request* PortLink::convertGEM5toSST( MemPkt *pkt )
{
    SimpleMem::Request *req = new SimpleMem::Request(SimpleMem::Request::Read, pkt->addr, pkt->size);

    /* From ${GEM5}/src/mem/request.hh */

    if ( pkt->req.flags & UNCACHED )            req->flags |=(SimpleMem::Request::F_UNCACHED);
    else if (UNLIKELY(pkt->req.flags & LOCKED)) req->flags |= (SimpleMem::Request::F_LOCKED);
    else if (UNLIKELY(pkt->req.flags & LLSC))   req->flags |= (SimpleMem::Request::F_LLSC);

    switch ( (::MemCmd::Command)pkt->cmd) {
        case ::MemCmd::ReadReq:
            req->cmd = SimpleMem::Request::Read;
            break;
        case ::MemCmd::WriteReq:
            req->cmd = SimpleMem::Request::Write;
            req->setPayload(pkt->data, pkt->size);
            break;
        default:
            _abort(Gem5Converter, "Don't know how to convert command %d to SST\n", pkt->cmd);
    }
    m_g5events[req->id] = pkt;

    dbg->debug(CALL_INFO,0,0,"---->\n");
    dbg->debug(CALL_INFO,0,0,"Received Event.  Addr: %"PRIx64", Queue Size: %zu,  ReqSize = %zu \n", pkt->addr, m_g5events.size(), req->size);
    for(std::map<uint64_t, MemPkt*>::iterator it = m_g5events.begin(); it != m_g5events.end(); it++){
        dbg->debug(CALL_INFO,0,0,"Queue Addr = %"PRIx64"\n", (*it).second->addr);
    }
    dbg->debug(CALL_INFO,0,0,"<----\n");
    received++;
    //std::cout << "TotalRequests: " << received << std::endl;
	return req;
}

void PortLink::printQueueSize(){
    dbg->debug(CALL_INFO,0,0, "Name: %s",  m_name.c_str());
    dbg->debug(CALL_INFO,0,0, "Size: %zu.\n",m_g5events.size());
    for(std::map<uint64_t, MemPkt*>::iterator it = m_g5events.begin(); it != m_g5events.end(); it++){
        dbg->debug(CALL_INFO,0,0,"Queue Addr = %"PRIx64"\n", (*it).second->addr);
    }
}

}}

