// Copyright 2013-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright(c) 2013-2014, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include <sst/core/serialization.h>
#include "directoryController.h"

#include <assert.h>

#include <sst/core/params.h>
#include <sst/core/simulation.h>

#include "memNIC.h"


using namespace SST;
using namespace SST::MemHierarchy;
using namespace std;


const MemEvent::id_type DirectoryController::DirEntry::NO_LAST_REQUEST = std::make_pair((uint64_t)-1, -1);

DirectoryController::DirectoryController(ComponentId_t id, Params &params) :
    Component(id), blocksize(0){
    int debugLevel = params.find_integer("debug_level", 0);
    if(debugLevel < 0 || debugLevel > 10)     _abort(Cache, "Debugging level must be betwee 0 and 10. \n");
    
    dbg.init("", debugLevel, 0, (Output::output_location_t)params.find_integer("debug", 0));
    printStatsLoc = (Output::output_location_t)params.find_integer("statistics", 0);

    targetCount = 0;

	registerTimeBase("1 ns", true);

	memLink = configureLink("memory", "1 ns", new Event::Handler<DirectoryController>(this, &DirectoryController::handleMemoryResponse));
	assert(memLink);


    entryCacheMaxSize = (size_t)params.find_integer("entry_cache_size", 32768);
    entryCacheSize = 0;
    int addr = params.find_integer("network_address");
    std::string net_bw = params.find_string("network_bw");

	addrRangeStart  = (uint64_t)params.find_integer("addr_range_start", 0);
	addrRangeEnd    = (uint64_t)params.find_integer("addr_range_end", 0);
	interleaveSize  = (Addr)params.find_integer("interleave_size", 0);
    interleaveSize  *= 1024;
	interleaveStep  = (Addr)params.find_integer("interleave_step", 0);
    interleaveStep  *= 1024;

	if(0 == addrRangeEnd) addrRangeEnd = (uint64_t)-1;
    numTargets = 0;


    MemNIC::ComponentInfo myInfo;
    myInfo.link_port                        = "network";
    myInfo.link_bandwidth                   = net_bw;
	myInfo.num_vcs                          = params.find_integer("network_num_vc", 3);
    myInfo.name                             = getName();
    myInfo.network_addr                     = addr;
    myInfo.type                             = MemNIC::TypeDirectoryCtrl;
    myInfo.typeInfo.dirctrl.rangeStart      = addrRangeStart;
    myInfo.typeInfo.dirctrl.rangeEnd        = addrRangeEnd;
    myInfo.typeInfo.dirctrl.interleaveSize  = interleaveSize;
    myInfo.typeInfo.dirctrl.interleaveStep  = interleaveStep;
    network = new MemNIC(this, myInfo, new Event::Handler<DirectoryController>(this, &DirectoryController::handlePacket));


	registerClock(params.find_string("clock", "1GHz"), new Clock::Handler<DirectoryController>(this, &DirectoryController::clock));


    /*Parameter not needed since cache entries are always stored at address 0.
      Entries always kept in the cache, but memory is accessed to get performance metrics. */

    lookupBaseAddr      = 128;
    numReqsProcessed    = 0;
    totalReqProcessTime = 0;
    numCacheHits        = 0;
    dataReads           = 0;
    dataWrites          = 0;
    dirEntryReads       = 0;
    dirEntryWrites      = 0;
    
    GetXReqReceived     = 0;
    GetSExReqReceived   = 0;
    GetSReqReceived     = 0;
    
    PutMReqReceived     = 0;
    PutEReqReceived     = 0;
    PutSReqReceived     = 0;

}


DirectoryController::~DirectoryController(){
    for(std::map<Addr, DirEntry*>::iterator i = directory.begin(); i != directory.end() ; ++i) {
        delete i->second;
    }
    directory.clear();
    
    while(workQueue.size()) {
        MemEvent *front = workQueue.front();
        workQueue.pop_front();
        delete front;
    }
}


