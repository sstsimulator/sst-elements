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

#include <sst_config.h>
#include <sst/core/serialization.h>
#include "dircontroller.h"

#include <assert.h>

#include <sst/core/element.h>
#include <sst/core/params.h>
#include <sst/core/simulation.h>

#include "memNIC.h"


using namespace SST;
using namespace SST::MemHierarchy;


const MemEvent::id_type DirectoryController::DirEntry::NO_LAST_REQUEST = std::make_pair((uint64_t)-1, -1);

DirectoryController::DirectoryController(ComponentId_t id, Params &params) :
    Component(id), blocksize(0)
{
    dbg.init("@t:DirectoryController::@p():@l " + getName() + ": ", 0, 0, (Output::output_location_t)params.find_integer("debug", 0));
    printStatsLoc = (Output::output_location_t)params.find_integer("printStats", 0);

    targetCount = 0;

	registerTimeBase("1 ns", true);

	memLink = configureLink("memory", "1 ns",
			new Event::Handler<DirectoryController>(this,
				&DirectoryController::handleMemoryResponse));
	assert(memLink);


    entryCacheSize = (size_t)params.find_integer("entryCacheSize", 0);
    int addr = params.find_integer("network_addr");
    std::string net_bw = params.find_string("network_bw");

	addrRangeStart = (uint64_t)params.find_integer("addrRangeStart", 0);
	addrRangeEnd = (uint64_t)params.find_integer("addrRangeEnd", 0);
	if ( 0 == addrRangeEnd ) addrRangeEnd = (uint64_t)-1;
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
    myInfo.typeInfo.dirctrl.interleaveSize = interleaveSize;
    myInfo.typeInfo.dirctrl.interleaveStep = interleaveStep;
    network = new MemNIC(this, myInfo,
            new Event::Handler<DirectoryController>(this,
                &DirectoryController::handlePacket));



	registerClock(params.find_string("clock", "1GHz"),
			new Clock::Handler<DirectoryController>(this, &DirectoryController::clock));

    lookupBaseAddr = params.find_integer("backingStoreSize", 0x1000000); // 16MB

    numReqsProcessed = 0;
    totalReqProcessTime = 0;
    numCacheHits = 0;
}


DirectoryController::~DirectoryController()
{
    for ( std::map<Addr, DirEntry*>::iterator i = directory.begin(); i != directory.end() ; ++i ) {
        delete i->second;
    }
    directory.clear();
    while ( workQueue.size() ) {
        MemEvent *front = workQueue.front();
        workQueue.pop_front();
        delete front;
    }
}


void DirectoryController::handleMemoryResponse(SST::Event *event)
{
	MemEvent *ev = static_cast<MemEvent*>(event);
    dbg.output(CALL_INFO, "Memory response for address 0x%"PRIx64" (Response to (%"PRIu64", %d))\n", ev->getAddr(), ev->getResponseToID().first, ev->getResponseToID().second);

    if ( uncachedWrites.find(ev->getResponseToID()) != uncachedWrites.end() ) {
        MemEvent *origEV = uncachedWrites[ev->getResponseToID()];
        uncachedWrites.erase(ev->getResponseToID());

        // TODO:  Send response on upstream

        MemEvent *resp = origEV->makeResponse(this);
        sendResponse(resp);

        delete origEV;

    } else if ( memReqs.find(ev->getResponseToID()) != memReqs.end() ) {
        Addr targetBlock = memReqs[ev->getResponseToID()];
        memReqs.erase(ev->getResponseToID());

        if ( SupplyData == ev->getCmd() ) {
            // Lookup complete, perform our work
            DirEntry *entry = getDirEntry(targetBlock);
            assert(entry);
            entry->inController = true;
            advanceEntry(entry, ev);
        } else if ( WriteResp == ev->getCmd() ) {
            // Final update complete.  Clear our status
            DirEntry *entry = getDirEntry(targetBlock);
            if ( entry && !entry->activeReq ) {
                entry->inController = false;
                if ( 0 == entry->countRefs() ) {
                    // Is empty, let's purge
                    dbg.output(CALL_INFO, "Entry for 0x%"PRIx64" has no referenes - purging\n", targetBlock);
                    directory.erase(entry->baseAddr);
                    delete entry;
                }
            }
        } else {
            _abort(DirectoryController, "Received unexpected message from Memory!\n");
        }
    } else {
        /* Don't have this req recorded */
    }


    delete ev;
}

