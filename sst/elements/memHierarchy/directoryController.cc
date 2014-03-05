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

#include <sst/core/params.h>
#include "directoryController.h"

#include <assert.h>
#include "memNIC.h"


using namespace SST;
using namespace SST::MemHierarchy;


const MemEvent::id_type DirectoryController::DirEntry::NO_LAST_REQUEST = std::make_pair((uint64_t)-1, -1);

DirectoryController::DirectoryController(ComponentId_t id, Params &params) :
    Component(id), blocksize(0)
{
    dbg.init("", 0, 0, (Output::output_location_t)params.find_integer("debug", 0));
    printStatsLoc = (Output::output_location_t)params.find_integer("statistics", 0);

    targetCount = 0;

	registerTimeBase("1 ns", true);

	memLink = configureLink("memory", "1 ns", new Event::Handler<DirectoryController>(this, &DirectoryController::handleMemoryResponse));
	assert(memLink);


    entryCacheSize = (size_t)params.find_integer("entry_cache_size", 0);
    int addr = params.find_integer("network_addr");
    std::string net_bw = params.find_string("network_bw");

	addrRangeStart = (uint64_t)params.find_integer("addr_range_start", 0);
	addrRangeEnd = (uint64_t)params.find_integer("addr_range_end", 0);
	if(0 == addrRangeEnd) addrRangeEnd = (uint64_t)-1;
	interleaveSize = (Addr)params.find_integer("interleave_size", 0);
    interleaveSize *= 1024;
	interleaveStep = (Addr)params.find_integer("interleave_step", 0);
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

    lookupBaseAddr = params.find_integer("backing_store_size", 0x1000000); // 16MB

    numReqsProcessed = 0;
    totalReqProcessTime = 0;
    numCacheHits = 0;
}


DirectoryController::~DirectoryController()
{
    for (std::map<Addr, DirEntry*>::iterator i = directory.begin(); i != directory.end() ; ++i) {
        delete i->second;
    }
    directory.clear();
    while (workQueue.size()) {
        MemEvent *front = workQueue.front();
        workQueue.pop_front();
        delete front;
    }
}


void DirectoryController::handleMemoryResponse(SST::Event *event)
{
	MemEvent *ev = static_cast<MemEvent*>(event);
    dbg.output("\n\n----------------------------------------------------------------------------------------\n");
    dbg.output(CALL_INFO, "Directory Controller - %s - Memory response. Cmd = %s for address %#016llx (Response to (%#016llx, %d))\n", getName().c_str(), CommandString[ev->getCmd()], ev->getAddr(), ev->getResponseToID().first, ev->getResponseToID().second);

    if(uncachedWrites.find(ev->getResponseToID()) != uncachedWrites.end()) {
        MemEvent *origEV = uncachedWrites[ev->getResponseToID()];
        uncachedWrites.erase(ev->getResponseToID());

        MemEvent *resp = origEV->makeResponse(this);
        sendResponse(resp);

        delete origEV;

    } else if(memReqs.find(ev->getResponseToID()) != memReqs.end()) {
        Addr targetBlock = memReqs[ev->getResponseToID()];
        memReqs.erase(ev->getResponseToID());
        //if(SupplyData == ev->getCmd())
        //Why would I get a 'SupplyData' response from a memory request?
        if(GetSResp == ev->getCmd() || GetXResp == ev->getCmd()) {
            // Lookup complete, perform our work
            DirEntry *entry = getDirEntry(targetBlock);
            assert(entry);
            entry->inController = true;
            advanceEntry(entry, ev);
        }
        else {
            _abort(DirectoryController, "Received unexpected message from Memory!\n");
        }
    } else {
        /* Don't have this req recorded */
        _abort(DirectoryController, "Unexpected event received\n");
    }


    delete ev;
}

void DirectoryController::handlePacket(SST::Event *event)
{
    MemEvent *ev = static_cast<MemEvent*>(event);
    ev->setDeliveryTime(getCurrentSimTimeNano());
    workQueue.push_back(ev);
    dbg.output("\n\n----------------------------------------------------------------------------------------\n");
    dbg.output(CALL_INFO, "Directory Controller - %s - Received (%#016llx, %d) %s Cmd = %s %#016llx from %s.  Position %zu in queue.\n", getName().c_str(), ev->getID().first, ev->getID().second,  ev->queryFlag(MemEvent::F_UNCACHED) ? "Uncached " : "", CommandString[ev->getCmd()], ev->getBaseAddr(), ev->getSrc().c_str(), workQueue.size());
}