void DirectoryController::handleMemoryResponse(SST::Event *event){
	MemEvent *ev = static_cast<MemEvent*>(event);
    dbg.debug(_L10_, "\n\n----------------------------------------------------------------------------------------\n");
    dbg.debug(_L10_, "Directory Controller - %s - Memory response. Cmd = %s for address %"PRIx64"(Response to(%"PRIx64", %d))\n", getName().c_str(), CommandString[ev->getCmd()], ev->getAddr(), ev->getResponseToID().first, ev->getResponseToID().second);

    if(uncachedWrites.find(ev->getResponseToID()) != uncachedWrites.end()) {
        MemEvent *origEV = uncachedWrites[ev->getResponseToID()];
        uncachedWrites.erase(ev->getResponseToID());

        MemEvent *resp = origEV->makeResponse();
        sendResponse(resp);

        delete origEV;
    }
    else if(memReqs.find(ev->getResponseToID()) != memReqs.end()) {
        Addr targetBlock = memReqs[ev->getResponseToID()];
        memReqs.erase(ev->getResponseToID());

        if(GetSResp == ev->getCmd() || GetXResp == ev->getCmd()) {
            // Lookup complete, perform our work
            DirEntry *entry = getDirEntry(targetBlock);
            assert(entry);
            entry->inController = true;
            advanceEntry(entry, ev);
        }
        else  _abort(DirectoryController, "Received unexpected message from Memory!\n");
    }
    else{
        
        _abort(DirectoryController, "Unexpected event received\n"); /* Don't have this req recorded */
    }


    delete ev;
}

void DirectoryController::handlePacket(SST::Event *event){
    MemEvent *ev = static_cast<MemEvent*>(event);
    ev->setDeliveryTime(getCurrentSimTimeNano());
    workQueue.push_back(ev);
}


bool DirectoryController::processPacket(MemEvent *ev){
    assert(isRequestAddressValid(ev));
    dbg.debug(_L10_, "\n\n----------------------------------------------------------------------------------------\n");
    dbg.debug(_L10_, "Directory Controller: %s, %s req, Cmd = %s, BsAddr = %"PRIx64", Src = %s\n", getName().c_str(), ev->queryFlag(MemEvent::F_UNCACHED) ? "un$" : "$", CommandString[ev->getCmd()], ev->getBaseAddr(), ev->getSrc().c_str());
    //dbg.debug(_L10_, "Processing(%"PRIu64", %d) %s 0x%"PRIx64" from %s.  Status: %s\n", ev->getID().first, ev->getID().second, CommandString[ev->getCmd()], ev->getAddr(), ev->getSrc().c_str(), printDirectoryEntryStatus(ev->getAddr()));
    Command cmd = ev->getCmd();
    
    if(NACK == cmd) {
        MemEvent* origEvent = ev->getNACKedEvent();
        processIncomingNACK(origEvent);
        delete ev;
        return true;
    }
    
    
    uint32_t requesting_node;
    pair<bool, bool> ret = make_pair<bool, bool>(false, false);
    DirEntry *entry = getDirEntry(ev->getBaseAddr());
    if(cmd == FetchResp) assert(entry && entry->inProgress());
    
    
    if(entry && entry->inProgress()) ret = handleEntryInProgress(ev, entry, cmd);
    if(ret.first == true) return ret.second;


    
    /* New Request */
    switch(cmd) {
    case PutS:
        PutSReqReceived++;
        
        if(!entry) return true;  //TODO: this is dangerous
        if(entry->dirty) return true;  //ignore request

        entry->activeReq = ev;
        dbg.debug(_L10_, "\n\nDC PutS - %s - Request Received\n", getName().c_str());
        requesting_node = node_name_to_id(entry->activeReq->getSrc());
        entry->sharers[requesting_node]= false;
        resetEntry(entry);
        break;

    case PutM:
    case PutE:
        if(cmd == PutM)         PutMReqReceived++;
        else if(cmd == PutE)    PutEReqReceived++;
        
        assert(entry);
        dbg.debug(_L10_, "\n\nDC PutM - %s - Request Received\n", getName().c_str());
        entry->activeReq = ev;
        if(entry->inController) {
            ++numCacheHits;
            handlePutM(entry, ev);
        } else {
            dbg.debug(_L10_, "Entry %"PRIx64" not in cache.  Requesting from memory.\n", entry->baseAddr);
            entry->nextFunc = &DirectoryController::handlePutM;
            requestDirEntryFromMemory(entry);
        }
        break;
    
    case GetX:
    case GetSEx:
    case GetS:
        if(cmd == GetS)         GetSReqReceived++;
        else if(cmd == GetX)    GetXReqReceived++;
        else if(cmd == GetSEx)  GetSExReqReceived++;
        
        dbg.debug(_L10_, "\n\nDC GetS/GetX - %s - Request Received\n", getName().c_str());
        if(!entry) {
            entry = createDirEntry(ev->getBaseAddr(), ev->getAddr(), ev->getSize());
            entry->inController = true;
        }

        if(entry->inController) {
            ++numCacheHits;
            handleRequestData(entry, ev);
        } else {
            dbg.debug(_L10_, "Entry %"PRIx64" not in cache.  Requesting from memory.\n", entry->baseAddr);
            entry->nextFunc = &DirectoryController::handleRequestData;
            requestDirEntryFromMemory(entry);
        }
        break;
    default:
        /* Ignore unexpected */
        _abort(DirectoryController, "Cmd not expected, Cmd = %s\n", CommandString[cmd]);
        break;
    }
    return true;
}

