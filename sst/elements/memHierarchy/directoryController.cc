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
    if(debugLevel < 0 || debugLevel > 10)     _abort(DirectoryController, "Debugging level must be between 0 and 10. \n");
    
    dbg.init("", debugLevel, 0, (Output::output_location_t)params.find_integer("debug", 0));
    printStatsLoc = (Output::output_location_t)params.find_integer("statistics", 0);

    targetCount = 0;

    registerTimeBase("1 ns", true);


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
    if (mshrSize < 1) dbg.fatal(CALL_INFO, -1, "Invalid param: mshr_num_entries - must be at least 1 or else -1 to indicate a very large MSHR\n");
    mshr            = new   MSHR(&dbg, mshrSize); 
    
    int directMem = params.find_integer("direct_mem_link",1);
    directMemoryLink = (directMem == 1) ? true : false;

    if(0 == addrRangeEnd) addrRangeEnd = (uint64_t)-1;
    numTargets = 0;
	
    /* Check parameter validity */
    assert(protocol == "MESI" || protocol == "mesi" || protocol == "MSI" || protocol == "msi");
    if (protocol == "mesi") protocol = "MESI";
    if (protocol == "msi") protocol = "MSI";

    /* Set up links/network to cache & memory */
    if (directMemoryLink) {
        memLink = configureLink("memory", "1 ns", new Event::Handler<DirectoryController>(this, &DirectoryController::handleMemoryResponse));
        assert(memLink);

        MemNIC::ComponentInfo myInfo;
        myInfo.link_port                        = "network";
        myInfo.link_bandwidth                   = net_bw;
        myInfo.num_vcs                          = params.find_integer("network_num_vc", 3);
        myInfo.name                             = getName();
        myInfo.network_addr                     = addr;
        myInfo.type                             = MemNIC::TypeDirectoryCtrl;
        network = new MemNIC(this, myInfo, new Event::Handler<DirectoryController>(this, &DirectoryController::handlePacket));

        MemNIC::ComponentTypeInfo typeInfo;
        typeInfo.rangeStart     = addrRangeStart;
        typeInfo.rangeEnd       = addrRangeEnd;
        typeInfo.interleaveSize = interleaveSize;
        typeInfo.interleaveStep = interleaveStep;
        typeInfo.blocksize      = 0;
        network->addTypeInfo(typeInfo);
    } else {
        memoryName  = params.find_string("net_memory_name", "");
        if (memoryName == "") dbg.fatal(CALL_INFO,-1,"Param not specified: net_memory_name - name of the memory owned by this directory controller\n");

        MemNIC::ComponentInfo myInfo;
        myInfo.link_port                        = "network";
        myInfo.link_bandwidth                   = net_bw;
        myInfo.num_vcs                          = params.find_integer("network_num_vc", 3);
        myInfo.name                             = getName();
        myInfo.network_addr                     = addr;
        myInfo.type                             = MemNIC::TypeNetworkDirectory;
        network = new MemNIC(this, myInfo, new Event::Handler<DirectoryController>(this, &DirectoryController::handlePacket));
        
        MemNIC::ComponentTypeInfo typeInfo;
        typeInfo.rangeStart     = addrRangeStart;
        typeInfo.rangeEnd       = addrRangeEnd;
        typeInfo.interleaveSize = interleaveSize;
        typeInfo.interleaveStep = interleaveStep;
        typeInfo.blocksize      = 0;
        network->addTypeInfo(typeInfo);
    }


    registerClock(params.find_string("clock", "1GHz"), new Clock::Handler<DirectoryController>(this, &DirectoryController::clock));


    /*Parameter not needed since cache entries are always stored at address 0.
      Entries always kept in the cache, but memory is accessed to get performance metrics. */

    // Clear statistics counters
    numReqsProcessed    = 0;
    totalReqProcessTime = 0;
    numCacheHits        = 0;
    mshrHits            = 0;
    GetXReqReceived     = 0;
    GetSExReqReceived   = 0;
    GetSReqReceived     = 0;
    PutMReqReceived     = 0;
    PutEReqReceived     = 0;
    PutSReqReceived     = 0;
    NACKReceived        = 0;
    FetchRespReceived   = 0;
    FetchRespXReceived  = 0;
    PutMRespReceived    = 0;
    PutERespReceived    = 0;
    PutSRespReceived    = 0;
    dataReads           = 0;
    dataWrites          = 0;
    dirEntryReads       = 0;
    dirEntryWrites      = 0;
    NACKSent            = 0;
    InvSent             = 0; 
    FetchInvSent        = 0;
    FetchInvXSent       = 0;
    GetSRespSent        = 0;
    GetXRespSent        = 0;


}



DirectoryController::~DirectoryController(){
    for(std::map<Addr, DirEntry*>::iterator i = directory.begin(); i != directory.end() ; ++i){
        delete i->second;
    }
    directory.clear();
    
    while(workQueue.size()){
        MemEvent *front = workQueue.front();
        delete front;
        workQueue.pop_front();
    }
}



