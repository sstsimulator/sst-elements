// Copyright 2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>

#include "sst_config.h"
#include "sst/core/serialization/element.h"
#include "sst/core/simulation.h"
#include <assert.h>

#include "sst/core/element.h"

#include "dircontroller.h"


#define DPRINTF( fmt, args...) __DBG( DBG_CACHE, DirectoryController, "%s: " fmt, getName().c_str(), ## args )

using namespace SST;
using namespace SST::MemHierarchy;


DirectoryController::DirectoryController(ComponentId_t id, Params_t &params) :
    Component(id), blocksize(0)
{
    targetCount = 0;

	registerTimeBase("1 ns", true);

	memLink = configureLink("memory", "1 ns",
			new Event::Handler<DirectoryController>(this,
				&DirectoryController::handleMemoryResponse));
	assert(memLink);


    int addr = params.find_integer("network_addr");
    std::string net_bw = params.find_string("network_bw");

	addrRangeStart = (uint64_t)params.find_integer("addrRangeStart", 0);
	addrRangeEnd = (uint64_t)params.find_integer("addrRangeEnd", 0);
	if ( addrRangeEnd == 0 ) addrRangeEnd = (uint64_t)-1;
	interleaveSize = (Addr)params.find_integer("interleaveSize", 0);
    interleaveSize *= 1024;
	interleaveStep = (Addr)params.find_integer("interleaveStep", 0);
    interleaveStep *= 1024;

    numTargets = 0;


    MemNIC::ComponentInfo myInfo;
    myInfo.link_port = "network";
    myInfo.link_bandwidth = net_bw;
    myInfo.name = getName();
    myInfo.network_addr = addr;
    myInfo.type = MemNIC::TypeDirectoryCtrl;
    myInfo.typeInfo.dirctrl.rangeStart = addrRangeStart;
    myInfo.typeInfo.dirctrl.rangeEnd = addrRangeEnd;
    network = new MemNIC(this, myInfo,
            new Event::Handler<DirectoryController>(this,
                &DirectoryController::handlePacket));



	registerClock(params.find_string("clock", "1GHz"),
			new Clock::Handler<DirectoryController>(this, &DirectoryController::clock));

    lookupBaseAddr = params.find_integer("backingStoreSize", 0x1000000); // 16MB
}


void DirectoryController::handleMemoryResponse(SST::Event *event)
{
	MemEvent *ev = dynamic_cast<MemEvent*>(event);
	if ( ev ) {
        DPRINTF("Memory response for address 0x%"PRIx64"\n", ev->getAddr());

        if ( memReqs.find(ev->getResponseToID()) != memReqs.end() ) {
            Addr targetBlock = memReqs[ev->getResponseToID()];
            memReqs.erase(ev->getResponseToID());
            if ( ev->getCmd() == SupplyData ) {
                // Lookup complete, perform our work
                DirEntry *entry = getDirEntry(targetBlock);
                assert(entry);
                assert(entry->nextFunc);
                (this->*(entry->nextFunc))(entry, ev);
            } else if ( ev->getCmd() == WriteResp ) {
                // Final update complete.  Clear our status
                DirEntry *entry = getDirEntry(targetBlock);
                assert(entry);
                if ( entry->activeReq ) {
                    resetEntry(entry);
                }
                if ( entry->countRefs() == 0 ) {
                    // Is empty, let's purge
                    DPRINTF("Entry for 0x%"PRIx64" has no referenes - purging\n", targetBlock);
                    delete entry;
                }
            } else {
                _abort(DirectoryController, "Received unexpected message from Memory!\n");
            }
        } else {
            /* Don't have this req recorded */
        }


		delete ev;
	} else {
		_abort(DirectoryController, "Received unknown packet!\n");
	}
}