void DirectoryController::processIncomingNACK(MemEvent* _origReqEvent){
    /* Re-send request */
    sendResponse(_origReqEvent);
    dbg.output("Orig Cmd NACKed = %s \n", CommandString[_origReqEvent->getCmd()]);
}



pair<bool, bool> DirectoryController::handleEntryInProgress(MemEvent *ev, DirEntry *entry, Command cmd){
    dbg.debug(_L10_, "Entry found and in progress\n");
        if((entry->nextCommand == cmd || (entry->nextCommand == FetchResp && cmd == PutM)) &&
          ("N/A" == entry->waitingOn || entry->waitingOn == ev->getSrc())) {
            dbg.debug(_L10_, "Incoming command matches for 0x%"PRIx64" in progress.\n", entry->baseAddr);
            if(ev->getResponseToID() != entry->lastRequest) {
                dbg.debug(_L10_, "This isn't a response to our request, but it fulfills the need.  Placing(%"PRIu64", %d) into list of ignorable responses.\n", entry->lastRequest.first, entry->lastRequest.second);
            }
            advanceEntry(entry, ev);
            delete ev;
            return make_pair<bool, bool>(true, true);
        }
        else{
            dbg.debug(_L10_, "Incoming command [%s,%s] doesn't match for 0x%"PRIx64" [%s,%s] in progress.\n", CommandString[ev->getCmd()], ev->getSrc().c_str(), entry->baseAddr, CommandString[entry->nextCommand], entry->waitingOn.c_str());
            return make_pair<bool, bool>(true, false);
        }
    return make_pair<bool, bool>(false, false);

}



void DirectoryController::printStatus(Output &out){
    out.output("MemHierarchy::DirectoryController %s\n", getName().c_str());
    out.output("\t# Entries in cache:  %zu\n", entryCacheSize);
    out.output("\t# Requests in queue:  %zu\n", workQueue.size());
    for(std::list<MemEvent*>::iterator i = workQueue.begin() ; i != workQueue.end() ; ++i) {
        out.output("\t\t(%"PRIu64", %d)\n", (*i)->getID().first, (*i)->getID().second);
    }
    out.output("\tRequests in Progress:\n");
    for(std::map<Addr, DirEntry*>::iterator i = directory.begin() ; i != directory.end() ; ++i) {
        if(i->second->inProgress())
            out.output("\t\t0x%"PRIx64"\t\t(%"PRIu64", %d)\n",i->first, i->second->activeReq->getID().first, i->second->activeReq->getID().second);
    }

}


bool DirectoryController::clock(SST::Cycle_t cycle){
    network->clock();

    if(!workQueue.empty()){
        MemEvent *event = workQueue.front();
        bool ret = processPacket(event);
        if(ret) workQueue.erase(workQueue.begin());
        else {
            workQueue.pop_front();
            workQueue.push_back(event);
        }
	}

	return false;
}