void DirectoryController::handlePacket(SST::Event *event){
    MemEvent *ev = static_cast<MemEvent*>(event);
    ev->setDeliveryTime(getCurrentSimTimeNano());
     
    if (ev->getCmd() == GetSResp || ev->getCmd() == GetXResp) handleMemoryResponse(event);
    else if (ev->queryFlag(MemEvent::F_NONCACHEABLE)) {
        dbg.debug(_L5_,"Forwarding noncacheable event to memory: Cmd = %s, BsAddr = 0x%" PRIx64 ", Addr = 0x%" PRIx64 "\n",CommandString[ev->getCmd()],ev->getBaseAddr(),ev->getAddr());
        profileRequestRecv(ev,NULL);
        Addr localAddr       = convertAddressToLocalAddress(ev->getAddr());
        Addr localBaseAddr   = convertAddressToLocalAddress(ev->getBaseAddr());
        noncacheMemReqs[ev->getID()] = make_pair<Addr,Addr>(ev->getBaseAddr(),ev->getAddr());
        ev->setBaseAddr(localBaseAddr);
        ev->setAddr(localAddr);
        profileRequestSent(ev);
        if (directMemoryLink) {
            memLink->send(ev);
        } else {
            ev->setDst(memoryName);
            network->send(ev);
        }
    } else {
        workQueue.push_back(ev);
    }
}

/**
 * Profile requests sent to directory controller
 * Could wrap this in "if printStatLoc" so that it's ignored if we're not printing stats
 */
inline void DirectoryController::profileRequestRecv(MemEvent * event, DirEntry * entry) {
    Command cmd = event->getCmd();
    switch (cmd) {
    case GetX:
        GetXReqReceived++;
        break;
    case GetSEx:
        GetSExReqReceived++;
        break;
    case GetS:
        GetSReqReceived++;
        break;
    case PutM:
        PutMReqReceived++;
        break;
    case PutE:
        PutEReqReceived++;
        break;
    case PutS:
        PutSReqReceived++;
        break;
    default:
        break;

    }
    if (!entry || entry->inController) {
        ++numCacheHits;
    }
}
/** 
 * Profile requests sent from directory controller to memory or other caches
 * Could wrap this in "if printStatLoc" so that it's ignored if we're not printing stats
 */
inline void DirectoryController::profileRequestSent(MemEvent * event) {
    Command cmd = event->getCmd();
    switch(cmd) {
    case PutM:
        if (event->getAddr() == 0) dirEntryWrites++;
        else dataWrites++;
        break;
    case GetX:
        if (event->queryFlag(MemEvent::F_NONCACHEABLE)) {
            dataWrites++;
            break;
        }
    case GetSEx:
    case GetS:
        if (event->getAddr() == 0) dirEntryReads++;
        else dataReads++;
        break;
    case FetchInv:
        FetchInvSent++;
        break;
    case FetchInvX:
        FetchInvXSent++;
        break;
    case Inv:
        InvSent++;
        break;
    default:
        break;
        
    }
}

/** 
 * Profile responses sent from directory controller to caches
 * Could wrap this in "if printStatLoc" so that it's ignored if we're not printing stats
 */
inline void DirectoryController::profileResponseSent(MemEvent * event) {
    Command cmd = event->getCmd();
    switch(cmd) {
    case GetSResp:
        GetSRespSent++;
        break;
    case GetXResp:
        GetXRespSent++;
        break;
    case NACK:
        NACKSent++;
        break;
    default:
        break;
    }
}

/** 
 * Profile responses received by directory controller from caches
 * Could wrap this in "if printStatLoc" so that it's ignored if we're not printing stats
 */
inline void DirectoryController::profileResponseRecv(MemEvent * event) {
    Command cmd = event->getCmd();
    switch(cmd) {
    case FetchResp:
        FetchRespReceived++;
        break;
    case FetchXResp:
        FetchRespXReceived++;
        break;
    case PutM:
        PutMRespReceived++;
        break;
    case PutE:
        PutERespReceived++;
        break;
    case PutS:
        PutSRespReceived++;
        break;
    case NACK:
        NACKReceived++;
        break;
    default:
        break;
    }
}   

bool DirectoryController::clock(SST::Cycle_t cycle){
    network->clock();

    if(!workQueue.empty()){
        MemEvent *event = workQueue.front();
	processPacket(event);
        workQueue.erase(workQueue.begin());
    }

	return false;
}