void DirectoryController::handlePacket(SST::Event *event)
{
    MemEvent *ev = static_cast<MemEvent*>(event);
    ev->setDeliveryTime(getCurrentSimTimeNano());
    workQueue.push_back(ev);
    dbg.output(CALL_INFO, "Received (%"PRIu64", %d) %s 0x%"PRIx64" from %s.  Position %zu in queue.\n", ev->getID().first, ev->getID().second, CommandString[ev->getCmd()], ev->getAddr(), ev->getSrc().c_str(), workQueue.size());
}


bool DirectoryController::processPacket(MemEvent *ev)
{
    assert(isRequestAddressValid(ev));
    dbg.output(CALL_INFO, "Processing (%"PRIu64", %d) %s 0x%"PRIx64" from %s.  Status: %s\n", ev->getID().first, ev->getID().second, CommandString[ev->getCmd()], ev->getAddr(), ev->getSrc().c_str(), printDirectoryEntryStatus(ev->getAddr()));

    std::set<MemEvent::id_type>::iterator ign = ignorableResponses.find(ev->getResponseToID());
    if ( ign != ignorableResponses.end() ) {
        dbg.output(CALL_INFO, "Command (%"PRIu64", %d) is a response to (%"PRIu64", %d), which is ignorable.\n",
                ev->getID().first, ev->getID().second, ev->getResponseToID().first, ev->getResponseToID().second);
        ignorableResponses.erase(ign);
        delete ev;
        return true;
    }

    if ( ACK == ev->getCmd() ) {
        handleACK(ev);
        delete ev;
        return true;
    }


    if ( ev->queryFlag(MemEvent::F_UNCACHED) ) {
        MemEvent::id_type sentID = writebackData(ev);
        uncachedWrites[sentID] = ev;
        return true;
    }


    DirEntry *entry = getDirEntry(ev->getAddr());

    if ( entry && entry->inProgress() ) {
        if ( entry->nextCommand == ev->getCmd() &&
            ( "N/A" == entry->waitingOn  ||
              !entry->waitingOn.compare(ev->getSrc()) ) ) {
            dbg.output(CALL_INFO, "Incoming command matches for 0x%"PRIx64" in progress.\n", entry->baseAddr);
            if ( ev->getResponseToID() != entry->lastRequest ) {
                dbg.output(CALL_INFO, "This isn't a response to our request, but it fulfills the need.  Placing (%"PRIu64", %d) into list of ignorable responses.\n", entry->lastRequest.first, entry->lastRequest.second);
                ignorableResponses.insert(entry->lastRequest);
            }
            advanceEntry(entry, ev);
            delete ev;
        } else {
            dbg.output(CALL_INFO, "Incoming command [%s,%s] doesn't match for 0x%"PRIx64" [%s,%s] in progress.\n", CommandString[ev->getCmd()], ev->getSrc().c_str(), entry->baseAddr, CommandString[entry->nextCommand], entry->waitingOn.c_str());
            switch ( ev->getCmd() ) {
            case Invalidate:
            case RequestData: {
                MemEvent *nack = ev->makeResponse(this);
                dbg.output(CALL_INFO, "Sending NACK (%"PRIu64", %d) for [%s,%s 0x%"PRIx64"]\n", nack->getID().first, nack->getID().second, CommandString[ev->getCmd()], ev->getSrc().c_str(), entry->baseAddr);
                nack->setCmd(NACK);
                sendResponse(nack);
                delete ev;
                break;
            }
            default:
                dbg.output(CALL_INFO, "Re-enqueuing for  [%s,%s 0x%"PRIx64"]\n", CommandString[ev->getCmd()], ev->getSrc().c_str(), entry->baseAddr);
                return false;
            }
        }
        return true;
    }


    /* New Request */

    dbg.output(CALL_INFO, "Entry 0x%"PRIx64" not in progress.\n", ev->getAddr());
    switch(ev->getCmd()) {
    case SupplyData: {
        /* May be a response to a Fetch/FetchInvalidate
         * May be a writeback
         */
        assert(entry);
        entry->activeReq = ev;
        if ( entry->inController ) {
            ++numCacheHits;
            handleWriteback(entry, ev);
        } else {
            dbg.output(CALL_INFO, "Entry 0x%"PRIx64" not in cache.  Requesting from memory.\n", entry->baseAddr);
            entry->nextFunc = &DirectoryController::handleWriteback;
            requestDirEntryFromMemory(entry);
        }
        break;
    }
    case RequestData: {
        if ( !entry ) {
            entry = createDirEntry(ev->getAddr());
            entry->inController = true;
        }
        entry->activeReq = ev;
        if ( entry->inController ) {
            ++numCacheHits;
            handleRequestData(entry, ev);
        } else {
            dbg.output(CALL_INFO, "Entry 0x%"PRIx64" not in cache.  Requesting from memory.\n", entry->baseAddr);
            entry->nextFunc = &DirectoryController::handleRequestData;
            requestDirEntryFromMemory(entry);
        }
        break;
    }
    case Invalidate: {
        assert(entry);
        assert(!entry->dirty);
        entry->activeReq = ev;
        if ( entry->inController ) {
            ++numCacheHits;
            handleInvalidate(entry, ev);
        } else {
            dbg.output(CALL_INFO, "Entry 0x%"PRIx64" not in cache.  Requesting from memory.\n", entry->baseAddr);
            entry->nextFunc = &DirectoryController::handleInvalidate;
            requestDirEntryFromMemory(entry);
        }
        break;
    }
    case Evicted: {
        assert(entry);
        entry->activeReq = ev;
        if ( entry->inController ) {
            ++numCacheHits;
            handleEviction(entry, ev);
        } else {
            dbg.output(CALL_INFO, "Entry 0x%"PRIx64" not in cache.  Requesting from memory.\n", entry->baseAddr);
            entry->nextFunc = &DirectoryController::handleEviction;
            requestDirEntryFromMemory(entry);
        }
        break;
    }
    default:
        /* Ignore unexpected */
        delete ev;
    }
    return true;
}