DirectoryController::DirEntry* DirectoryController::getDirEntry(Addr baseAddr){
	std::map<Addr, DirEntry*>::iterator i = directory.find(baseAddr);
	if(directory.end() == i) return NULL;
	return i->second;
}


DirectoryController::DirEntry* DirectoryController::createDirEntry(Addr baseAddr, Addr addr, uint32_t reqSize){
    dbg.debug(_L10_, "Creating Directory Entry for 0x%"PRIx64"\n", baseAddr);
	DirEntry *entry = new DirEntry(baseAddr, addr, reqSize, numTargets);
    entry->cacheIter = entryCache.end();
	directory[baseAddr] = entry;
	return entry;
}


void DirectoryController::sendInvalidate(int target, DirEntry* entry){
    MemEvent *me = new MemEvent(this, entry->activeReq->getAddr(), entry->activeReq->getBaseAddr(), Inv, entry->activeReq->getSize());
    me->setDst(nodeid_to_name[target]);
    network->send(me);
}


void DirectoryController::handleRequestData(DirEntry* entry, MemEvent *new_ev){
    entry->activeReq = new_ev;
    entry->reqSize = new_ev->getSize();
	uint32_t requesting_node = node_id(entry->activeReq->getSrc());
	if(entry->dirty) {
		// Must do a fetch
		Command cmd = FetchInv;
        
        if(entry->activeReq->getCmd() == GetX || entry->activeReq->getCmd() == GetSEx)
            cmd = FetchInv;

		MemEvent *ev = new MemEvent(this, entry->activeReq->getAddr(), entry->activeReq->getBaseAddr(), cmd, entry->activeReq->getSize());
        std::string &dest = nodeid_to_name[entry->findOwner()];
        ev->setDst(dest);

		entry->nextFunc = &DirectoryController::finishFetch;
        entry->nextCommand = FetchResp;
        entry->waitingOn = dest;
        entry->lastRequest = ev->getID();

		sendResponse(ev);
        dbg.debug(_L10_, "Sending %s to %s to fulfill request for data for 0x%"PRIx64".\n", CommandString[cmd], dest.c_str(), entry->baseAddr);

	}
    else if(entry->activeReq->getCmd() == GetX || entry->activeReq->getCmd() == GetSEx) {
        bool invSent = false;
        // Must send invalidates
		assert(0 == entry->waitingAcks);
		for(uint32_t i = 0 ; i < numTargets ; i++) {
			if(i == requesting_node) continue;
			if(entry->sharers[i]) {
				sendInvalidate(i, entry);
				invSent = true;
			}
		}
        if(invSent) dbg.debug(_L10_, "Sending Invalidates\n");
		
		getExclusiveDataForRequest(entry, NULL);

	}
    else if(entry->activeReq->queryFlag(MemEvent::F_UNCACHED)) {
        // Don't set as a sharer whne dealing with uncached
		entry->nextFunc = &DirectoryController::sendRequestedData;
		requestDataFromMemory(entry);
    }
    else{
        //Handle GetS requests
		entry->sharers[requesting_node]= true;
		entry->nextFunc = &DirectoryController::sendRequestedData;
		requestDataFromMemory(entry);
	}
}

void DirectoryController::finishFetch(DirEntry* entry, MemEvent *new_ev){
	if(entry->activeReq->getCmd() == GetX || entry->activeReq->getCmd() == GetSEx) {
		entry->dirty = true;
        entry->clearSharers();
	}
    else entry->dirty = false;

    if(!entry->activeReq->queryFlag(MemEvent::F_UNCACHED))
        entry->sharers[node_id(entry->activeReq->getSrc())] = true;

	MemEvent *ev = entry->activeReq->makeResponse();
	ev->setPayload(new_ev->getPayload());
	sendResponse(ev);
	writebackData(new_ev);

	updateEntryToMemory(entry);
}