bool DirectoryController::processPacket(MemEvent *ev){
    dbg.debug(_L3_, "\n\n----------------------------------------------------------------------------------------\n");
    dbg.debug(_L3_, "EVENT: %s, Received: Cmd = %s, BsAddr = 0x%" PRIx64 ", Src = %s, id (%" PRIu64 ",%d), Time = %" PRIu64 "\n",
            getName().c_str(),  CommandString[ev->getCmd()], ev->getBaseAddr(), ev->getSrc().c_str(), 
            ev->getID().first, ev->getID().second, getCurrentSimTimeNano());
    assert(isRequestAddressValid(ev));
    Command cmd = ev->getCmd();
    
    if(NACK == cmd){
        MemEvent* origEvent = ev->getNACKedEvent();
        profileResponseRecv(ev);
        processIncomingNACK(origEvent,ev);
        delete ev;
        return true;
    }
    
    uint32_t requesting_node;
    pair<bool, bool> ret = make_pair<bool, bool>(false, false);
    DirEntry *entry = getDirEntry(ev->getBaseAddr());
    
    if(entry && entry->waitingAcks > 0 && PutS == ev->getCmd()){
        profileResponseRecv(ev);
        handlePutS(ev);
        delete ev;
        return true;
    }
    
    if(entry && entry->inProgress()) ret = handleEntryInProgress(ev, entry, cmd);
    if(ret.first == true && ret.second == false) { // new request which conflicts with current request
        if (!(mshr->elementIsHit(ev->getBaseAddr(),ev))) {
            mshrHits++;
            bool inserted = mshr->insert(ev->getBaseAddr(),ev);
            dbg.debug(_L8_, "Inserting conflicting request in mshr. Cmd = %s, BaseAddr = 0x%" PRIx64 ", Addr = 0x%" PRIx64 ", MSHR size: %d\n", CommandString[ev->getCmd()], ev->getBaseAddr(), ev->getAddr(), mshr->getSize());
            if (!inserted) {
                dbg.debug(_L8_, "MSHR is full. NACKing request\n");
                mshrNACKRequest(ev);
                return true;
            }
        } else {
            dbg.debug(_L8_, "Conflicting request already in mshr. Cmd = %s, BaseAddr = 0x%" PRIx64 ", Addr = 0x%" PRIx64 ", MSHR size: %d\n", CommandString[ev->getCmd()], ev->getBaseAddr(), ev->getAddr(), mshr->getSize());
        }
    }
    if(ret.first == true) return ret.second;


    /* New Request */
    profileRequestRecv(ev,entry);     // profile request
    switch(cmd){
        case PutS:
            assert(entry);
            requesting_node = node_name_to_id(ev->getSrc());
            entry->removeSharer(requesting_node);
            if(entry->countRefs() == 0 && !entry->inProgress()) resetEntry(entry);
            if(mshr->elementIsHit(ev->getBaseAddr(),ev)) mshr->removeElement(ev->getBaseAddr(),ev);
            delete ev;
            break;
            
        case PutM:
        case PutE:
            assert(entry);
            
            if(entry->inController){
                entry->activeReq = ev;
                handlePutM(entry, ev);
            } else {
                if (!(mshr->elementIsHit(ev->getBaseAddr(),ev))) {
                    mshrHits++;
                    bool inserted = mshr->insert(ev->getBaseAddr(),ev);
                    dbg.debug(_L8_, "Inserting request in mshr. Cmd = %s, BaseAddr = 0x%" PRIx64 ", Addr = 0x%" PRIx64 ", MSHR size: %d\n", CommandString[ev->getCmd()], ev->getBaseAddr(), ev->getAddr(), mshr->getSize());
                    if (!inserted) {
                        dbg.debug(_L8_, "MSHR is full. NACKing request\n");
                        mshrNACKRequest(ev);
                        return true;
                    }
                    entry->activeReq = ev;
                }
                dbg.debug(_L6_, "Entry %" PRIx64 " not in cache.  Requesting from memory.\n", entry->baseAddr);
                entry->nextFunc = &DirectoryController::handlePutM;
                requestDirEntryFromMemory(entry);
            }
            break;
        
        case GetX:
        case GetSEx:
        case GetS:
           
            /* Create directory entry if this is a new block */
            if(!entry){
                entry = createDirEntry(ev->getBaseAddr(), ev->getAddr(), ev->getSize());
                entry->inController = true;
            }

            /* Insert event in mshrs */
            if (!(mshr->elementIsHit(ev->getBaseAddr(),ev))) {
                mshrHits++;
                bool inserted = mshr->insert(ev->getBaseAddr(),ev);
                dbg.debug(_L8_, "Inserting request in mshr. Cmd = %s, BaseAddr = 0x%" PRIx64 ", Addr = 0x%" PRIx64 ", MSHR size: %d\n", CommandString[ev->getCmd()], ev->getBaseAddr(), ev->getAddr(), mshr->getSize());
                if (!inserted) {
                    dbg.debug(_L8_, "MSHR is full. NACKing request\n");
                    mshrNACKRequest(ev);
                    return true;
                }
            }
            
            entry->activeReq = ev; // should also be the top request in the mshrs for this addr

            if(entry->inController){
                handleDataRequest(entry, ev);
            }
            else{
                dbg.debug(_L8_, "Entry %" PRIx64 " not in cache.  Requesting from memory.\n", entry->baseAddr);
                entry->nextFunc = &DirectoryController::handleDataRequest;
                requestDirEntryFromMemory(entry);
            }
            break;
        default:
            /* Ignore unexpected */
            dbg.fatal(CALL_INFO, -1, "Command not recognized: %s\n", CommandString[cmd]);
            break;
    }

    return true;
}



