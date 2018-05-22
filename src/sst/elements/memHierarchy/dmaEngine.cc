// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include "dmaEngine.h"

#include <sst/core/component.h>
#include <sst/core/params.h>

using namespace SST;
using namespace SST::MemHierarchy;

uint64_t DMACommand::main_id = 0;

DMAEngine::DMAEngine(ComponentId_t id, Params &params) :
    Component(id)
{
    dbg.init("@t:DMAEngine::@p():@l " + getName() + ": ", 0, 0,
            (Output::output_location_t)params.find<int>("debug", 0));
    statsOutputTarget = (Output::output_location_t)params.find<int>("printStats", 0);

    TimeConverter *tc = registerClock(params.find<std::string>("clockRate", "1 GHz"),
            new Clock::Handler<DMAEngine>(this, &DMAEngine::clock));
    commandLink = configureLink("cmdLink", tc, NULL);
    if ( NULL == commandLink ) dbg.fatal(CALL_INFO, 1, "Missing cmdLink\n");

    if ( !isPortConnected("netLink") ) dbg.fatal(CALL_INFO, 1, "Missing netLink\n");
/*
    MemNIC::ComponentInfo myInfo;
    myInfo.link_port = "netLink";
    myInfo.link_bandwidth = "1GiB/s";
	myInfo.num_vcs = params.find<int>("network_num_vc", 1);
    myInfo.name = getName();
    myInfo.type = MemNIC::TypeDMAEngine;
    networkLink = new MemNIC(this, &dbg, -1, myInfo);
*/
    blocksize = 0;
}


void DMAEngine::init(unsigned int phase)
{
    networkLink->init(phase);
}


void DMAEngine::setup(void)
{
    networkLink->setup();
    bool blocksizeSet = false;

/*    const std::vector<MemNIC::PeerInfo_t> &peers = networkLink->getPeerInfo();
    for ( std::vector<MemNIC::PeerInfo_t>::const_iterator i = peers.begin() ; i != peers.end() ; ++i ) {
        switch (i->first.type) {
        case MemNIC::TypeDirectoryCtrl:
            break;
        case MemNIC::TypeCache:
            if ( !blocksizeSet ) {
                blocksize = i->second.blocksize;
                blocksizeSet = true;
            }
        default:
            ;
        }
    }

    networkLink->clearPeerInfo();*/
}


void DMAEngine::finish(void)
{
    Output out("", 0, 0, statsOutputTarget);
    out.output("DMA Controller %s stats:\n"
            "\t # Transfers:        %" PRIu64 "\n"
            "\t Bytes Transferred:  %" PRIu64 "\n",
            getName().c_str(),
            numTransfers,
            bytesTransferred);
}


bool DMAEngine::clock(Cycle_t cycle)
{
    /* Process Network
     * Check Command link
     * If new command, check overlap, and delay if needed, otherwise process
     */

    //networkLink->clock();

    MemEventBase *me = NULL;
    SST::Event *se = NULL;

//    while ( NULL != (me = networkLink->recv()) ) {
        /* Process network packet */
/*        Request* req = findRequest(me->getResponseToID());
#ifdef __SST_DEBUG_OUTPUT__
        if ( NULL == req ) {
            dbg.debug(_L10_, "Received Packet for which we have no response ID waiting.  ID received: (%" PRIx64 ", %d)\n", me->getResponseToID().first, me->getResponseToID().second);
        }
#endif
        processPacket(req, me);
    }
*/

    while ( NULL != (se = commandLink->recv()) ) {
        /* Process new commands */
        DMACommand* cmd = static_cast<DMACommand*>(se);
        commandQueue.push_back(cmd);
    }

    /* See if we can process the next command */
    if ( !commandQueue.empty() ) {
        DMACommand *cmd = commandQueue.front();
        if ( isIssuable(cmd) ) {
            commandQueue.pop_front();
            Request *req = new Request(cmd);
            startRequest(req);
        }
    }

    return false;
}