void DirectoryController::init(unsigned int phase)
{
    network->init(phase);

    /* Pass data on to memory */
    while ( MemEvent *ev = network->recvInitData() ) {
        dbg.output(CALL_INFO, "Found Init Info for address 0x%"PRIx64"\n", ev->getAddr());
        if ( isRequestAddressValid(ev) ) {
            ev->setAddr(convertAddressToLocalAddress(ev->getAddr()));
            dbg.output(CALL_INFO, "Sending Init Data for address 0x%"PRIx64" to memory\n", ev->getAddr());
            memLink->sendInitData(ev);
        } else {
            delete ev;
        }
    }

}

void DirectoryController::finish(void)
{
    network->finish();

    Output out("", 0, 0, printStatsLoc);
    out.output("Directory %s stats:\n"
            "\t# Requests:        %"PRIu64"\n"
            "\tAvg Req Time:      %"PRIu64" ns\n"
            "\tEntry Cache Hits:  %"PRIu64"\n",
            getName().c_str(),
            numReqsProcessed,
            (numReqsProcessed > 0) ? totalReqProcessTime / numReqsProcessed : 0,
            numCacheHits);
}


void DirectoryController::setup(void)
{
    network->setup();

    const std::vector<MemNIC::ComponentInfo> &ci = network->getPeerInfo();
    for ( std::vector<MemNIC::ComponentInfo>::const_iterator i = ci.begin() ; i != ci.end() ; ++i ) {
        dbg.output(CALL_INFO, "DC found peer %d (%s) of type %d.\n", i->network_addr, i->name.c_str(), i->type);
        if ( MemNIC::TypeCache == i->type ) {
            numTargets++;
            if ( blocksize ) {
                assert(blocksize == i->typeInfo.cache.blocksize);
            } else {
                blocksize = i->typeInfo.cache.blocksize;
            }
        }
    }
    if ( 0 == numTargets ) {
        _abort(DirectoryController, "Directory Controller %s unable to see any caches.\n", getName().c_str());
    }

    network->clearPeerInfo();
}