bool DirectoryController::processPacket(MemEvent *ev)
{
    assert(isRequestAddressValid(ev));
    Command cmd = ev->getCmd();
    uint32_t requesting_node;

    dbg.output(CALL_INFO, "Processing %s (%#016llx, %d) %#016llx from %s.  Status: %s\n", CommandString[cmd], ev->getID().first, ev->getID().second, ev->getBaseAddr(), ev->getSrc().c_str(), printDirectoryEntryStatus(ev->getBaseAddr()));
    std::set<MemEvent::id_type>::iterator ign = ignorableResponses.find(ev->getResponseToID());
    
    //TODO: What is this for??
    if(ign != ignorableResponses.end()) {
        assert(0);
        std::set<MemEvent::id_type>::iterator ign = ignorableResponses.find(ev->getResponseToID());
        if(ign != ignorableResponses.end()) {
            dbg.output(CALL_INFO, "Command (%#016llx, %d) is a response to (%#016llx, %d), which is ignorable.\n",
                ev->getID().first, ev->getID().second, ev->getResponseToID().first, ev->getResponseToID().second);
        ignorableResponses.erase(ign);
        delete ev;
        return true;
    }
    }

    
    if(InvAck == cmd) {
        delete ev;
        return true;
    }

    //Writeback uncached data
    if(ev->queryFlag(MemEvent::F_UNCACHED) && PutM == cmd) {  //PutM was SupplyData
        assert(true == false);
    }


    DirEntry *entry = getDirEntry(ev->getBaseAddr());
    std::cout << std::flush;

    //Is request already in progress??
    if(entry && entry->inProgress()) {
        /* Advance entry if this is a request we are expecting */
        if((entry->nextCommand == cmd || (entry->nextCommand == FetchResp && cmd == PutM)) &&
           ("N/A" == entry->waitingOn || entry->waitingOn == ev->getSrc())) {
            dbg.output(CALL_INFO, "Incoming command matches for %#016llx in progress.\n", entry->baseAddr);
            if(ev->getResponseToID() != entry->lastRequest) {
                dbg.output(CALL_INFO, "This isn't a response to our request, but it fulfills the need.  Placing (%#016llx, %d) into list of ignorable responses.\n", entry->lastRequest.first, entry->lastRequest.second);
            }
            advanceEntry(entry, ev);
            delete ev;
            return true;
        } else {
            /* Ignore request */
            dbg.output(CALL_INFO, "Next Command = %s, WaitingOn = %s\n", CommandString[entry->nextCommand], entry->waitingOn.c_str());
            dbg.output(CALL_INFO, "Re-enqueuing for  [%s,%s %#016llx]\n", CommandString[ev->getCmd()], ev->getSrc().c_str(), entry->baseAddr);
            return false;
        }
        
    }

    /* New Request */
    switch(cmd) {
    case PutS:
    case InvAck:
        assert(entry);
        if(entry->dirty) return true;  //ignore request

        entry->activeReq = ev;
        dbg.output(CALL_INFO, "\n\nDC PutS - %s - Request Received\n", getName().c_str());
        requesting_node = node_name_to_id(entry->activeReq->getSrc());
        entry->sharers[requesting_node]= false;
        resetEntry(entry);
        break;
        
    case PutM:        /* Was SupplyData */
    //case FetchResp:  /* May be a response to a Fetch/FetchInvalidate, May be a writeback */
        assert(entry);
        dbg.output(CALL_INFO, "\n\nDC PutM - %s - Request Received\n", getName().c_str());
        entry->activeReq = ev;
        if(entry->inController) {
            ++numCacheHits;
            handleWriteback(entry, ev);
        } else {
            dbg.output(CALL_INFO, "Entry %#016llx not in cache.  Requesting from memory.\n", entry->baseAddr);
            entry->nextFunc = &DirectoryController::handleWriteback;
            requestDirEntryFromMemory(entry);
        }
        break;
    case GetX:    /* was RequestData */
    case GetSEx:
    case GetS:
        dbg.output(CALL_INFO, "\n\nDC GetS/GetX - %s - Request Received\n", getName().c_str());
        if(!entry) {
            entry = createDirEntry(ev->getBaseAddr(), ev->getAddr(), ev->getSize());
            entry->inController = true;
        }

        if(entry->inController) {
            ++numCacheHits;  //TODO: not a hit if it was created above
            handleRequestData(entry, ev);
        } else {
            //_abort(DirectoryController, "When is the entry NOT in the dirController?");
            dbg.output(CALL_INFO, "Entry %#016llx not in cache.  Requesting from memory.\n", entry->baseAddr);
            entry->nextFunc = &DirectoryController::handleRequestData;
            requestDirEntryFromMemory(entry);
        }
        break;
    
    case Inv:
        _abort(DirectoryController, "No Inv allowed..");
        break;
    default:
        /* Ignore unexpected */
        _abort(DirectoryController, "Cmd not expected, Cmd = %s\n", CommandString[cmd]);
        break;
    }
    return true;
}