void DirectoryController::handleMemoryResponse(SST::Event *event){
    MemEvent *ev = static_cast<MemEvent*>(event);
    dbg.debug(_L3_, "\n\n----------------------------------------------------------------------------------------\n");
    if (ev->queryFlag(MemEvent::F_NONCACHEABLE)) {
        if (noncacheMemReqs.find(ev->getResponseToID()) != noncacheMemReqs.end()) {
            Addr globalBaseAddr = noncacheMemReqs[ev->getID()].first;
            Addr globalAddr = noncacheMemReqs[ev->getID()].second;
            dbg.debug(_L3_, "EVENT: %s, Received: MemResp: Cmd = %s, BaseAddr = 0x%" PRIx64 ", Size = %u, Time = %" PRIu64 "\n", getName().c_str(), CommandString[ev->getCmd()], globalBaseAddr, ev->getSize(),getCurrentSimTimeNano());
            ev->setBaseAddr(globalBaseAddr);
            ev->setAddr(globalAddr);
            noncacheMemReqs.erase(ev->getResponseToID());
            profileResponseSent(ev);
            network->send(ev);
            return;
        }
        dbg.fatal(CALL_INFO, -1, "Received unexpected noncacheable response from memory for memory addr %" PRIx64 "\n",ev->getAddr());
    }


    if (ev->getBaseAddr() == 0 && dirEntryMiss.find(ev->getResponseToID()) != dirEntryMiss.end()) {    // directory entry miss
        dbg.debug(_L3_, "EVENT: %s, Received:Cmd = %s, BaseAddr = 0x%" PRIx64 ", Src: Memory, Size = %u, Time = %" PRIu64 "\n", getName().c_str(), CommandString[ev->getCmd()], ev->getAddr(), ev->getSize(),getCurrentSimTimeNano());
        dirEntryMiss.erase(ev->getResponseToID());
    }    
    
    if(memReqs.find(ev->getResponseToID()) != memReqs.end()){
        Addr targetBlock = memReqs[ev->getResponseToID()];
        dbg.debug(_L3_, "RECV: %s, MemResp: Cmd = %s, BaseAddr = 0x%" PRIx64 ", Size = %u, Time = %" PRIu64 "\n", getName().c_str(), CommandString[ev->getCmd()], targetBlock, ev->getSize(),getCurrentSimTimeNano());
        memReqs.erase(ev->getResponseToID());
        if(GetSResp == ev->getCmd() || GetXResp == ev->getCmd()){   // Lookup complete, perform our work
            DirEntry *entry = getDirEntry(targetBlock);
            assert(entry);
            entry->inController = true;
            advanceEntry(entry, ev);
        }
        else  dbg.fatal(CALL_INFO, -1, "Received unexpected response from memory - response type not recognized: %s\n",CommandString[ev->getCmd()]);
    } else {
        dbg.fatal(CALL_INFO, -1, "Received unexpected response from memory - matching request not found for memory address %" PRIx64 "\n",ev->getAddr());
    }

    delete ev;
}

void DirectoryController::mshrNACKRequest(MemEvent* ev) {
    MemEvent * nackEv = ev->makeNACKResponse(this,ev);
    profileResponseSent(nackEv);
    sendEventToCaches(nackEv);
}


void DirectoryController::processIncomingNACK(MemEvent* _origReqEvent, MemEvent* _nackEvent){
    DirEntry *entry = getDirEntry(_origReqEvent->getBaseAddr());
    dbg.debug(_L5_, "Orig resp ID = (%" PRIu64 ",%d), Nack resp ID = (%" PRIu64 ",%d), last req ID = (%" PRIu64 ",%d)\n", 
	_origReqEvent->getResponseToID().first, _origReqEvent->getResponseToID().second, _nackEvent->getResponseToID().first, 
	_nackEvent->getResponseToID().second, entry->lastRequest.first, entry->lastRequest.second);
    
    /* Retry request if it has not already been handled */
    if ((_nackEvent->getResponseToID() == entry->lastRequest) || _origReqEvent->getCmd() == Inv) {
	/* Re-send request */
	sendEventToCaches(_origReqEvent);
	dbg.debug(_L5_,"Orig Cmd NACKed, retry = %s \n", CommandString[_origReqEvent->getCmd()]);
    } else {
	dbg.debug(_L5_,"Orig Cmd NACKed, no retry = %s \n", CommandString[_origReqEvent->getCmd()]);
    }
}