bool DMAEngine::isIssuable(DMACommand *cmd) const
{
    bool isOK = true;
    /* Cycle through current requests.  If any overlap, then we should wait. */
    for ( std::set<Request*>::iterator i = activeRequests.begin() ; isOK && i != activeRequests.end() ; ++i ) {
        Request *req = (*i);
        isOK = !findOverlap(req->command, cmd);
    }

    return isOK;
}


void DMAEngine::startRequest(Request *req)
{
    assert(0);
    /*  Needs to be updated with current MemHierarchy Commands/States, MemHierarchyInterface
    dbg.debug(_L10_, "Received request to transfer from %#" PRIx64 " to 0x%" PRIx64 "\n",
            req->getSrc(), req->getDst());
    ++numTransfers;
    Addr ptr = req->getSrc();
    while ( ptr < (req->getSrc() + req->getSize()) ) {
        MemEvent *ev = new MemEvent(this, ptr, ptr, GetS);
        ev->setSize(blocksize);
        ev->setFlag(MemEvent::F_NONCACHEABLE);
        ev->setDst(networkLink->findTargetDestination(ptr));
        req->loadKeys.insert(ev->getID());
        networkLink->send(ev);
        ptr += blocksize;
    }
    activeRequests.insert(req);
    */
}


void DMAEngine::processPacket(Request *req, MemEventBase *ev)
{
    assert(0);
    /*  Needs to be updated with current MemHierarchy Commands/States, MemHierarchyInterface
    if ( ev->getCmd() == GetSResp) {
        if ( !req->loadKeys.count(ev->getResponseToID()) ) {
            dbg.fatal(CALL_INFO, 1, "Received GetSResp, but we don't have a corresponding request in loadKeys!\n");
        }
        size_t offset = ev->getAddr() - req->getSrc();
        req->loadKeys.erase(ev->getResponseToID());
        MemEvent *storeEV = new MemEvent(this, (req->getDst() + offset), (req->getDst() + offset), GetX);
        storeEV->setFlag(MemEvent::F_NONCACHEABLE);
        storeEV->setPayload(ev->getPayload());
        storeEV->setDst(networkLink->findTargetDestination(req->getDst() + offset));
        req->storeKeys.insert(storeEV->getID());
        networkLink->send(storeEV);
    } else if ( ev->getCmd() == GetXResp ) {
        bytesTransferred += ev->getSize();
        req->storeKeys.erase(ev->getResponseToID());
        if ( req->storeKeys.empty() ) {
            // Done with this request.
            activeRequests.erase(req);
            commandLink->send(req->command);
            dbg.debug(_L10_, "Request to transfer 0x%" PRIx64 " to 0x%" PRIx64 " is complete.\n", req->getSrc(), req->getDst());
            delete req;
        }
    } else {
	dbg.fatal(CALL_INFO, 1, "Received unexpected message %s 0x%" PRIx64 " from %s\n", CommandString[ev->getCmd()], ev->getAddr(), ev->getSrc().c_str());
    }
    */
}


/* Returns true if there is overlap */
bool DMAEngine::findOverlap(DMACommand *c1, DMACommand *c2) const
{
    return (findOverlap(c1->src, c1->size, c2->src, c2->size) ||
            findOverlap(c1->src, c1->size, c2->dst, c2->size) ||
            findOverlap(c1->dst, c1->size, c2->src, c2->size) ||
            findOverlap(c1->dst, c1->size, c2->dst, c2->size));
}


/* Returns true if there is overlap */
bool DMAEngine::findOverlap(Addr a1, size_t s1, Addr a2, size_t s2) const
{
    Addr end1 = a1 + s1;
    Addr end2 = a2 + s2;

    return (( a1 <= end2 ) || ( a2 <= end1 ));
}


DMAEngine::Request* DMAEngine::findRequest(SST::Event::id_type id)
{
    for ( std::set<Request*>::iterator i = activeRequests.begin() ; i != activeRequests.end() ; ++i ) {
        Request *req = (*i);
        if ( req->loadKeys.count(id) || req->storeKeys.count(id) )
            return req;
    }
    return NULL;
}