void DirectoryController::printStatus(Output &out)
{
    out.output("MemHierarchy::DirectoryController %s\n", getName().c_str());
    out.output("\t# Entries in cache:  %zu\n", entryCache.size());
    out.output("\t# Requests in queue:  %zu\n", workQueue.size());
    for ( std::list<MemEvent*>::iterator i = workQueue.begin() ; i != workQueue.end() ; ++i ) {
        out.output("\t\t(%"PRIu64", %d)\n", (*i)->getID().first, (*i)->getID().second);
    }
    out.output("\tRequests in Progress:\n");
    for ( std::map<Addr, DirEntry*>::iterator i = directory.begin() ; i != directory.end() ; ++i ) {
        if ( i->second->inProgress() ) {
            out.output("\t\t0x%"PRIx64"\t\t(%"PRIu64", %d)\n",
                    i->first,
                    i->second->activeReq->getID().first,
                    i->second->activeReq->getID().second);
        }
    }

}


bool DirectoryController::clock(SST::Cycle_t cycle)
{
    network->clock();

	for ( std::list<MemEvent*>::iterator i = workQueue.begin() ; i != workQueue.end() ; ++i ) {
        if ( processPacket(*i) ) {
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
	if ( directory.end() == i ) return NULL;
	return i->second;
}


DirectoryController::DirEntry* DirectoryController::createDirEntry(Addr addr)
{
    dbg.output(CALL_INFO, "Creating Directory Entry for 0x%"PRIx64"\n", addr);
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
	if ( 0 == entry->waitingAcks ) {
		advanceEntry(entry);
	}
}


void DirectoryController::handleInvalidate(DirEntry *entry, MemEvent *new_ev)
{
    assert(!entry->dirty);
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
        entry->nextCommand = ACK;
        entry->waitingOn = "N/A";
        entry->lastRequest = DirEntry::NO_LAST_REQUEST;
        dbg.output(CALL_INFO, "Sending invalidates to allow exclusive access for 0x%"PRIx64".\n", entry->baseAddr);
	} else {
		finishInvalidate(entry, NULL);
	}
}


void DirectoryController::finishInvalidate(DirEntry *entry, MemEvent *new_ev)
{
	assert(0 == entry->waitingAcks);

	uint32_t target_id = node_id(entry->activeReq->getSrc());
    entry->clearSharers();
	entry->sharers[target_id] = true;
	entry->dirty = true;

    dbg.output(CALL_INFO, "Setting dirty, with owner %s for 0x%"PRIx64".\n", nodeid_to_name[target_id].c_str(), entry->baseAddr);

	MemEvent *ev = entry->activeReq->makeResponse(this);
	sendResponse(ev);

	updateEntryToMemory(entry);
}


void DirectoryController::sendInvalidate(int target, Addr addr)
{
    MemEvent *me = new MemEvent(this, addr, Invalidate);
    me->setSize(blocksize);
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
        std::string &dest = nodeid_to_name[entry->findOwner()];
        ev->setSize(blocksize);
        ev->setDst(dest);

		entry->nextFunc = &DirectoryController::finishFetch;
        entry->nextCommand = SupplyData;
        entry->waitingOn = dest;
        entry->lastRequest = ev->getID();

		sendResponse(ev);
        dbg.output(CALL_INFO, "Sending %s to %s to fulfill request for data for 0x%"PRIx64".\n",
                CommandString[cmd], dest.c_str(), entry->baseAddr);

	} else if ( entry->activeReq->queryFlag(MemEvent::F_EXCLUSIVE) ) {
		// Must send invalidates
		assert(0 == entry->waitingAcks);
		for ( uint32_t i = 0 ; i < numTargets ; i++ ) {
			if ( i == requesting_node ) continue;
			if ( entry->sharers[i] ) {
				sendInvalidate(i, entry->baseAddr);
				entry->waitingAcks++;
			}
		}
		if ( entry->waitingAcks > 0 ) {
			entry->nextFunc = &DirectoryController::getExclusiveDataForRequest;
            entry->nextCommand = ACK;
            entry->waitingOn = "N/A";
            entry->lastRequest = DirEntry::NO_LAST_REQUEST;
            dbg.output(CALL_INFO, "Sending Invalidates to fulfill request for exclusive 0x%"PRIx64".\n", entry->baseAddr);
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
        entry->clearSharers();
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
    dbg.output(CALL_INFO, "Sending requested data for 0x%"PRIx64" to %s\n", entry->baseAddr, ev->getDst().c_str());
	sendResponse(ev);
	updateEntryToMemory(entry);
}


void DirectoryController::getExclusiveDataForRequest(DirEntry* entry, MemEvent *new_ev)
{
	assert(0 == entry->waitingAcks);

	uint32_t target_id = node_id(entry->activeReq->getSrc());
    entry->clearSharers();
	entry->sharers[target_id] = true;
	entry->dirty = true;

	entry->nextFunc = &DirectoryController::sendRequestedData;
	requestDataFromMemory(entry);
}



void DirectoryController::handleWriteback(DirEntry *entry, MemEvent *ev)
{
    dbg.output(CALL_INFO, "Entry 0x%"PRIx64" loaded.  Performing writeback of 0x%"PRIx64" for %s\n", entry->baseAddr, entry->activeReq->getAddr(), entry->activeReq->getSrc().c_str());
    assert(entry->dirty);
    assert(entry->findOwner() == node_lookup[entry->activeReq->getSrc()]);
    entry->dirty = false;
    entry->sharers[node_id(entry->activeReq->getSrc())] = true;
    writebackData(entry->activeReq);
	updateEntryToMemory(entry);
}


/** This is an eviction of a block that is clean */
void DirectoryController::handleEviction(DirEntry *entry, MemEvent *ev)
{
    dbg.output(CALL_INFO, "Entry 0x%"PRIx64" loaded.  Handling eviction of 0x%"PRIx64" from %s\n", entry->baseAddr, entry->activeReq->getAddr(), entry->activeReq->getSrc().c_str());
    assert(!entry->dirty);
    entry->sharers[node_id(entry->activeReq->getSrc())] = false;

    updateEntryToMemory(entry);
}




/* Advance the processing of this directory entry */
void DirectoryController::advanceEntry(DirEntry *entry, MemEvent *ev)
{
	assert(NULL != entry->nextFunc);
	(this->*(entry->nextFunc))(entry, ev);
}


uint32_t DirectoryController::node_id(const std::string &name)
{
	uint32_t id;
	std::map<std::string, uint32_t>::iterator i = node_lookup.find(name);
	if ( node_lookup.end() == i ) {
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
    entry->nextCommand = SupplyData;
    entry->waitingOn = "memory";
    entry->lastRequest = me->getID();
    dbg.output(CALL_INFO, "Requesting Entry from memory for 0x%"PRIx64" (%"PRIu64", %d)\n", entry->baseAddr, me->getID().first, me->getID().second);
	memLink->send(me);
}


void DirectoryController::requestDataFromMemory(DirEntry *entry)
{
    MemEvent *ev = new MemEvent(this, convertAddressToLocalAddress(entry->baseAddr), RequestData);
    ev->setSize(blocksize);
    memReqs[ev->getID()] = entry->baseAddr;
    entry->nextCommand = SupplyData;
    entry->waitingOn = "memory";
    entry->lastRequest = ev->getID();
    dbg.output(CALL_INFO, "Requesting data from memory at 0x%"PRIx64" (%"PRIu64", %d)\n", entry->baseAddr, ev->getID().first, ev->getID().second);
    memLink->send(ev);
}

void DirectoryController::updateCacheEntry(DirEntry *entry)
{
    if ( 0 == entryCacheSize ) {
        sendEntryToMemory(entry);
    } else {
        /* Find if we're in the cache */
        for ( std::list<DirEntry*>::iterator i = entryCache.begin() ; i != entryCache.end() ; ++i ) {
            if ( *i == entry ) {
                entryCache.erase(i);
                break;
            }
        }
        entryCache.push_front(entry);

        while ( entryCache.size() > entryCacheSize ) {
            DirEntry *oldEntry = entryCache.back();
            // If the oldest entry is still in progress, everything is in progress
            if ( oldEntry->inProgress() ) break;

            dbg.output(CALL_INFO, "entryCache too large.  Evicting entry for 0x%"PRIx64"\n", oldEntry->baseAddr);
            entryCache.pop_back();
            sendEntryToMemory(oldEntry);
        }
    }
}


void DirectoryController::updateEntryToMemory(DirEntry *entry)
{
    updateCacheEntry(entry);
    resetEntry(entry);
}


void DirectoryController::sendEntryToMemory(DirEntry *entry)
{
	Addr entryAddr = 0; /*  Offset into our buffer? */
	MemEvent *me = new MemEvent(this, entryAddr, SupplyData);
    dbg.output(CALL_INFO, "Updating entry for 0x%"PRIx64" to memory (%"PRIu64", %d)\n", entry->baseAddr, me->getID().first, me->getID().second);
	me->setSize((numTargets+1)/8 +1);
    memReqs[me->getID()] = entry->baseAddr;
	memLink->send(me);
}


MemEvent::id_type DirectoryController::writebackData(MemEvent *data_event)
{
	MemEvent *ev = new MemEvent(this, convertAddressToLocalAddress(data_event->getAddr()), SupplyData);
    if ( data_event->queryFlag(MemEvent::F_UNCACHED) ) ev->setFlag(MemEvent::F_UNCACHED);
	ev->setFlag(MemEvent::F_WRITEBACK);
	ev->setPayload(data_event->getPayload());
    dbg.output(CALL_INFO, "Writing back data to 0x%"PRIx64" (%"PRIu64", %d)\n", data_event->getAddr(), ev->getID().first, ev->getID().second);

	memLink->send(ev);

    return ev->getID();
}


void DirectoryController::resetEntry(DirEntry *entry)
{
	if ( entry->activeReq ) {
        dbg.output(CALL_INFO, "Resetting entry after event (%"PRIu64", %d) %s 0x%"PRIx64".  Processing time: %"PRIu64"\n",
                entry->activeReq->getID().first, entry->activeReq->getID().second,
                CommandString[entry->activeReq->getCmd()], entry->activeReq->getAddr(),
                getCurrentSimTimeNano() - entry->activeReq->getDeliveryTime());

        ++numReqsProcessed;
        totalReqProcessTime += (getCurrentSimTimeNano() - entry->activeReq->getDeliveryTime());

        delete entry->activeReq;
    }
	entry->activeReq = NULL;
	entry->nextFunc = NULL;
    entry->nextCommand = NULLCMD;
    entry->lastRequest = DirEntry::NO_LAST_REQUEST;
    entry->waitingOn = "N/A";
}


void DirectoryController::sendResponse(MemEvent *ev)
{
    dbg.output(CALL_INFO, "Sending %s 0x%"PRIx64" to %s\n", CommandString[ev->getCmd()], ev->getAddr(), ev->getDst().c_str());
	network->send(ev);
}


bool DirectoryController::isRequestAddressValid(MemEvent *ev)
{
    Addr addr = ev->getAddr();

    if ( 0 == interleaveSize ) {
        return ( addr >= addrRangeStart && addr < addrRangeEnd );
    } else {
        if ( addr < addrRangeStart ) return false;
        if ( addr >= addrRangeEnd ) return false;

        addr = addr - addrRangeStart;

        Addr offset = addr % interleaveStep;
        if ( offset >= interleaveSize ) return false;

        return true;
    }

}


Addr DirectoryController::convertAddressToLocalAddress(Addr addr)
{
    Addr res = 0;
    if ( 0 == interleaveSize ) {
        res = lookupBaseAddr + addr - addrRangeStart;
    } else {
        addr = addr - addrRangeStart;
        Addr step = addr / interleaveStep;
        Addr offset = addr % interleaveStep;
        res = lookupBaseAddr + (step * interleaveSize) + offset;
    }
    dbg.output(CALL_INFO, "Converted physical address 0x%"PRIx64" to ACTUAL memory address 0x%"PRIx64"\n", addr, res);
    return res;
}

static char dirEntStatus[1024] = {0};
const char* DirectoryController::printDirectoryEntryStatus(Addr addr)
{
    DirEntry *entry = getDirEntry(addr);
    if ( !entry ) {
        sprintf(dirEntStatus, "[Not Created]");
    } else {
        uint32_t refs = entry->countRefs();

        if ( 0 == refs ) {
            sprintf(dirEntStatus, "[Uncached]");
        } else if ( entry->dirty ) {
            uint32_t owner = entry->findOwner();
            sprintf(dirEntStatus, "[owned by %s]", nodeid_to_name[owner].c_str());
        } else {
            sprintf(dirEntStatus, "[Shared by %u]", refs);
        }

    }
    return dirEntStatus;
}