/*
 *  Return value indicates (successful, erase packet from queue)
 */
pair<bool, bool> DirectoryController::handleEntryInProgress(MemEvent *ev, DirEntry *entry, Command cmd){
    dbg.debug(_L5_, "Entry found and in progress\n");
        if((entry->nextCommand == cmd || 
            (entry->nextCommand == FetchResp && cmd == PutM) ||
            (entry->nextCommand == FetchXResp && cmd == PutM) ||
            (entry->nextCommand == GetSResp && cmd == PutS)) &&
            ("N/A" == entry->waitingOnType || entry->waitingOn == ev->getSrc())){
            
            dbg.debug(_L4_, "Incoming command matches for 0x%" PRIx64 " in progress.\n", entry->baseAddr);
            profileResponseRecv(ev);
            if(ev->getResponseToID() != entry->lastRequest){
                dbg.debug(_L4_, "This isn't a response to our request, but it fulfills the need.  Placing(%" PRIu64 ", %d) into list of ignorable responses.\n", 
                        entry->lastRequest.first, entry->lastRequest.second);
            }
            advanceEntry(entry, ev);
            delete ev;
            return make_pair<bool, bool>(true, true);
        
        } else if ((entry->nextCommand == FetchResp || entry->nextCommand == FetchXResp) && cmd == PutE) {    // exclusive replacement raced with a fetch, re-direct to memory

            dbg.debug(_L4_, "Entry %" PRIx64 " request raced with replacement. Handling as a miss to memory.\n", entry->baseAddr);
            profileResponseRecv(ev);
            assert(entry->findOwner() == node_name_to_id(ev->getSrc()));
            int id = node_name_to_id(ev->getSrc());
            entry->clearDirty(id);
            entry->nextFunc = &DirectoryController::handleDataRequest;
            advanceEntry(entry, ev);
            delete ev;
            return make_pair<bool, bool>(true,true);
        } else if(entry->waitingOnType == "memory" && cmd == PutS && entry->nextCommand == GetSResp) {
	    dbg.debug(_L4_, "Replacement during a GetS for a different cache, handling replacement immediately.\n");
	    return make_pair<bool,bool>(false,false); // not a conflicting request
        } else {
            dbg.debug(_L4_, "Incoming command [%s,%s] doesn't match for 0x%" PRIx64 " [%s,%s] in progress.\n", 
                    CommandString[ev->getCmd()], ev->getSrc().c_str(), entry->baseAddr, CommandString[entry->nextCommand], entry->waitingOn.c_str());
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
        out.output("\t\t(%" PRIu64 ", %d)\n", (*i)->getID().first, (*i)->getID().second);
    }
    out.output("\tRequests in Progress:\n");
    for(std::map<Addr, DirEntry*>::iterator i = directory.begin() ; i != directory.end() ; ++i){
        if(i->second->inProgress())
            out.output("\t\t0x%" PRIx64 "\t\t(%" PRIu64 ", %d)\n",i->first, i->second->activeReq->getID().first, i->second->activeReq->getID().second);
    }

}



DirectoryController::DirEntry* DirectoryController::getDirEntry(Addr baseAddr){
	std::map<Addr, DirEntry*>::iterator i = directory.find(baseAddr);
	if(directory.end() == i) return NULL;
	return i->second;
}



DirectoryController::DirEntry* DirectoryController::createDirEntry(Addr baseAddr, Addr addr, uint32_t reqSize){
    dbg.debug(_L10_, "Creating Directory Entry for 0x%" PRIx64 "\n", baseAddr);
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
    dbg.debug(_L4_, "Sending Invalidate.  Dst: %s\n", nodeid_to_name[target].c_str());
    profileRequestSent(me);
    network->send(me);
}


/**
 *  Handle data request (Get) to directory controller from caches
 */
void DirectoryController::handleDataRequest(DirEntry* entry, MemEvent *new_ev){
    entry->reqSize     = entry->activeReq->getSize();
    Command cmd        = entry->activeReq->getCmd();
    
    uint32_t requesting_node = node_id(entry->activeReq->getSrc());
    assert(!entry->activeReq->queryFlag(MemEvent::F_NONCACHEABLE)); 
    if(entry->isDirty()){                                       // Must do a fetch
	MemEvent *ev;      
        if (cmd == GetS) { // Downgrade
            ev = new MemEvent(this, entry->activeReq->getAddr(), entry->activeReq->getBaseAddr(), FetchInvX, cacheLineSize);
            entry->nextCommand = FetchXResp;
        } else { // Invalidate
            ev = new MemEvent(this, entry->activeReq->getAddr(), entry->activeReq->getBaseAddr(), FetchInv, cacheLineSize);
            entry->nextCommand = FetchResp;
        }
        std::string &dest = nodeid_to_name[entry->findOwner()];
        ev->setDst(dest);

	entry->nextFunc    = &DirectoryController::finishFetch;
        entry->waitingOn   = dest;
        entry->waitingOnType = dest;
        entry->lastRequest = ev->getID();
        
        profileRequestSent(ev);
	sendEventToCaches(ev);
    } else if(cmd == GetX || cmd == GetSEx){
	assert(0 == entry->waitingAcks);
	for(uint32_t i = 0 ; i < numTargets ; i++){             // Must send invalidates
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
             entry->waitingOnType = "N/A";
             entry->lastRequest = DirEntry::NO_LAST_REQUEST;
             dbg.debug(_L4_, "Sending Invalidates to fulfill request for exclusive, BsAddr = %" PRIx64 ".\n", entry->baseAddr);
        } else getExclusiveDataForRequest(entry, NULL);

    } else {                                                    //Handle GetS requests
	entry->addSharer(requesting_node);
	entry->nextFunc = &DirectoryController::sendResponse;
	requestDataFromMemory(entry);
    }
}



