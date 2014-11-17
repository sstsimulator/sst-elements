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
    cacheLineSize = params.find_integer("cache_line_size", 64);
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
    protocol        = params.find_string("coherence_protocol", "");
    dbg.debug(_L5_, "Directory controller using protocol: %s\n", protocol.c_str());
    
    int mshrSize    = params.find_integer("mshr_num_entries",-1);
    if (mshrSize == -1) mshrSize = HUGE_MSHR;
    if (mshrSize < 10) _abort(DirectoryController, "MSHR should have at least 10 entries");
    mshr            = new   MSHR(&dbg, mshrSize); 

    if(0 == addrRangeEnd) addrRangeEnd = (uint64_t)-1;
    numTargets = 0;
    
    /* Check parameter validity */
    assert(protocol == "MESI" || protocol == "mesi" || protocol == "MSI" || protocol == "msi");
    if (protocol == "mesi") protocol = "MESI";
    if (protocol == "msi") protocol = "MSI";

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

    lookupBaseAddr      = 0;  /* Use to offset into main memory from where DirEntries are stored */
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
    for(std::map<Addr, DirEntry*>::iterator i = directory.begin(); i != directory.end() ; ++i){
        delete i->second;
    }
    directory.clear();
    
    while(workQueue.size()){
        MemEvent *front = workQueue.front();
        workQueue.pop_front();
        delete front;
    }
}



void DirectoryController::handlePacket(SST::Event *event){
    MemEvent *ev = static_cast<MemEvent*>(event);
    ev->setDeliveryTime(getCurrentSimTimeNano());
    workQueue.push_back(ev);
}



bool DirectoryController::clock(SST::Cycle_t cycle){
    network->clock();   // what does this do?

    if(!workQueue.empty()){
        MemEvent *event = workQueue.front();
        processPacket(event);
        workQueue.erase(workQueue.begin());
    }

	return false;
}