void DirectoryController::sendRequestedData(DirEntry* entry, MemEvent *new_ev){
	MemEvent *ev = entry->activeReq->makeResponse();
	//TODO:  only if write should you set the payload
    ev->setPayload(new_ev->getPayload());
    dbg.debug(_L10_, "Sending requested data for 0x%"PRIx64" to %s, granted state = %s\n", entry->baseAddr, ev->getDst().c_str(), BccLineString[ev->getGrantedState()]);
	sendResponse(ev);
    
    if(entry->activeReq->queryFlag(MemEvent::F_UNCACHED) && 0 == entry->countRefs()) directory.erase(entry->baseAddr);
    else updateEntryToMemory(entry);

    delete entry->activeReq;
}


void DirectoryController::getExclusiveDataForRequest(DirEntry* entry, MemEvent *new_ev){
	assert(0 == entry->waitingAcks);

	uint32_t target_id = node_id(entry->activeReq->getSrc());
    entry->clearSharers();
    if(!entry->activeReq->queryFlag(MemEvent::F_UNCACHED)) entry->sharers[target_id] = true;
	entry->dirty = true;

	entry->nextFunc = &DirectoryController::sendRequestedData;
	requestDataFromMemory(entry);
}



void DirectoryController::handlePutM(DirEntry *entry, MemEvent *ev){
    dbg.debug(_L10_, "Entry 0x%"PRIx64" loaded.  Performing writeback of 0x%"PRIx64" for %s\n", entry->baseAddr, entry->activeReq->getAddr(), entry->activeReq->getSrc().c_str());
    assert(entry->dirty);
    assert(entry->findOwner() == node_lookup[entry->activeReq->getSrc()]);
    entry->dirty = false;
    if(!entry->activeReq->queryFlag(MemEvent::F_UNCACHED))
        entry->sharers[node_id(entry->activeReq->getSrc())] = false; //or true?
    if(ev->getCmd() == PutM) writebackData(entry->activeReq);
	updateEntryToMemory(entry);
}

void DirectoryController::handlePutS(DirEntry *entry, MemEvent *ev){
    entry->activeReq = ev;
    dbg.debug(_L10_, "\n\nDC PutS - %s - Request Received\n", getName().c_str());
    uint32_t requesting_node = node_name_to_id(entry->activeReq->getSrc());
    entry->sharers[requesting_node]= false;
    resetEntry(entry);
}



/* Advance the processing of this directory entry */
void DirectoryController::advanceEntry(DirEntry *entry, MemEvent *ev){
	assert(NULL != entry->nextFunc);
	(this->*(entry->nextFunc))(entry, ev);
}


uint32_t DirectoryController::node_id(const std::string &name){
	uint32_t id;
	std::map<std::string, uint32_t>::iterator i = node_lookup.find(name);
	if(node_lookup.end() == i) {
		node_lookup[name] = id = targetCount++;
        nodeid_to_name.resize(targetCount);
        nodeid_to_name[id] = name;
	}
    else id = i->second;
    
	return id;
}

uint32_t DirectoryController::node_name_to_id(const std::string &name){
   
	std::map<std::string, uint32_t>::iterator i = node_lookup.find(name);
    assert(node_lookup.end() != i);
	uint32_t id = i->second;
    return id;
}


void DirectoryController::requestDirEntryFromMemory(DirEntry *entry){
	Addr entryAddr = 0; /*  Offset into our buffer? */
	MemEvent *me = new MemEvent(this, entryAddr, GetS);
	me->setSize(entrySize);
    memReqs[me->getID()] = entry->baseAddr;
    entry->nextCommand = MemEvent::commandResponse(entry->activeReq->getCmd());
    entry->waitingOn = "memory";
    entry->lastRequest = me->getID();
    dbg.debug(_L10_, "Requesting Entry from memory for 0x%"PRIx64"(%"PRIu64", %d)\n", entry->baseAddr, me->getID().first, me->getID().second);
	memLink->send(me);
    ++dirEntryReads;
}


void DirectoryController::requestDataFromMemory(DirEntry *entry){
    Addr localAddr = convertAddressToLocalAddress(entry->activeReq->getAddr());
    Addr localBaseAddr = convertAddressToLocalAddress(entry->activeReq->getBaseAddr());
    MemEvent *ev = new MemEvent(this, localAddr, localBaseAddr, entry->activeReq->getCmd(), entry->activeReq->getSize());
    memReqs[ev->getID()] = entry->baseAddr;
    entry->nextCommand = MemEvent::commandResponse(entry->activeReq->getCmd());
    entry->waitingOn = "memory";
    entry->lastRequest = ev->getID();
    dbg.debug(_L10_, "Requesting data from memory at 0x%"PRIx64"(%"PRIu64", %d)\n", entry->baseAddr, ev->getID().first, ev->getID().second);
    memLink->send(ev);
    ++dataReads;
}