void DirectoryController::handlePacket(SST::Event *event)
{
    MemEvent *ev = static_cast<MemEvent*>(event);
    assert(isRequestAddressValid(ev));
    DPRINTF("Received %s 0x%"PRIx64" from %s\n", CommandString[ev->getCmd()], ev->getAddr(), ev->getSrc().c_str());

    /* Make sure we know about the sender */
    registerSender(ev->getSrc());
    switch(ev->getCmd()) {
    case ACK:
        handleACK(ev);
        delete ev;
        break;
    case SupplyData: {
        /* May be a response to a Fetch/FetchInvalidate
         * May be a writeback
         */
        DirEntry *entry = getDirEntry(ev->getAddr());
        assert(entry);
        if ( entry->inProgress() ) {
            // Message should only be coming from the exclusive owner
            /* TODO:  Handle potential race of owner doing an eviction at same time */
            DPRINTF("Entry 0x%"PRIx64" in progress.  Advancing.\n", entry->baseAddr);
            advanceEntry(entry, ev);
        } else {
            DPRINTF("Entry 0x%"PRIx64" not in progress.  Requesting from memory.\n", entry->baseAddr);
            entry->activeReq = ev;
            entry->nextFunc = &DirectoryController::handleExclusiveEviction;
            requestDirEntryFromMemory(entry);
        }
        break;
    }
    case RequestData: {
        DirEntry *entry = getDirEntry(ev->getAddr());
        if ( !entry ) {
            entry = createDirEntry(ev->getAddr());
        }
        if ( entry->inProgress() ) {
            workQueue.push_back(ev);
        } else {
            DPRINTF("Entry 0x%"PRIx64" not in progress.  Requesting from memory.\n", entry->baseAddr);
            entry->activeReq = ev;
            entry->nextFunc = &DirectoryController::handleRequestData;
            requestDirEntryFromMemory(entry);
        }
        break;
    }
    case Invalidate: {
        DirEntry *entry = getDirEntry(ev->getAddr());
        assert(entry);
        if ( entry->inProgress() ) {
            // Put it on the queue;
            workQueue.push_back(ev);
        } else {
            entry->activeReq = ev;
            entry->nextFunc = &DirectoryController::handleInvalidate;
            requestDirEntryFromMemory(entry);
        }
        break;
    }
    default:
        /* Ignore unexpected */
        delete ev;
    }
}


void DirectoryController::init(unsigned int phase)
{
    network->init(phase);

    /* Pass data on to memory */
    while ( SST::Event *ev = network->recvInitData() ) {
        MemEvent *me = dynamic_cast<MemEvent*>(ev);
        if ( me ) {
            if ( isRequestAddressValid(me) ) {
                me->setAddr(convertAddressToLocalAddress(me->getAddr()));
                memLink->sendInitData(me);
            }
        } else {
            delete ev;
        }
    }

}


void DirectoryController::setup(void)
{
    network->setup();

    const std::vector<MemNIC::ComponentInfo> &ci = network->getPeerInfo();
    for ( std::vector<MemNIC::ComponentInfo>::const_iterator i = ci.begin() ; i != ci.end() ; ++i ) {
        DPRINTF("DC found peer %d (%s) of type %d.\n", i->network_addr, i->name.c_str(), i->type);
        if ( i->type == MemNIC::TypeCache ) {
            numTargets++;
            if ( blocksize ) {
                assert(blocksize == i->typeInfo.cache.blocksize);
            } else {
                blocksize = i->typeInfo.cache.blocksize;
            }
        }
    }
    if ( numTargets == 0 ) {
        _abort(DirectoryController, "Directory Controller %s unable to see any caches.\n", getName().c_str());
    }

    network->clearPeerInfo();
}


bool DirectoryController::clock(SST::Cycle_t cycle)
{
    network->clock();
	for ( std::list<MemEvent*>::iterator i = workQueue.begin() ; i != workQueue.end() ; ++i ) {
		DirEntry *entry = getDirEntry((*i)->getAddr());
		if ( !entry || entry->activeReq == NULL ) {
			// This one can now be processed.
			handlePacket(*i);
			workQueue.erase(i);

			// Let's only process one per cycle
			break;
		}
	}

	return false;
}



DirectoryController::DirEntry* DirectoryController::getDirEntry(Addr addr)
{
	std::map<Addr, DirEntry*>::iterator i = directory.find(addr);
	if ( i == directory.end() ) return NULL;
	return i->second;
}


DirectoryController::DirEntry* DirectoryController::createDirEntry(Addr addr)
{
    DPRINTF("Creating Directory Entry for 0x%"PRIx64"\n", addr);
	DirEntry *entry = new DirEntry(addr, numTargets);
	directory[addr] = entry;
	return entry;
}