void DirectoryController::finishFetch(DirEntry* entry, MemEvent *new_ev){
    dbg.debug(_L4_, "Finishing Fetch. Writing data to memory. \n");
    int wb_id         = node_name_to_id(new_ev->getSrc());
    int req_id        = node_name_to_id(entry->activeReq->getSrc());
    Command cmd       = entry->activeReq->getCmd();
    
    entry->clearDirty(wb_id);
    
    if(cmd == GetX || cmd == GetSEx) entry->setDirty(req_id);
    else {
        entry->addSharer(req_id);
        if (new_ev->getCmd() == FetchXResp) entry->addSharer(wb_id);    // responder kept a shared copy
    }

    MemEvent * ev;
    if (cmd == GetS && protocol == "MESI" && entry->countRefs() == 1) {
        ev = entry->activeReq->makeResponse(E);
        entry->clearSharers();
        entry->setDirty(req_id);
    } else {
        ev = entry->activeReq->makeResponse();
    }
    ev->setPayload(new_ev->getPayload());
    profileResponseSent(ev);
    sendEventToCaches(ev);
    writebackData(new_ev);
    updateEntryToMemory(entry);
}



void DirectoryController::sendResponse(DirEntry* entry, MemEvent *new_ev){
    MemEvent *ev;
    uint32_t requesting_node = node_name_to_id(entry->activeReq->getSrc());
    
    if (entry->activeReq->getCmd() == GetS) {                   // Handle GetS requests according to coherence protocol
        if (protocol == "MESI" && entry->countRefs() == 1 && entry->sharers[requesting_node]) {    // Return block in E state if this is the only requestor
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
    profileResponseSent(ev);
    sendEventToCaches(ev);
    
    dbg.debug(_L4_, "Sending requested data for 0x%" PRIx64 " to %s, granted state = %s\n", entry->baseAddr, ev->getDst().c_str(), BccLineString[ev->getGrantedState()]);

    updateEntryToMemory(entry);

}


void DirectoryController::getExclusiveDataForRequest(DirEntry* entry, MemEvent *new_ev){
    uint32_t requesting_node = node_name_to_id(entry->activeReq->getSrc());
	
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
        dbg.debug(_L4_, "Alert! dropping PutM\n");
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
    Addr entryAddr       = 0; /* Dummy addr reused for dir cache misses */
    MemEvent *me         = new MemEvent(this, entryAddr, entryAddr, GetS, cacheLineSize);
    me->setSize(entrySize);
    memReqs[me->getID()] = entry->baseAddr;
    dirEntryMiss[me->getID()] = entry->baseAddr;
    entry->nextCommand   = MemEvent::commandResponse(entry->activeReq->getCmd());
    entry->waitingOn     = "memory";
    entry->waitingOnType = "memory";
    entry->lastRequest   = me->getID();
    profileRequestSent(me);
    if (directMemoryLink) {
        memLink->send(me);
    } else {
        entry->waitingOn = memoryName;
        me->setDst(memoryName);
        network->send(me);
    }
    dbg.debug(_L10_, "Requesting Entry from memory for 0x%" PRIx64 "(%" PRIu64 ", %d); next cmd: %s\n", entry->baseAddr, me->getID().first, me->getID().second, CommandString[entry->nextCommand]);
}



void DirectoryController::requestDataFromMemory(DirEntry *entry){
    Addr localAddr       = convertAddressToLocalAddress(entry->activeReq->getAddr());
    Addr localBaseAddr   = convertAddressToLocalAddress(entry->activeReq->getBaseAddr());
    MemEvent *ev         = new MemEvent(this, localAddr, localBaseAddr, entry->activeReq->getCmd(), cacheLineSize);
    memReqs[ev->getID()] = entry->baseAddr;
    entry->nextCommand   = MemEvent::commandResponse(entry->activeReq->getCmd());
    entry->waitingOn     = "memory";
    entry->waitingOnType = "memory";
    entry->lastRequest   = ev->getID();
    profileRequestSent(ev);
    if (directMemoryLink) {
        memLink->send(ev);
    } else {
        entry->waitingOn = memoryName;
        entry->waitingOnType = "memory";
        ev->setDst(memoryName);
        network->send(ev);
    }
    dbg.debug(_L5_, "Requesting data from memory.  Cmd = %s, BaseAddr = x%" PRIx64 ", Size = %u, noncacheable = %s\n", CommandString[ev->getCmd()], ev->getBaseAddr(), ev->getSize(), ev->queryFlag(MemEvent::F_NONCACHEABLE) ? "true" : "false");
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
        if((!entry->activeReq) && (0 == entry->countRefs())){
            dbg.debug(_L10_, "Entry for 0x%" PRIx64 " has no references - purging\n", entry->baseAddr);
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

                dbg.debug(_L10_, "entryCache too large.  Evicting entry for 0x%" PRIx64 "\n", oldEntry->baseAddr);
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
    Addr entryAddr = 0; // Always use local address 0 for directory entries
    MemEvent *me   = new MemEvent(this, entryAddr, entryAddr, PutE, cacheLineSize); // MemController discards PutE's without writeback so this is safe
    me->setSize(entrySize);
    profileRequestSent(me);
    if (directMemoryLink) {
        memLink->send(me);
    } else {
        me->setDst(memoryName);
        network->send(me);
    }
}



MemEvent::id_type DirectoryController::writebackData(MemEvent *data_event){
    Addr localBaseAddr = convertAddressToLocalAddress(data_event->getBaseAddr());
    MemEvent *ev       = new MemEvent(this, localBaseAddr, localBaseAddr, PutM, cacheLineSize);
    
    assert(data_event->getPayload().size() == cacheLineSize);
    ev->setSize(data_event->getPayload().size());
    ev->setPayload(data_event->getPayload());
    profileRequestSent(ev);
    if (directMemoryLink) {
        memLink->send(ev);
    } else {
        ev->setDst(memoryName);
        network->send(ev);
    }
    
    dbg.debug(_L5_, "Writing back data. Cmd = %s, BaseAddr = 0x%" PRIx64 ", Size = %u\n", CommandString[ev->getCmd()], ev->getBaseAddr(), ev->getSize());
    return ev->getID();
}



void DirectoryController::resetEntry(DirEntry *entry){
    if(entry->activeReq){
        ++numReqsProcessed;
        totalReqProcessTime += (getCurrentSimTimeNano() - entry->activeReq->getDeliveryTime());

        if (mshr->elementIsHit(entry->activeReq->getBaseAddr(),entry->activeReq)) {
            mshr->removeElement(entry->activeReq->getBaseAddr(),entry->activeReq);
            dbg.debug(_L10_, "Removing request from mshr. Cmd = %s, BaseAddr = 0x%" PRIx64 ", Addr = 0x%" PRIx64 ", MSHR size: %d\n", CommandString[entry->activeReq->getCmd()], entry->activeReq->getBaseAddr(), entry->activeReq->getAddr(), mshr->getSize());
            if (mshr->isHit(entry->activeReq->getBaseAddr())) {
	    vector<mshrType> replayEntries = mshr->removeAll(entry->activeReq->getBaseAddr());
	    for(vector<mshrType>::reverse_iterator it = replayEntries.rbegin(); it != replayEntries.rend(); it++) {
		MemEvent *ev = boost::get<MemEvent*>((*it).elem);
                std::list<MemEvent*>::iterator insert_it = workQueue.begin();
		insert_it++;
		dbg.debug(_L5_, "Reactivating event %p. Cmd = %s, BaseAddr = 0x%" PRIx64 ", Addr = 0x%" PRIx64 ", MSHR size: %d\n", ev, CommandString[ev->getCmd()], ev->getBaseAddr(), ev->getAddr(), mshr->getSize());
	        workQueue.insert(insert_it,ev);
}
}
        }
        delete entry->activeReq;
    }
    entry->setToSteadyState();
}



void DirectoryController::sendEventToCaches(MemEvent *ev){
    dbg.debug(_L3_, "Sending Event. Cmd = %s, BaseAddr = 0x%" PRIx64 ", Dst = %s, Size = %u\n", CommandString[ev->getCmd()], ev->getBaseAddr(), ev->getDst().c_str(), ev->getSize());
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

Addr DirectoryController::convertAddressFromLocalAddress(Addr addr){
    assert(interleaveStep <= interleaveSize); // one-to-one mapping required for this method
    Addr res = 0;
    if(0 == interleaveSize){
        res = addr + addrRangeStart;
    }
    else {
        Addr a 	    = addr;
        Addr step   = a / interleaveSize; 
        Addr offset = a % interleaveSize;
        res = (step * interleaveStep) + offset;
        res = res + addrRangeStart;

    }
    dbg.debug(_L10_, "Converted ACTUAL memory address 0x%" PRIx64 " to physical address 0x%" PRIx64 "\n", addr, res);
    return res;
}

Addr DirectoryController::convertAddressToLocalAddress(Addr addr){
    Addr res = 0;
    if(0 == interleaveSize){
        res = addr - addrRangeStart;
    }
    else {
        Addr a      = addr - addrRangeStart;
        Addr step   = a / interleaveStep;
        Addr offset = a % interleaveStep;
        res         = (step * interleaveSize) + offset;
    }
    dbg.debug(_L10_, "Converted physical address 0x%" PRIx64 " to ACTUAL memory address 0x%" PRIx64 "\n", addr, res);
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

    if (!directMemoryLink && network->initDataReady() && !network->isValidDestination(memoryName)) {
        dbg.fatal(CALL_INFO,-1,"Invalid param: net_memory_name - must name a valid memory component in the system. You specified: %s\n",memoryName.c_str());
    }
    /* Pass data on to memory */
    while(MemEvent *ev = network->recvInitData()){
        dbg.debug(_L10_, "Found Init Info for address 0x%" PRIx64 "\n", ev->getAddr());
        if(isRequestAddressValid(ev)){
            ev->setBaseAddr(convertAddressToLocalAddress(ev->getBaseAddr()));
            ev->setAddr(convertAddressToLocalAddress(ev->getAddr()));
            dbg.debug(_L10_, "Sending Init Data for address 0x%" PRIx64 " to memory\n", ev->getAddr());
            if (directMemoryLink) {
                memLink->sendInitData(ev);
            } else {
                ev->setDst(memoryName);
                network->sendInitData(ev);
            }
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
    out.output("- Total requests received:  %" PRIu64 "\n", numReqsProcessed);
    out.output("- GetS received:            %" PRIu64 "\n", GetSReqReceived);
    out.output("- GetX received:            %" PRIu64 "\n", GetXReqReceived);
    out.output("- GetSEx received:          %" PRIu64 "\n", GetSExReqReceived);
    out.output("- PutM received:            %" PRIu64 "\n", PutMReqReceived);
    out.output("- PutE received:            %" PRIu64 "\n", PutEReqReceived);
    out.output("- PutS received:            %" PRIu64 "\n", PutSReqReceived);
    out.output("- NACK received:            %" PRIu64 "\n", NACKReceived);
    out.output("- FetchResp received:       %" PRIu64 "\n", FetchRespReceived);
    out.output("- FetchXResp received:      %" PRIu64 "\n", FetchRespXReceived);
    out.output("- PutM response received:   %" PRIu64 "\n", PutMRespReceived);
    out.output("- PutE response received:   %" PRIu64 "\n", PutERespReceived);
    out.output("- PutS response received:   %" PRIu64 "\n", PutSRespReceived);
    out.output("- Data reads issued:        %" PRIu64 "\n", dataReads);
    out.output("- Data writes issued:       %" PRIu64 "\n", dataWrites);
    out.output("- Dir entry reads:          %" PRIu64 "\n", dirEntryReads);
    out.output("- Dir entry writes:         %" PRIu64 "\n", dirEntryWrites);
    out.output("- Inv sent:                 %" PRIu64 "\n", InvSent);
    out.output("- FetchInv sent:            %" PRIu64 "\n", FetchInvSent);
    out.output("- FetchInvX sent:           %" PRIu64 "\n", FetchInvXSent);
    out.output("- GetSResp sent:            %" PRIu64 "\n", GetSRespSent);
    out.output("- GetXResp sent:            %" PRIu64 "\n", GetXRespSent);
    out.output("- NACKs sent:               %" PRIu64 "\n", NACKSent);
    out.output("- Avg Req Time:             %" PRIu64 " ns\n", (numReqsProcessed > 0) ? totalReqProcessTime / numReqsProcessed : 0);
    out.output("- Entry Cache Hits:         %" PRIu64 "\n", numCacheHits);
    out.output("- MSHR hits:                %" PRIu64 "\n", mshrHits);
}



void DirectoryController::setup(void){
    network->setup();

    const std::vector<MemNIC::PeerInfo_t> &ci = network->getPeerInfo();
    for(std::vector<MemNIC::PeerInfo_t>::const_iterator i = ci.begin() ; i != ci.end() ; ++i){
        dbg.debug(_L10_, "DC found peer %d(%s) of type %d.\n", i->first.network_addr, i->first.name.c_str(), i->first.type);
        if(MemNIC::TypeCache == i->first.type || MemNIC::TypeNetworkCache == i->first.type){
            numTargets++;
            if(blocksize)   assert(blocksize == i->second.blocksize);
            else            blocksize = i->second.blocksize;
        }
    }
    if(0 == numTargets) dbg.fatal(CALL_INFO,-1,"Directory Controller %s is unable to see any caches\n",getName().c_str());

    network->clearPeerInfo();
    entrySize = (numTargets+1)/8 +1;
}