void DirectoryController::init(unsigned int phase)
{
    network->init(phase);

    /* Pass data on to memory */
    while (MemEvent *ev = network->recvInitData()) {
        dbg.output(CALL_INFO, "Found Init Info for address %#016llx\n", ev->getBaseAddr());
        if(isRequestAddressValid(ev)) {
            ev->setBaseAddr(convertAddressToLocalAddress(ev->getBaseAddr()));
            ev->setAddr(convertAddressToLocalAddress(ev->getAddr()));
            dbg.output(CALL_INFO, "Sending Init Data for address %#016llx to memory\n", ev->getAddr());
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
            "\t# Requests:        %#016llx\n"
            "\tAvg Req Time:      %#016llx ns\n"
            "\tEntry Cache Hits:  %#016llx\n",
            getName().c_str(),
            numReqsProcessed,
            (numReqsProcessed > 0) ? totalReqProcessTime / numReqsProcessed : 0,
            numCacheHits);
}


void DirectoryController::setup(void)
{
    network->setup();

    const std::vector<MemNIC::ComponentInfo> &ci = network->getPeerInfo();
    for (std::vector<MemNIC::ComponentInfo>::const_iterator i = ci.begin() ; i != ci.end() ; ++i) {
        dbg.output(CALL_INFO, "DC found peer %d (%s) of type %d.\n", i->network_addr, i->name.c_str(), i->type);
        if(MemNIC::TypeCache == i->type) {
            numTargets++;
            if(blocksize) {
                assert(blocksize == i->typeInfo.cache.blocksize);
            } else {
                blocksize = i->typeInfo.cache.blocksize;
            }
        }
    }
    if(0 == numTargets) {
        _abort(DirectoryController, "Directory Controller %s unable to see any caches.\n", getName().c_str());
    }

    network->clearPeerInfo();
}


void DirectoryController::printStatus(Output &out)
{
    out.output("MemHierarchy::DirectoryController %s\n", getName().c_str());
    out.output("\t# Entries in cache:  %zu\n", entryCache.size());
    out.output("\t# Requests in queue:  %zu\n", workQueue.size());
    for (std::list<MemEvent*>::iterator i = workQueue.begin() ; i != workQueue.end() ; ++i) {
        out.output("\t\t(%#016llx, %d)\n", (*i)->getID().first, (*i)->getID().second);
    }
    out.output("\tRequests in Progress:\n");
    for (std::map<Addr, DirEntry*>::iterator i = directory.begin() ; i != directory.end() ; ++i) {
        if(i->second->inProgress()) {
            out.output("\t\t%#016llx\t\t(%#016llx, %d)\n",
                    i->first,
                    i->second->activeReq->getID().first,
                    i->second->activeReq->getID().second);
        }
    }

}


bool DirectoryController::clock(SST::Cycle_t cycle)
{
    network->clock();

    if(!workQueue.empty()){	//for (std::list<MemEvent*>::iterator i = workQueue.begin() ; i != workQueue.end() ; ++i) {
        if(processPacket(workQueue.front())) {
            workQueue.erase(workQueue.begin()); // Let's only process one per cycle
        }
	}

	return false;
}



DirectoryController::DirEntry* DirectoryController::getDirEntry(Addr baseAddr)
{
	std::map<Addr, DirEntry*>::iterator i = directory.find(baseAddr);
	if(directory.end() == i) return NULL;
	return i->second;
}


DirectoryController::DirEntry* DirectoryController::createDirEntry(Addr baseAddr, Addr addr, uint32_t reqSize)
{
    dbg.output(CALL_INFO, "Creating Directory Entry for %#016llx\n", baseAddr);
	DirEntry *entry = new DirEntry(baseAddr, addr, reqSize, numTargets);
	directory[baseAddr] = entry;
	return entry;
}



void DirectoryController::handleACK(MemEvent *ev)
{
	DirEntry *entry = getDirEntry(ev->getBaseAddr());
	assert(entry);
	assert(entry->waitingAcks > 0);
	entry->waitingAcks--;
	if(0 == entry->waitingAcks) {
		advanceEntry(entry);
	}
}


void DirectoryController::sendInvalidate(int target, DirEntry* entry)
{
    MemEvent *me = new MemEvent(this, entry->activeReq->getAddr(), entry->activeReq->getBaseAddr(), 64, Inv, entry->activeReq->getSize());
    me->setDst(nodeid_to_name[target]);
    network->send(me);
}



void DirectoryController::handleRequestData(DirEntry* entry, MemEvent *new_ev)
{
    entry->activeReq = new_ev;
    entry->reqSize = new_ev->getSize();
	uint32_t requesting_node = node_id(entry->activeReq->getSrc());
    
    if(entry->dirty) {  /* If a cache has in Modified State, must do a fetch */
        //Command cmd = FetchInvalidateX;  //TODO: Fetch modifies M -> S  (not M -> I)
		Command cmd = FetchInvalidate;
        
		if(entry->activeReq->getCmd() == GetX ||
           entry->activeReq->getCmd() == GetSEx) cmd = FetchInvalidate;
		MemEvent *ev = new MemEvent(this, entry->activeReq->getAddr(), entry->activeReq->getBaseAddr(), 64, cmd, entry->activeReq->getSize());
        std::string &dest = nodeid_to_name[entry->findOwner()];
        ev->setDst(dest);

		entry->nextFunc = &DirectoryController::finishFetch;
        entry->nextCommand = FetchResp;
        entry->waitingOn = dest;
        entry->lastRequest = ev->getID();

		sendResponse(ev);
        dbg.output(CALL_INFO, "Sending %s to %s to fulfill request for data for %#016llx.\n", CommandString[cmd], dest.c_str(), entry->baseAddr);
        return;
	}
    
    //Handle Un-Cached request
    if(entry->activeReq->queryFlag(MemEvent::F_UNCACHED)) {
        // Don't set as a sharer when dealing with uncached
		entry->nextFunc = &DirectoryController::sendRequestedData;
		requestDataFromMemory(entry);
        return;
    } 
    
    bool invSent = false;
    //Handle GetX requests - no dirty sharer
    if(entry->activeReq->getCmd() == GetX || entry->activeReq->getCmd() == GetSEx) {
		assert(0 == entry->waitingAcks);
		for (uint32_t i = 0 ; i < numTargets ; i++) { /* Send invalidates to all sharers */
			if(i == requesting_node) continue;
			if(entry->sharers[i]) {
				sendInvalidate(i, entry);
				invSent = true;
                //entry->waitingAcks++;
			}
		}
        
        if(invSent) dbg.output(CALL_INFO, "Sending Invalidates\n");

		/*if(entry->waitingAcks > 0) {
            //TODO: no need to wait for ACKs since they are all sharers only
			entry->nextFunc = &DirectoryController::getExclusiveDataForRequest;
            entry->nextCommand = InvAck;
            entry->waitingOn = "N/A";
            entry->lastRequest = DirEntry::NO_LAST_REQUEST;
            dbg.output(CALL_INFO, "Sending Invalidates to fulfill request for exclusive 0x%"PRIx64".\n", entry->baseAddr);
		} else {
        */
			getExclusiveDataForRequest(entry, NULL);
        //}
	}
    //Handle GetS requests
    else {
		entry->sharers[requesting_node]= true;
		entry->nextFunc = &DirectoryController::sendRequestedData;
		requestDataFromMemory(entry);
	}
}

void DirectoryController::finishFetch(DirEntry* entry, MemEvent *new_ev)
{
	if(entry->activeReq->getCmd() == GetX || entry->activeReq->getCmd() == GetSEx) {
		entry->dirty = true;
        entry->clearSharers();
	} else {
		entry->dirty = false;
	}
    
    //owner is also a sharer if request is not uncached
    if(!entry->activeReq->queryFlag(MemEvent::F_UNCACHED)) entry->sharers[node_id(entry->activeReq->getSrc())] = true;

    
    assert(entry->activeReq->getBaseAddr() == entry->baseAddr);
    dbg.output(CALL_INFO, "Finishing Fetch.  Status: %s\n",  printDirectoryEntryStatus(entry->baseAddr));

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
    dbg.output(CALL_INFO, "Sending requested data for %#016llx to %s\n", entry->baseAddr, ev->getDst().c_str());
	sendResponse(ev);
    if(entry->activeReq->queryFlag(MemEvent::F_UNCACHED) &&
            0 == entry->countRefs()) {
        // Uncached request, entry not cached anywhere.  Delete
        directory.erase(entry->baseAddr);
    } else {
        updateEntryToMemory(entry);
    }
}


void DirectoryController::getExclusiveDataForRequest(DirEntry* entry, MemEvent *new_ev)
{
	assert(0 == entry->waitingAcks);

	uint32_t target_id = node_id(entry->activeReq->getSrc());
    entry->clearSharers();
    if(!entry->activeReq->queryFlag(MemEvent::F_UNCACHED)) entry->sharers[target_id] = true;
	entry->dirty = true;

	entry->nextFunc = &DirectoryController::sendRequestedData;
	requestDataFromMemory(entry);
}



void DirectoryController::handleWriteback(DirEntry *entry, MemEvent *ev)
{
    dbg.output(CALL_INFO, "Writeback - Entry %#016llx loaded.  Performing writeback of %#016llx for %s\n", entry->baseAddr, entry->activeReq->getBaseAddr(), entry->activeReq->getSrc().c_str());
    assert(entry->dirty);
    assert(nodeid_to_name[entry->findOwner()].c_str() == ev->getSrc().c_str());
    entry->dirty = false;
    //if(!entry->activeReq->queryFlag(MemEvent::F_UNCACHED))
    entry->sharers[node_id(entry->activeReq->getSrc())] = false;
    assert(entry->countRefs() == 0);
    writebackData(entry->activeReq);
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
	if(node_lookup.end() == i) {
		node_lookup[name] = id = targetCount++;
        nodeid_to_name.resize(targetCount);
        nodeid_to_name[id] = name;
	} else {
		id = i->second;
	}
	return id;
}


uint32_t DirectoryController::node_name_to_id(const std::string &name){
   
	std::map<std::string, uint32_t>::iterator i = node_lookup.find(name);
    assert(node_lookup.end() != i);
	uint32_t id = i->second;
    return id;
}


void DirectoryController::requestDirEntryFromMemory(DirEntry *entry)
{
    _abort(DirectoryController, "Not Supported\n");
	/*Addr entryAddr = 0;
	MemEvent *me = new MemEvent(this, entryAddr, GetS);
	me->setSize((numTargets+1)/8 +1);
    memReqs[me->getID()] = entry->baseAddr;
    entry->nextCommand = MemEvent::commandResponse(entry->activeReq->getCmd());
    entry->waitingOn = "memory";
    entry->lastRequest = me->getID();
    dbg.output(CALL_INFO, "Requesting Entry from memory for 0x%"PRIx64" (%"PRIu64", %d)\n", entry->baseAddr, me->getID().first, me->getID().second);
	memLink->send(me);*/
}


void DirectoryController::requestDataFromMemory(DirEntry *entry)
{
    MemEvent *ev = new MemEvent(this, entry->activeReq->getAddr(), entry->activeReq->getBaseAddr(), 64, entry->activeReq->getCmd(), entry->activeReq->getSize());
    memReqs[ev->getID()] = entry->baseAddr;
    entry->nextCommand = MemEvent::commandResponse(entry->activeReq->getCmd());
    entry->waitingOn = "memory";
    entry->lastRequest = ev->getID();
    dbg.output(CALL_INFO, "Requesting data from memory. Cmd = %s at %#016llx (%#016llx, %d)\n", CommandString[entry->activeReq->getCmd()], entry->baseAddr, ev->getID().first, ev->getID().second);
    memLink->send(ev);
}

void DirectoryController::updateCacheEntry(DirEntry *entry)
{

    if(0 == entryCacheSize) {
        sendEntryToMemory(entry);
    } else {
        /* Find if we're in the cache */
        for (std::list<DirEntry*>::iterator i = entryCache.begin() ; i != entryCache.end() ; ++i) {
            if(*i == entry) {
                entryCache.erase(i);
                break;
            }
        }
        /* Find out if we're no longer cached, and just remove */
        if((!entry->activeReq) && (0 == entry->countRefs())) {
            dbg.output(CALL_INFO, "Entry for %#016llx has no referenes - purging\n", entry->baseAddr);
            directory.erase(entry->baseAddr);
            delete entry;
            return;
        } else {

            entryCache.push_front(entry);

            while (entryCache.size() > entryCacheSize) {
                assert(true == false);
                DirEntry *oldEntry = entryCache.back();
                // If the oldest entry is still in progress, everything is in progress
                if(oldEntry->inProgress()) break;

                dbg.output(CALL_INFO, "entryCache too large.  Evicting entry for %#016llx\n", oldEntry->baseAddr);
                entryCache.pop_back();
                sendEntryToMemory(oldEntry);
            }
        }
    }
}


void DirectoryController::updateEntryToMemory(DirEntry *entry)
{
    resetEntry(entry);
    updateCacheEntry(entry);
}


void DirectoryController::sendEntryToMemory(DirEntry *entry)
{
    _abort(DirectoryController, "not supported\n");
	Addr entryAddr = 0; /*  Offset into our buffer? */
	//TODO
    assert(0);  //PutM base and regular address = base address
    MemEvent *me = new MemEvent(this, entryAddr, PutM);
    dbg.output(CALL_INFO, "Updating entry for %#016llx to memory (%#016llx, %d)\n", entry->baseAddr, me->getID().first, me->getID().second);
	me->setSize((numTargets+1)/8 +1);
    
    if(entry && !entry->activeReq) {
        entry->inController = false;
        directory.erase(entry->baseAddr);
        delete entry;
    }
    //memReqs[me->getID()] = entry->baseAddr;
	memLink->send(me);
}


MemEvent::id_type DirectoryController::writebackData(MemEvent *data_event)
{   assert(data_event->getSize() == 64);
	MemEvent *ev = new MemEvent(this, data_event->getBaseAddr(), data_event->getBaseAddr(), 64, PutM, data_event->getSize());
	ev->setPayload(data_event->getPayload());
    dbg.output(CALL_INFO, "Writing back data to %#016llx (%#016llx, %d)\n", data_event->getBaseAddr(), ev->getID().first, ev->getID().second);

	memLink->send(ev);
    return ev->getID();
}


/*------------------------------------ Utils ---------------------------------------- */

void DirectoryController::resetEntry(DirEntry *entry)
{
	if(entry->activeReq) {
        dbg.output(CALL_INFO, "Resetting entry after event (%#016llx, %d) %s %#016llx.  Processing time: %#016llx\n",
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
    dbg.output(CALL_INFO, "Sending %s %#016llx to %s\n", CommandString[ev->getCmd()], ev->getAddr(), ev->getDst().c_str());
	network->send(ev);
}


bool DirectoryController::isRequestAddressValid(MemEvent *ev)
{
    Addr addr = ev->getAddr();

    if(0 == interleaveSize) {
        return (addr >= addrRangeStart && addr < addrRangeEnd);
    } else {
        if(addr < addrRangeStart) return false;
        if(addr >= addrRangeEnd) return false;

        addr = addr - addrRangeStart;

        Addr offset = addr % interleaveStep;
        if(offset >= interleaveSize) return false;
        return true;
    }

}


Addr DirectoryController::convertAddressToLocalAddress(Addr addr)
{
    return addr;
}

static char dirEntStatus[1024] = {0};
const char* DirectoryController::printDirectoryEntryStatus(Addr baseAddr)
{
    DirEntry *entry = getDirEntry(baseAddr);

    if(!entry) {
        sprintf(dirEntStatus, "[Not Created]");
    } else {
        uint32_t refs = entry->countRefs();

        if(0 == refs) {
            sprintf(dirEntStatus, "[Uncached]");
        } else if(entry->dirty) {
            assert(refs == 1);
            uint32_t owner = entry->findOwner();
            sprintf(dirEntStatus, "[owned by %s]", nodeid_to_name[owner].c_str());
        } else {
            sprintf(dirEntStatus, "[Shared by %u]", refs);
        }

    }
    return dirEntStatus;
}