void DirectoryController::handleACK(MemEvent *ev)
{
	DirEntry *entry = getDirEntry(ev->getAddr());
	assert(entry);
	assert(entry->waitingAcks > 0);
	entry->waitingAcks--;
	if ( entry->waitingAcks == 0 ) {
		advanceEntry(entry);
	}
}


void DirectoryController::handleInvalidate(DirEntry *entry, MemEvent *new_ev)
{
	uint32_t id = node_id(entry->activeReq->getSrc());
	for ( uint32_t i = 0 ; i < numTargets ; i++ ) {
		if ( i == id ) continue;
		if ( entry->sharers[i] ) {
			sendInvalidate(i, entry->baseAddr);
			entry->waitingAcks++;
		}
	}
	if ( entry->waitingAcks > 0 ) {
		entry->nextFunc = &DirectoryController::finishInvalidate;
	} else {
		finishInvalidate(entry, NULL);
	}
}


void DirectoryController::finishInvalidate(DirEntry *entry, MemEvent *new_ev)
{
	assert(entry->waitingAcks == 0);

	uint32_t target_id = node_id(entry->activeReq->getSrc());
	for ( uint32_t i = 0 ; i < numTargets ; i++ ) {
		entry->sharers[i] = false;
	}
	entry->sharers[target_id] = true;
	entry->dirty = true;

	MemEvent *ev = entry->activeReq->makeResponse(this);
	sendResponse(ev);

	updateEntryToMemory(entry);
}


void DirectoryController::sendInvalidate(int target, Addr addr)
{
    MemEvent *me = new MemEvent(this, addr, Invalidate);
    me->setDst(nodeid_to_name[target]);
    network->send(me);
}



void DirectoryController::handleRequestData(DirEntry* entry, MemEvent *new_ev)
{
	uint32_t requesting_node = node_id(entry->activeReq->getSrc());
	if ( entry->dirty ) {
		// Must do a fetch
		Command cmd = Fetch;
		if ( entry->activeReq->queryFlag(MemEvent::F_EXCLUSIVE) ) {
			cmd = FetchInvalidate;
		}
		MemEvent *ev = new MemEvent(this, entry->baseAddr, cmd);
		sendResponse(ev);

		entry->nextFunc = &DirectoryController::finishFetch;

	} else if ( entry->activeReq->queryFlag(MemEvent::F_EXCLUSIVE) ) {
		// Must send invalidates
		assert(entry->waitingAcks == 0);
		for ( uint32_t i = 0 ; i < numTargets ; i++ ) {
			if ( i == requesting_node ) continue;
			if ( entry->sharers[i] ) {
				sendInvalidate(i, entry->baseAddr);
				entry->waitingAcks++;
			}
		}
		if ( entry->waitingAcks > 0 ) {
			entry->nextFunc = &DirectoryController::getExclusiveDataForRequest;
		} else {
			getExclusiveDataForRequest(entry, NULL);
		}
	} else {
		// Just a simple share
		entry->sharers[requesting_node]= true;
		entry->nextFunc = &DirectoryController::sendRequestedData;
		requestDataFromMemory(entry);
	}
}

void DirectoryController::finishFetch(DirEntry* entry, MemEvent *new_ev)
{
	if ( entry->activeReq->queryFlag(MemEvent::F_EXCLUSIVE) ) {
		entry->dirty = true;
		for ( uint32_t i = 0 ; i < numTargets ; i++ ) {
			entry->sharers[i] = false;
		}
	} else {
		entry->dirty = false;
	}

	entry->sharers[node_id(entry->activeReq->getSrc())] = true;

	MemEvent *ev = entry->activeReq->makeResponse(this);
	ev->setPayload(new_ev->getPayload());
	sendResponse(ev);
	writebackData(new_ev);
	updateEntryToMemory(entry);
}



void DirectoryController::sendRequestedData(DirEntry* entry, MemEvent *new_ev)
{
	MemEvent *ev = entry->activeReq->makeResponse(this);
	ev->setPayload(new_ev->getPayload());
	sendResponse(ev);
	updateEntryToMemory(entry);
}


void DirectoryController::getExclusiveDataForRequest(DirEntry* entry, MemEvent *new_ev)
{
	assert(entry->waitingAcks == 0);

	uint32_t target_id = node_id(entry->activeReq->getSrc());
	for ( uint32_t i = 0 ; i < numTargets ; i++ ) {
		entry->sharers[i] = false;
	}
	entry->sharers[target_id] = true;
	entry->dirty = true;

	entry->nextFunc = &DirectoryController::sendRequestedData;
	requestDataFromMemory(entry);
}



