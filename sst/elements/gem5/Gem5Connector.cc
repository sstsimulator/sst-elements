// Copyright 2009-2015 Sandia Coporation.  Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package.  For lucense
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include <sst/core/serialization.h>

#include "Gem5Connector.h"

#include <sim/sim_object.hh>
#include <mem/packet.hh>
#include <mem/request.hh>
#include <mem/ext_connector.hh>

#include <sst/core/params.h>
#include <sst/core/output.h>
#include <sst/core/link.h>
#include <sst/core/interfaces/simpleMem.h>

#include "gem5.h"

#ifdef fatal  // Gem5 sets this
#undef fatal
#endif

using namespace SST;
using namespace SST::gem5;
using namespace SST::Interfaces;


static void g5RecvHandler(::PacketPtr pkt, void *obj)
{
    Gem5Connector* conn = static_cast<Gem5Connector*>(obj);
    conn->handleRecvFromG5(pkt);
}


Gem5Connector::Gem5Connector(Gem5Comp *g5Comp, Output &out,
        ::SimObject *g5obj, std::string &linkName) :
    comp(g5Comp), out(out), simPhase(CONSTRUCTION)
{
    conn = dynamic_cast<ExtConnector*>(g5obj);
    if ( !conn ) out.fatal(CALL_INFO, 1, "SimObject not a ExtConnector\n");
    if ( linkName == "system.mem_ctrls.connector" ) conn->setDebug( true );

    SST::Params params;
    sstlink = static_cast<SimpleMem*>(comp->loadModuleWithComponent("memHierarchy.memInterface", comp, params));
    bool ok = sstlink->initialize(linkName,
            new SimpleMem::Handler<Gem5Connector>(this, &Gem5Connector::handleRecvFromSST));

    if ( !ok ) {
        out.fatal(CALL_INFO, 1, "Failed to configure link %s\n", linkName.c_str());
    }

    conn->setRecvHandler(g5RecvHandler, this);

}

void Gem5Connector::init(unsigned int phase)
{
    simPhase = INIT;
    if ( !initBlobs.empty() ) {
        for ( std::vector<Blob>::iterator i = initBlobs.begin(); i != initBlobs.end(); ++i ) {
            SimpleMem::Request *req = new SimpleMem::Request(SimpleMem::Request::Write, i->addr, i->size);
            req->setPayload(i->data);
            sstlink->sendInitData(req);
        }
        initBlobs.clear();
    }
}

void Gem5Connector::setup(void)
{
    simPhase = RUN;
}


void Gem5Connector::handleRecvFromG5(::PacketPtr pkt)
{
    if ( CONSTRUCTION == simPhase ) {
        ::MemCmd::Command pktCmd = (::MemCmd::Command)pkt->cmd.toInt();
        assert(pktCmd == ::MemCmd::WriteReq || pktCmd == ::MemCmd::Writeback);
        initBlobs.push_back(Blob(pkt->getAddr(), pkt->getSize(), pkt->getPtr<uint8_t>()));
        delete pkt;
        return;
    }


    SimpleMem::Request *req = new SimpleMem::Request(SimpleMem::Request::Read, pkt->getAddr(), pkt->getSize());
    req->setPayload(pkt->getPtr<uint8_t>(), pkt->getSize());

    if(pkt->req->isLocked()) {
        req->flags |= (SimpleMem::Request::F_LOCKED);
    }

    if(pkt->req->isUncacheable()) {
        req->flags |= (SimpleMem::Request::F_NONCACHEABLE);
    }

    switch ( (::MemCmd::Command)pkt->cmd.toInt() ) {
	case ::MemCmd::ReadReq:
	case ::MemCmd::ReadExReq:
		req->cmd = SimpleMem::Request::Read;
		break;
	case ::MemCmd::WriteReq:
		req->cmd = SimpleMem::Request::Write;
		break;
    case ::MemCmd::LoadLockedReq:
        req->cmd = SimpleMem::Request::Read;
        req->flags |= SimpleMem::Request::F_LLSC;
        break;
    case ::MemCmd::StoreCondReq:
        req->cmd = SimpleMem::Request::Write;
        req->flags |= SimpleMem::Request::F_LLSC;
        break;
        
    default:
        out.fatal(CALL_INFO, 1, "Don't know how to convert GEM5 command %s to SST\n", pkt->cmd.toString().c_str());
    }


    if ( INIT == simPhase ) {
        sstlink->sendInitData(req);
        delete pkt;
    } else {
        if ( pkt->needsResponse() )
            g5packets[req->id] = pkt;

        sstlink->sendRequest(req);
    }

}


void Gem5Connector::handleRecvFromSST(SimpleMem::Request *event)
{
    SimpleMem::Request::id_t origID = event->id;

    PacketMap_t::iterator mi = g5packets.find(origID);
    assert(mi != g5packets.end());

    PacketPtr pkt = mi->second;
    g5packets.erase(mi);

    pkt->makeResponse();  // Convert to a response packet
    pkt->setData(event->data.data());

    // Did it succeed?
    if (pkt->isLLSC() && pkt->isWrite()){
        pkt->req->setExtraData(!(event->flags & SimpleMem::Request::F_LLSC_RESP));
    }
    
        
    // Clear out bus delay notifications
    pkt->busFirstWordDelay = pkt->busLastWordDelay = 0;


    bool ok = conn->sendResponsePacket(pkt);
    assert(ok); // TODO:  Handle retry?

    delete event;
}