void DirectoryController::updateCacheEntry(DirEntry *entry){
    if(0 == entryCacheMaxSize) {
        sendEntryToMemory(entry);
    } else {
        /* Find if we're in the cache */
        if(entry->cacheIter != entryCache.end()) {
            entryCache.erase(entry->cacheIter);
            --entryCacheSize;
            entry->cacheIter = entryCache.end();
        }

        /* Find out if we're no longer cached, and just remove */
        if((!entry->activeReq) &&(0 == entry->countRefs())) {
            dbg.debug(_L10_, "Entry for 0x%"PRIx64" has no referenes - purging\n", entry->baseAddr);
            directory.erase(entry->baseAddr);
            delete entry;
            return;
        } else {

            entryCache.push_front(entry);
            entry->cacheIter = entryCache.begin();
            ++entryCacheSize;

            while(entryCacheSize > entryCacheMaxSize) {
                DirEntry *oldEntry = entryCache.back();
                // If the oldest entry is still in progress, everything is in progress
                if(oldEntry->inProgress()) break;

                dbg.debug(_L10_, "entryCache too large.  Evicting entry for 0x%"PRIx64"\n", oldEntry->baseAddr);
                entryCache.pop_back();
                --entryCacheSize;
                oldEntry->cacheIter = entryCache.end();
                sendEntryToMemory(oldEntry);
            }
        }
    }
}


void DirectoryController::updateEntryToMemory(DirEntry *entry){
    resetEntry(entry);
    updateCacheEntry(entry);
}


void DirectoryController::sendEntryToMemory(DirEntry *entry){
	Addr entryAddr = 0; /*  Offset into our buffer? */
	MemEvent *me = new MemEvent(this, entryAddr, PutM); //PutM?
    dbg.debug(_L10_, "Updating entry for 0x%"PRIx64" to memory(%"PRIu64", %d)\n", entry->baseAddr, me->getID().first, me->getID().second);
	me->setSize(entrySize);
    memReqs[me->getID()] = entry->baseAddr;
	memLink->send(me);
    ++dirEntryWrites;
}


MemEvent::id_type DirectoryController::writebackData(MemEvent *data_event){
    Addr localBaseAddr = convertAddressToLocalAddress(data_event->getBaseAddr());
	MemEvent *ev = new MemEvent(this, localBaseAddr, localBaseAddr, PutM, data_event->getPayload());
    dbg.debug(_L10_, "Writing back data to 0x%"PRIx64"(%"PRIu64", %d)\n", data_event->getAddr(), ev->getID().first, ev->getID().second);

	memLink->send(ev);
    ++dataWrites;

    return ev->getID();
}