void DirectoryController::handleExclusiveEviction(DirEntry *entry, MemEvent *ev)
{
    DPRINTF("Entry 0x%"PRIx64" loaded.  Performing writeback of 0x%"PRIx64"\n", entry->baseAddr, entry->activeReq->getAddr());
	entry->dirty = false;
	entry->sharers[node_id(ev->getSrc())] = false;
    writebackData(entry->activeReq);
	updateEntryToMemory(entry);
}


/* Advance the processing of this directory entry */
void DirectoryController::advanceEntry(DirEntry *entry, MemEvent *ev)
{
	assert(entry->nextFunc != NULL);
	(this->*(entry->nextFunc))(entry, ev);
}


void DirectoryController::registerSender(const std::string &name)
{
    node_id(name);
}



uint32_t DirectoryController::node_id(const std::string &name)
{
	uint32_t id;
	std::map<std::string, uint32_t>::iterator i = node_lookup.find(name);
	if ( i == node_lookup.end() ) {
		node_lookup[name] = id = targetCount++;
        nodeid_to_name.resize(targetCount);
        nodeid_to_name[id] = name;
	} else {
		id = i->second;
	}
	return id;
}


void DirectoryController::requestDirEntryFromMemory(DirEntry *entry)
{
	Addr entryAddr = 0; /*  Offset into our buffer? */
	MemEvent *me = new MemEvent(this, entryAddr, RequestData);
	me->setSize((numTargets+1)/8 +1);
    memReqs[me->getID()] = entry->baseAddr;
	memLink->send(me);
}


void DirectoryController::requestDataFromMemory(DirEntry *entry)
{
    DPRINTF("Requesting data from memory at 0x%"PRIx64"\n", entry->baseAddr);
    MemEvent *ev = new MemEvent(this, convertAddressToLocalAddress(entry->baseAddr), RequestData);
    ev->setSize(blocksize);
    memReqs[ev->getID()] = entry->baseAddr;
    memLink->send(ev);
}


void DirectoryController::updateEntryToMemory(DirEntry *entry)
{
	Addr entryAddr = 0; /*  Offset into our buffer? */
	MemEvent *me = new MemEvent(this, entryAddr, SupplyData);
	me->setSize((numTargets+1)/8 +1);
    memReqs[me->getID()] = entry->baseAddr;

	memLink->send(me);
}


void DirectoryController::writebackData(MemEvent *data_event)
{
    DPRINTF("Writing back data to 0x%"PRIx64"\n", data_event->getAddr());
	MemEvent *ev = new MemEvent(this, convertAddressToLocalAddress(data_event->getAddr()), SupplyData);
	ev->setFlag(MemEvent::F_WRITEBACK);
	ev->setPayload(data_event->getPayload());

	memLink->send(ev);
}


void DirectoryController::resetEntry(DirEntry *entry)
{
	delete entry->activeReq;
	entry->activeReq = NULL;
	entry->nextFunc = NULL;
}


void DirectoryController::sendResponse(MemEvent *ev)
{
	network->send(ev);
}


bool DirectoryController::isRequestAddressValid(MemEvent *ev)
{
    Addr addr = ev->getAddr();

    if ( interleaveSize == 0 ) {
        return ( addr >= addrRangeStart && addr < addrRangeEnd );
    } else {
        if ( addr < addrRangeStart ) return false;
        if ( addr > addrRangeEnd ) return false;

        addr = addr - addrRangeStart;

        Addr offset = addr % interleaveStep;
        if ( offset >= interleaveSize ) return false;

        return true;
    }

}


Addr DirectoryController::convertAddressToLocalAddress(Addr addr)
{
    Addr res = 0;
    if ( interleaveSize == 0 ) {
        res = lookupBaseAddr + addr - addrRangeStart;
    } else {
        addr = addr - addrRangeStart;
        Addr step = addr / interleaveStep;
        Addr offset = addr % interleaveStep;
        res = lookupBaseAddr + (step * interleaveSize) + offset;
    }
    DPRINTF("Converted physical address 0x%"PRIx64" to ACTUAL memory address 0x%"PRIx64"\n", addr, res);
    return res;
}