bool DirectoryController::processPacket(MemEvent *ev){
    assert(isRequestAddressValid(ev));
    dbg.debug(_L10_, "\n\n----------------------------------------------------------------------------------------\n");
    dbg.debug(_L10_, "Directory Controller: %s, Proc Pkt: id (%" PRIu64 ",%d) Cmd = %s, BsAddr = 0x%"PRIx64", Src = %s\n",
            getName().c_str(), ev->getID().first, ev->getID().second, CommandString[ev->getCmd()],
            ev->getBaseAddr(), ev->getSrc().c_str());
    Command cmd = ev->getCmd();
    
    if(NACK == cmd){
        MemEvent* origEvent = ev->getNACKedEvent();
        processIncomingNACK(origEvent,ev);
        delete ev;
        return true;
    }
    
    uint32_t requesting_node;
    pair<bool, bool> ret = make_pair<bool, bool>(false, false);
    DirEntry *entry = getDirEntry(ev->getBaseAddr());
    
    if(entry && entry->waitingAcks > 0 && PutS == ev->getCmd()){
        handlePutS(ev);
        delete ev;
        return true;
    }
    
    if(entry && entry->inProgress()) ret = handleEntryInProgress(ev, entry, cmd);
    if(ret.first == true && ret.second == false) { // new request which conflicts with current request
        if (!(mshr->elementIsHit(ev->getBaseAddr(),ev))) {
            bool inserted = mshr->insert(ev->getBaseAddr(),ev);
            dbg.debug(_L10_, "Inserting conflicting request in mshr. Cmd = %s, BaseAddr = 0x%"PRIx64", Addr = 0x%"PRIx64", MSHR size: %d\n", CommandString[ev->getCmd()], ev->getBaseAddr(), ev->getAddr(), mshr->getSize());
            if (!inserted) {
                dbg.debug(_L10_, "MSHR is full. NACKing request\n");
                mshrNACKRequest(ev);
                return true;
            }
        } else {
            dbg.debug(_L10_, "Conflicting request already in mshr. Cmd = %s, BaseAddr = 0x%"PRIx64", Addr = 0x%"PRIx64", MSHR size: %d\n", CommandString[ev->getCmd()], ev->getBaseAddr(), ev->getAddr(), mshr->getSize());
        }
    }
    if(ret.first == true) return ret.second;


    /* New Request */
    switch(cmd){
        case PutS:
            assert(entry);
            PutSReqReceived++;
            requesting_node = node_name_to_id(ev->getSrc());
            entry->removeSharer(requesting_node);
            if(entry->countRefs() == 0) resetEntry(entry);
            if(mshr->elementIsHit(ev->getBaseAddr(),ev)) mshr->removeElement(ev->getBaseAddr(),ev);
            delete ev;
            break;
            
        case PutM:
        case PutE:
            assert(entry);

            if(cmd == PutM)         PutMReqReceived++;
            else if(cmd == PutE)    PutEReqReceived++;
            
            if(entry->inController){
                entry->activeReq = ev;
                ++numCacheHits;
                handlePutM(entry, ev);
            } else {
                if (!(mshr->elementIsHit(ev->getBaseAddr(),ev))) {
                    bool inserted = mshr->insert(ev->getBaseAddr(),ev);
                    dbg.debug(_L10_, "Inserting request in mshr. Cmd = %s, BaseAddr = 0x%"PRIx64", Addr = 0x%"PRIx64", MSHR size: %d\n", CommandString[ev->getCmd()], ev->getBaseAddr(), ev->getAddr(), mshr->getSize());
                    if (!inserted) {
                        dbg.debug(_L10_, "MSHR is full. NACKing request\n");
                        mshrNACKRequest(ev);
                        return true;
                    }
                    entry->activeReq = ev;
                }
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
           
            /* Create directory entry if this is a new block */
            if(!entry){
                entry = createDirEntry(ev->getBaseAddr(), ev->getAddr(), ev->getSize());
                entry->inController = true;
            }

            /* Insert event in mshrs */
            if (!(mshr->elementIsHit(ev->getBaseAddr(),ev))) {
                bool inserted = mshr->insert(ev->getBaseAddr(),ev);
                dbg.debug(_L10_, "Inserting request in mshr. Cmd = %s, BaseAddr = 0x%"PRIx64", Addr = 0x%"PRIx64", MSHR size: %d\n", CommandString[ev->getCmd()], ev->getBaseAddr(), ev->getAddr(), mshr->getSize());
                if (!inserted) {
                    dbg.debug(_L10_, "MSHR is full. NACKing request\n");
                    mshrNACKRequest(ev);
                    return true;
                }
            }
            
            entry->activeReq = ev; // should also be the top request in the mshrs for this addr

            if(entry->inController){
                ++numCacheHits;
                handleDataRequest(entry, ev);
            }
            else{
                dbg.debug(_L10_, "Entry %"PRIx64" not in cache.  Requesting from memory.\n", entry->baseAddr);
                entry->nextFunc = &DirectoryController::handleDataRequest;
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



void DirectoryController::handleMemoryResponse(SST::Event *event){
    MemEvent *ev = static_cast<MemEvent*>(event);
    dbg.debug(_L10_, "\n\n----------------------------------------------------------------------------------------\n");
    dbg.debug(_L10_, "Directory Controller: %s, MemResp: Cmd = %s, BaseAddr = 0x%"PRIx64", Size = %u \n", getName().c_str(), CommandString[ev->getCmd()], ev->getAddr(), ev->getSize());

    //MemEvent *origRequest = getOrigReq(mshr->lookup(ev->getAddr()));
    
    //DirEntry *entr = getDirEntry(mshr->lookupFront(ev->getAddr()));
    Addr target = ev->getBaseAddr();
    if (target == 0 && dirEntryMiss.find(ev->getResponseToID()) != dirEntryMiss.end()) {    // directory entry miss
        target = dirEntryMiss[ev->getResponseToID()];
        dirEntryMiss.erase(ev->getResponseToID());
    } else {
        MemEvent *origReq = mshr->lookupFront(ev->getBaseAddr());
        target = origReq->getBaseAddr();
    }
    

    
    if(memReqs.find(ev->getResponseToID()) != memReqs.end()){
        Addr targetBlock = memReqs[ev->getResponseToID()];
        memReqs.erase(ev->getResponseToID());
        dbg.debug(_L10_, "Directory controller: target: %"PRIx64"; targetBlock: %"PRIx64"\n",target,targetBlock); 
        if(GetSResp == ev->getCmd() || GetXResp == ev->getCmd()){   // Lookup complete, perform our work
            DirEntry *entry = getDirEntry(targetBlock);
            DirEntry *entr = getDirEntry(target);
            assert(entry);
            assert(entry == entr);
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

void DirectoryController::mshrNACKRequest(MemEvent* ev) {
    MemEvent * nackEv = ev->makeNACKResponse(this,ev);
    sendResponse(nackEv);
}


void DirectoryController::processIncomingNACK(MemEvent* _origReqEvent, MemEvent* _nackEvent){
    DirEntry *entry = getDirEntry(_origReqEvent->getBaseAddr());
    dbg.debug(_L10_, "Orig resp ID = (%" PRIu64 ",%d), Nack resp ID = (%" PRIu64 ",%d), last req ID = (%" PRIu64 ",%d)\n", 
	_origReqEvent->getResponseToID().first, _origReqEvent->getResponseToID().second, _nackEvent->getResponseToID().first, 
	_nackEvent->getResponseToID().second, entry->lastRequest.first, entry->lastRequest.second);
    
    /* Retry request if it has not already been handled */
    if ((_nackEvent->getResponseToID() == entry->lastRequest) || _origReqEvent->getCmd() == Inv) {
	/* Re-send request */
	sendResponse(_origReqEvent);
	dbg.output("Orig Cmd NACKed, retry = %s \n", CommandString[_origReqEvent->getCmd()]);
    } else {
	dbg.output("Orig Cmd NACKed, no retry = %s \n", CommandString[_origReqEvent->getCmd()]);
    }
}


/*
 *  Return value indicates (successful, erase packet from queue)
 */
pair<bool, bool> DirectoryController::handleEntryInProgress(MemEvent *ev, DirEntry *entry, Command cmd){
    dbg.debug(_L10_, "Entry found and in progress\n");
        if((entry->nextCommand == cmd || 
                    (entry->nextCommand == FetchResp && cmd == PutM) || 
                    (entry->nextCommand == GetSResp && cmd == PutS)) &&
          ("N/A" == entry->waitingOn || entry->waitingOn == ev->getSrc())){
            dbg.debug(_L10_, "Incoming command matches for 0x%"PRIx64" in progress.\n", entry->baseAddr);
            if(ev->getResponseToID() != entry->lastRequest){
                dbg.debug(_L10_, "This isn't a response to our request, but it fulfills the need.  Placing(%"PRIu64", %d) into list of ignorable responses.\n", entry->lastRequest.first, entry->lastRequest.second);
            }
            advanceEntry(entry, ev);
            delete ev;
            return make_pair<bool, bool>(true, true);
        } else if (entry->nextCommand == FetchResp && cmd == PutE) {    // exclusive replacement raced with a fetch, re-direct to memory
            dbg.debug(_L10_, "Entry %"PRIx64" request raced with replacement. Handling as a miss to memory.\n", entry->baseAddr);
            
            assert(entry->isDirty());
            assert(entry->findOwner() == node_name_to_id(ev->getSrc()));
            int id = node_name_to_id(ev->getSrc());
            entry->clearDirty(id);
            entry->nextFunc = &DirectoryController::handleDataRequest;
            advanceEntry(entry, ev);
            delete ev;
            return make_pair<bool, bool>(true,true);
        }
        else{
            assert(!(entry->nextCommand == FetchResp && cmd == PutS));
            dbg.debug(_L10_, "Incoming command [%s,%s] doesn't match for 0x%"PRIx64" [%s,%s] in progress.\n", CommandString[ev->getCmd()], ev->getSrc().c_str(), entry->baseAddr, CommandString[entry->nextCommand], entry->waitingOn.c_str());
            if (!(entry->activeReq->getCmd() == PutM || entry->activeReq->getCmd() == PutE || entry->activeReq->getCmd() == PutS)) {
                assert(entry->nextCommand != NULLCMD);
            }
            return make_pair<bool, bool>(true, false);
        }
    return make_pair<bool, bool>(false, false);

}



void DirectoryController::printStatus(Output &out){
    out.output("MemHierarchy::DirectoryController %s\n", getName().c_str());
    out.output("\t# Entries in cache:  %zu\n", entryCacheSize);
    out.output("\t# Requests in queue:  %zu\n", workQueue.size());
    for(std::list<MemEvent*>::iterator i = workQueue.begin() ; i != workQueue.end() ; ++i){
        out.output("\t\t(%"PRIu64", %d)\n", (*i)->getID().first, (*i)->getID().second);
    }
    out.output("\tRequests in Progress:\n");
    for(std::map<Addr, DirEntry*>::iterator i = directory.begin() ; i != directory.end() ; ++i){
        if(i->second->inProgress())
            out.output("\t\t0x%"PRIx64"\t\t(%"PRIu64", %d)\n",i->first, i->second->activeReq->getID().first, i->second->activeReq->getID().second);
    }

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
    MemEvent *me = new MemEvent(this, entry->activeReq->getAddr(), entry->activeReq->getBaseAddr(), Inv, cacheLineSize);
    me->setDst(nodeid_to_name[target]);
    me->setAckNeeded();
    me->setRqstr(entry->activeReq->getRqstr());
    dbg.debug(_L10_, "Sending Invalidate.  Dst: %s\n", nodeid_to_name[target].c_str());
    network->send(me);
}


/**
 *  Handle data request (Get) to directory controller from caches
 */
void DirectoryController::handleDataRequest(DirEntry* entry, MemEvent *new_ev){
    entry->reqSize     = entry->activeReq->getSize();
    Command cmd        = entry->activeReq->getCmd();
    
    uint32_t requesting_node = node_id(entry->activeReq->getSrc());
    
    if(entry->activeReq->queryFlag(MemEvent::F_NONCACHEABLE)){      // Don't set as a sharer whne dealing with noncacheable
	entry->nextFunc = &DirectoryController::sendResponse;
	requestDataFromMemory(entry);
    } else if(entry->isDirty()){                                      // Must do a fetch
	MemEvent *ev      = new MemEvent(this, entry->activeReq->getAddr(), entry->activeReq->getBaseAddr(), FetchInv, cacheLineSize);
        std::string &dest = nodeid_to_name[entry->findOwner()];
        ev->setDst(dest);

	entry->nextFunc    = &DirectoryController::finishFetch;
        entry->nextCommand = FetchResp;
        entry->waitingOn   = dest;
        entry->lastRequest = ev->getID();

	sendResponse(ev);
    } else if(cmd == GetX || cmd == GetSEx){
	assert(0 == entry->waitingAcks);
	for(uint32_t i = 0 ; i < numTargets ; i++){                 // Must send invalidates
	    if(i == requesting_node) continue;
	    if(entry->sharers[i]){
                sendInvalidate(i, entry);
                entry->waitingAcks++;
            }
	}
        if( entry->waitingAcks > 0){
             entry->nextFunc = &DirectoryController::getExclusiveDataForRequest;
             entry->nextCommand = PutS;
             entry->waitingOn = "N/A";
             entry->lastRequest = DirEntry::NO_LAST_REQUEST;
             dbg.output(CALL_INFO, "Sending Invalidates to fulfill request for exclusive, BsAddr = %"PRIx64".\n", entry->baseAddr);
        } else getExclusiveDataForRequest(entry, NULL);

    } else {                                                        //Handle GetS requests
	entry->addSharer(requesting_node);
	entry->nextFunc = &DirectoryController::sendResponse;
	requestDataFromMemory(entry);
    }
}



void DirectoryController::finishFetch(DirEntry* entry, MemEvent *new_ev){
    dbg.debug(_L10_, "Finishing Fetch. Writing data to memory. \n");
    int wb_id         = node_name_to_id(new_ev->getSrc());
    int req_id        = node_name_to_id(entry->activeReq->getSrc());
    Command cmd       = entry->activeReq->getCmd();

    entry->clearDirty(wb_id);
    
    if(cmd == GetX || cmd == GetSEx) entry->setDirty(req_id);
    else entry->addSharer(req_id);

	MemEvent *ev = entry->activeReq->makeResponse();
	ev->setPayload(new_ev->getPayload());
	sendResponse(ev);
	writebackData(new_ev);
	updateEntryToMemory(entry);
}



void DirectoryController::sendResponse(DirEntry* entry, MemEvent *new_ev){
    //bool noncacheable = entry->activeReq->queryFlag(MemEvent::F_NONCACHEABLE);
    MemEvent *ev;
    if (entry->activeReq->getCmd() == GetS) {                   // Handle GetS requests according to coherence protocol
        if (protocol == "MESI" && entry->countRefs() == 1) {    // Return block in E state if this is the only requestor
            uint32_t requesting_node = node_name_to_id(entry->activeReq->getSrc());
            assert(entry->sharers[requesting_node]);            // ensure we are responding to the right requestor
            entry->clearSharers();
            entry->setDirty(requesting_node);                   
            ev = entry->activeReq->makeResponse(E);             
        } else {                                                // Assume protocol is MSI, enough error checking already exists to make sure protocol is valid
            ev = entry->activeReq->makeResponse();
        }
    } else {
        ev = entry->activeReq->makeResponse();
    }

    ev->setSize(cacheLineSize);
    ev->setPayload(new_ev->getPayload());
    sendResponse(ev);
    
    dbg.debug(_L10_, "Sending requested data for 0x%"PRIx64" to %s, granted state = %s\n", entry->baseAddr, ev->getDst().c_str(), BccLineString[ev->getGrantedState()]);

    updateEntryToMemory(entry);
    //if(noncacheable && 0 == entry->countRefs()) directory.erase(entry->baseAddr);
    //else updateEntryToMemory(entry);

}


void DirectoryController::getExclusiveDataForRequest(DirEntry* entry, MemEvent *new_ev){
    uint32_t requesting_node = node_name_to_id(entry->activeReq->getSrc());
	
    assert(0 == entry->waitingAcks);
    assert(entry->countRefs() <= 1);
    if(entry->countRefs() == 1) assert(entry->sharers[requesting_node]);

    entry->clearSharers();
    
    entry->setDirty(requesting_node);

	entry->nextFunc = &DirectoryController::sendResponse;
	requestDataFromMemory(entry);
}



void DirectoryController::handlePutS(MemEvent* ev){
    DirEntry *entry = getDirEntry(ev->getAddr());
    assert(entry && entry->waitingAcks > 0);
    int requesting_node = node_name_to_id(ev->getSrc());
    entry->removeSharer(requesting_node);
    entry->waitingAcks--;
    if(0 == entry->waitingAcks) advanceEntry(entry);
}



void DirectoryController::handlePutM(DirEntry *entry, MemEvent *ev){
    assert(entry->isDirty());
    assert(entry->findOwner() == node_name_to_id(entry->activeReq->getSrc()));
    int id = node_name_to_id(entry->activeReq->getSrc());

    entry->clearDirty(id);

    if(ev->getCmd() == PutM || ev->getCmd() == GetSResp) {
        writebackData(entry->activeReq);
    } else if (ev->getCmd() != PutE) { // ok to drop a PutE -> block is not really dirty
        dbg.debug(_L10_, "Alert! dropping PutM\n");
    }
	
    updateEntryToMemory(entry);

}



/* Advance the processing of this directory entry */
void DirectoryController::advanceEntry(DirEntry *entry, MemEvent *ev){
	assert(NULL != entry->nextFunc);
	(this->*(entry->nextFunc))(entry, ev);
}



uint32_t DirectoryController::node_id(const std::string &name){
	uint32_t id;
	std::map<std::string, uint32_t>::iterator i = node_lookup.find(name);
	if(node_lookup.end() == i){
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
    Addr entryAddr       = 0; /*  Offset into our buffer? */
    MemEvent *me         = new MemEvent(this, entryAddr, entryAddr, GetS, cacheLineSize);
    me->setSize(entrySize);
    memReqs[me->getID()] = entry->baseAddr;
    dirEntryMiss[me->getID()] = entry->baseAddr;
    entry->nextCommand   = MemEvent::commandResponse(entry->activeReq->getCmd());
    entry->waitingOn     = "memory";
    entry->lastRequest   = me->getID();
    memLink->send(me);
    ++dirEntryReads;
    dbg.debug(_L10_, "Requesting Entry from memory for 0x%"PRIx64"(%"PRIu64", %d); next cmd: %s\n", entry->baseAddr, me->getID().first, me->getID().second, CommandString[entry->nextCommand]);
}



void DirectoryController::requestDataFromMemory(DirEntry *entry){
    Addr localAddr       = convertAddressToLocalAddress(entry->activeReq->getAddr());
    Addr localBaseAddr   = convertAddressToLocalAddress(entry->activeReq->getBaseAddr());
    MemEvent *ev         = new MemEvent(this, localAddr, localBaseAddr, entry->activeReq->getCmd(), cacheLineSize);
    memReqs[ev->getID()] = entry->baseAddr;
    
    entry->nextCommand   = MemEvent::commandResponse(entry->activeReq->getCmd());
    entry->waitingOn     = "memory";
    entry->lastRequest   = ev->getID();
    memLink->send(ev);
    ++dataReads;
    dbg.debug(_L10_, "Requesting data from memory.  Cmd = %s, BaseAddr = x%"PRIx64", Size = %u\n", CommandString[ev->getCmd()], ev->getBaseAddr(), ev->getSize());
}



void DirectoryController::updateCacheEntry(DirEntry *entry){
    if(0 == entryCacheMaxSize){
        sendEntryToMemory(entry);
    } else {
        /* Find if we're in the cache */
        if(entry->cacheIter != entryCache.end()){
            entryCache.erase(entry->cacheIter);
            --entryCacheSize;
            entry->cacheIter = entryCache.end();
        }

        /* Find out if we're no longer cached, and just remove */
        if((!entry->activeReq) &&(0 == entry->countRefs())){
            dbg.debug(_L10_, "Entry for 0x%"PRIx64" has no referenes - purging\n", entry->baseAddr);
            directory.erase(entry->baseAddr);
            delete entry;
            return;
        } else {

            entryCache.push_front(entry);
            entry->cacheIter = entryCache.begin();
            ++entryCacheSize;

            while(entryCacheSize > entryCacheMaxSize){
                DirEntry *oldEntry = entryCache.back();
                // If the oldest entry is still in progress, everything is in progress
                if(oldEntry->inProgress()) break;

                dbg.debug(_L10_, "entryCache too large.  Evicting entry for 0x%"PRIx64"\n", oldEntry->baseAddr);
                entryCache.pop_back();
                --entryCacheSize;
                oldEntry->cacheIter = entryCache.end();
                oldEntry->inController = false;
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
    Addr entryAddr = 0; /* Offset into our buffer? This won't work for a mem that doesn't have addr 0 in its range */
    MemEvent *me   = new MemEvent(this, entryAddr, entryAddr, PutM, cacheLineSize); //PutM?
    me->setSize(entrySize);
    //memReqs[me->getID()] = entry->baseAddr; // ...Does this ever get removed????
    memLink->send(me);
    ++dirEntryWrites;
}



MemEvent::id_type DirectoryController::writebackData(MemEvent *data_event){
    Addr localBaseAddr = convertAddressToLocalAddress(data_event->getBaseAddr());
    MemEvent *ev       = new MemEvent(this, localBaseAddr, localBaseAddr, PutM, cacheLineSize);
    
    assert(data_event->getPayload().size() == cacheLineSize);
    ev->setSize(data_event->getPayload().size());
    ev->setPayload(data_event->getPayload());

    memLink->send(ev);
    ++dataWrites;

    dbg.debug(_L10_, "Writing back data. Cmd = %s, BaseAddr = 0x%"PRIx64", Size = %u\n", CommandString[ev->getCmd()], ev->getBaseAddr(), ev->getSize());
    return ev->getID();
}



void DirectoryController::resetEntry(DirEntry *entry){
    if(entry->activeReq){
        ++numReqsProcessed;
        totalReqProcessTime += (getCurrentSimTimeNano() - entry->activeReq->getDeliveryTime());

        if (mshr->elementIsHit(entry->activeReq->getBaseAddr(),entry->activeReq)) {
            mshr->removeElement(entry->activeReq->getBaseAddr(),entry->activeReq);
            dbg.debug(_L10_, "Removing request from mshr. Cmd = %s, BaseAddr = 0x%"PRIx64", Addr = 0x%"PRIx64", MSHR size: %d\n", CommandString[entry->activeReq->getCmd()], entry->activeReq->getBaseAddr(), entry->activeReq->getAddr(), mshr->getSize());
            if (mshr->isHit(entry->activeReq->getBaseAddr())) {
                MemEvent * ev = mshr->lookupFront(entry->baseAddr);
                dbg.debug(_L10_, "Reactivating event. Cmd = %s, BaseAddr = 0x%"PRIx64", Addr = 0x%"PRIx64", MSHR size: %d\n", CommandString[ev->getCmd()], ev->getBaseAddr(), ev->getAddr(), mshr->getSize());
                workQueue.push_back(ev);
            }
        }
        delete entry->activeReq;
    }
    entry->setToSteadyState();
}



void DirectoryController::sendResponse(MemEvent *ev){
    dbg.debug(_L10_, "Sending Event. Cmd = %s, BaseAddr = 0x%"PRIx64", Dst = %s, Size = %u\n", CommandString[ev->getCmd()], ev->getBaseAddr(), ev->getDst().c_str(), ev->getSize());
    network->send(ev);
}



bool DirectoryController::isRequestAddressValid(MemEvent *ev){
    Addr addr = ev->getBaseAddr();

    if(0 == interleaveSize){
        return(addr >= addrRangeStart && addr < addrRangeEnd);
    } else {
        if(addr < addrRangeStart) return false;
        if(addr >= addrRangeEnd) return false;

        addr        = addr - addrRangeStart;
        Addr offset = addr % interleaveStep;
        
        if(offset >= interleaveSize) return false;
        return true;
    }

}



Addr DirectoryController::convertAddressToLocalAddress(Addr addr){
    Addr res = 0;
    if(0 == interleaveSize){
        res = lookupBaseAddr + addr - addrRangeStart;
    }
    else {
        addr        = addr - addrRangeStart;
        Addr step   = addr / interleaveStep;
        Addr offset = addr % interleaveStep;
        res         = lookupBaseAddr +(step * interleaveSize) + offset;
    }
    dbg.debug(_L10_, "Converted physical address 0x%"PRIx64" to ACTUAL memory address 0x%"PRIx64"\n", addr, res);
    return res;
}



static char dirEntStatus[1024] = {0};
const char* DirectoryController::printDirectoryEntryStatus(Addr baseAddr){
    DirEntry *entry = getDirEntry(baseAddr);
    if(!entry){
        sprintf(dirEntStatus, "[Not Created]");
    } else {
        uint32_t refs = entry->countRefs();

        if(0 == refs) sprintf(dirEntStatus, "[Noncacheable]");
        else if(entry->isDirty()){
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
    while(MemEvent *ev = network->recvInitData()){
        dbg.debug(_L10_, "Found Init Info for address 0x%"PRIx64"\n", ev->getAddr());
        if(isRequestAddressValid(ev)){
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
    for(std::vector<MemNIC::ComponentInfo>::const_iterator i = ci.begin() ; i != ci.end() ; ++i){
        dbg.debug(_L10_, "DC found peer %d(%s) of type %d.\n", i->network_addr, i->name.c_str(), i->type);
        if(MemNIC::TypeCache == i->type){
            numTargets++;
            if(blocksize)   assert(blocksize == i->typeInfo.cache.blocksize);
            else            blocksize = i->typeInfo.cache.blocksize;
        }
    }
    if(0 == numTargets) _abort(DirectoryController, "Directory Controller %s unable to see any caches.\n", getName().c_str());

    network->clearPeerInfo();
    entrySize = (numTargets+1)/8 +1;
}