void DirectoryController::resetEntry(DirEntry *entry){
	if(entry->activeReq) {
        dbg.debug(_L10_, "Resetting entry after event(%"PRIu64", %d) %s 0x%"PRIx64".  Processing time: %"PRIu64"\n",
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


void DirectoryController::sendResponse(MemEvent *ev){
    dbg.debug(_L10_, "Sending %s 0x%"PRIx64" to %s\n", CommandString[ev->getCmd()], ev->getAddr(), ev->getDst().c_str());
	network->send(ev);
}


bool DirectoryController::isRequestAddressValid(MemEvent *ev){
    Addr addr = ev->getBaseAddr();

    if(0 == interleaveSize) {
        return(addr >= addrRangeStart && addr < addrRangeEnd);
    } else {
        if(addr < addrRangeStart) return false;
        if(addr >= addrRangeEnd) return false;

        addr = addr - addrRangeStart;

        Addr offset = addr % interleaveStep;
        if(offset >= interleaveSize) return false;

        return true;
    }

}


Addr DirectoryController::convertAddressToLocalAddress(Addr addr){
    Addr res = 0;
    if(0 == interleaveSize) {
        res = lookupBaseAddr + addr - addrRangeStart;
    } else {
        addr = addr - addrRangeStart;
        Addr step = addr / interleaveStep;
        Addr offset = addr % interleaveStep;
        res = lookupBaseAddr +(step * interleaveSize) + offset;
    }
    dbg.debug(_L10_, "Converted physical address 0x%"PRIx64" to ACTUAL memory address 0x%"PRIx64"\n", addr, res);
    return res;
}


static char dirEntStatus[1024] = {0};
const char* DirectoryController::printDirectoryEntryStatus(Addr baseAddr){
    DirEntry *entry = getDirEntry(baseAddr);
    if(!entry) {
        sprintf(dirEntStatus, "[Not Created]");
    } else {
        uint32_t refs = entry->countRefs();

        if(0 == refs) sprintf(dirEntStatus, "[Uncached]");
        else if(entry->dirty) {
            uint32_t owner = entry->findOwner();
            sprintf(dirEntStatus, "[owned by %s]", nodeid_to_name[owner].c_str());
        }
        else sprintf(dirEntStatus, "[Shared by %u]", refs);


    }
    return dirEntStatus;
}


void DirectoryController::init(unsigned int phase){
    network->init(phase);

    /* Pass data on to memory */
    while(MemEvent *ev = network->recvInitData()) {
        dbg.debug(_L10_, "Found Init Info for address 0x%"PRIx64"\n", ev->getAddr());
        if(isRequestAddressValid(ev)) {
            ev->setBaseAddr(convertAddressToLocalAddress(ev->getBaseAddr()));
            ev->setAddr(convertAddressToLocalAddress(ev->getAddr()));
            dbg.debug(_L10_, "Sending Init Data for address 0x%"PRIx64" to memory\n", ev->getAddr());
            memLink->sendInitData(ev);
        }
        else delete ev;
    }

}

void DirectoryController::finish(void){
    network->finish();

    Output out("", 0, 0, printStatsLoc);
    out.output("\n--------------------------------------------------------------------\n");
    out.output("--- Directory Controller\n");
    out.output("--- Name: %s\n", getName().c_str());
    out.output("--------------------------------------------------------------------\n");
    out.output("- Total requests received:  %"PRIu64"\n", numReqsProcessed);
    out.output("- GetS recieved:  %"PRIu64"\n", GetSReqReceived);
    out.output("- GetX received:  %"PRIu64"\n", GetXReqReceived);
    out.output("- GetSEx recieved:  %"PRIu64"\n", GetSExReqReceived);
    out.output("- PutM received:  %"PRIu64"\n", PutMReqReceived);
    out.output("- PutE received:  %"PRIu64"\n", PutEReqReceived);
    out.output("- PutS received:  %"PRIu64"\n", PutSReqReceived);
    out.output("- Entry Data Reads:  %"PRIu64"\n", dirEntryReads);
    out.output("- Entry Data Writes:  %"PRIu64"\n", dirEntryWrites);
    out.output("- Avg Req Time:  %"PRIu64" ns\n", (numReqsProcessed > 0) ? totalReqProcessTime / numReqsProcessed : 0);
    out.output("- Entry Cache Hits:  %"PRIu64"\n", numCacheHits);
}


void DirectoryController::setup(void){
    network->setup();

    const std::vector<MemNIC::ComponentInfo> &ci = network->getPeerInfo();
    for(std::vector<MemNIC::ComponentInfo>::const_iterator i = ci.begin() ; i != ci.end() ; ++i) {
        dbg.debug(_L10_, "DC found peer %d(%s) of type %d.\n", i->network_addr, i->name.c_str(), i->type);
        if(MemNIC::TypeCache == i->type) {
            numTargets++;
            if(blocksize)   assert(blocksize == i->typeInfo.cache.blocksize);
            else            blocksize = i->typeInfo.cache.blocksize;
        }
    }
    if(0 == numTargets) _abort(DirectoryController, "Directory Controller %s unable to see any caches.\n", getName().c_str());

    network->clearPeerInfo();
    entrySize = (numTargets+1)/8 +1;
}

