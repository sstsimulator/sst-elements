// Copyright 2013-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright(c) 2013-2018, NTESS
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
#include "MESI_Directory.h"


#include <sst/core/params.h>
#include <sst/core/simulation.h>

#include "memNIC.h"

/* Debug macros */
#ifdef __SST_DEBUG_OUTPUT__ /* From sst-core, enable with --enable-debug */
#define is_debug_addr(addr) (DEBUG_ADDR.empty() || DEBUG_ADDR.find(addr) != DEBUG_ADDR.end())
#define is_debug_event(ev) (DEBUG_ADDR.empty() || ev->doDebug(DEBUG_ADDR))
#else
#define is_debug_addr(addr) false
#define is_debug_event(ev) false
#endif

using namespace SST;
using namespace SST::MemHierarchy;
using namespace std;


const MemEvent::id_type MESIDirectory::DirEntry::NO_LAST_REQUEST = std::make_pair((uint64_t)-1, -1);

MESIDirectory::MESIDirectory(ComponentId_t id, Params &params) :
    SubComponent(id) {
    
    int debugLevel = params.find<int>("debug_level", 0);
    dbg.init("", debugLevel, 0, (Output::output_location_t)params.find<int>("debug", 0));
    out.init("", params.find<int>("verbose", 1), 0, Output::STDOUT);

    // Debug address
    std::vector<Addr> addrArr;
    params.find_array<Addr>("debug_addr", addrArr);
    for (std::vector<Addr>::iterator it = addrArr.begin(); it != addrArr.end(); it++) {
        DEBUG_ADDR.insert(*it);
    }

    cacheLineSize = params.find<uint32_t>("cache_line_size", 64);

    entryCacheMaxSize = params.find<uint64_t>("entry_cache_size", 32768);
    entryCacheSize = 0;
    entrySize = 4; //TODO parameterize

    string protstr  = params.find<std::string>("coherence_protocol", "MESI");
    if (protstr == "mesi" || protstr == "MESI") protocol = CoherenceProtocol::MESI;
    else if (protstr == "msi" || protstr == "MSI") protocol = CoherenceProtocol::MSI;
    else dbg.fatal(CALL_INFO, -1, "Invalid param(%s): coherence_protocol - must be 'MESI' or 'MSI'. You specified: %s\n", getName().c_str(), protstr.c_str());


    int mshrSize    = params.find<int>("mshr_num_entries",-1);
    if (mshrSize == -1) mshrSize = HUGE_MSHR;
    if (mshrSize < 1) dbg.fatal(CALL_INFO, -1, "Invalid param(%s): mshr_num_entries - must be at least 1 or else -1 to indicate a very large MSHR\n", getName().c_str());
    mshr                = new MSHR(&dbg, mshrSize, this->getName(), DEBUG_ADDR);

    /* Check parameter validity */

    /* Get latencies */
    accessLatency   = params.find<uint64_t>("access_latency_cycles", 0);
    mshrLatency     = params.find<uint64_t>("mshr_latency_cycles", 0);

    // Timestamp - aka cycle count
    timestamp = 0;

    // Coherence protocol configuration
    waitWBAck = false; // Don't expect WB Acks

    // Register statistics
    stat_replacementRequestLatency  = registerStatistic<uint64_t>("replacement_request_latency");
    stat_getRequestLatency          = registerStatistic<uint64_t>("get_request_latency");
    stat_cacheHits                  = registerStatistic<uint64_t>("directory_cache_hits");
    stat_mshrHits                   = registerStatistic<uint64_t>("mshr_hits");
    stat_GetXReqReceived            = registerStatistic<uint64_t>("requests_received_GetX");
    stat_GetSXReqReceived          = registerStatistic<uint64_t>("requests_received_GetSX");
    stat_GetSReqReceived            = registerStatistic<uint64_t>("requests_received_GetS");
    stat_PutMReqReceived            = registerStatistic<uint64_t>("requests_received_PutM");
    stat_PutEReqReceived            = registerStatistic<uint64_t>("requests_received_PutE");
    stat_PutSReqReceived            = registerStatistic<uint64_t>("requests_received_PutS");
    stat_NACKRespReceived           = registerStatistic<uint64_t>("responses_received_NACK");
    stat_FetchRespReceived          = registerStatistic<uint64_t>("responses_received_FetchResp");
    stat_FetchXRespReceived         = registerStatistic<uint64_t>("responses_received_FetchXResp");
    stat_PutMRespReceived           = registerStatistic<uint64_t>("responses_received_PutM");
    stat_PutERespReceived           = registerStatistic<uint64_t>("responses_received_PutE");
    stat_PutSRespReceived           = registerStatistic<uint64_t>("responses_received_PutS");
    stat_dataReads                  = registerStatistic<uint64_t>("memory_requests_data_read");
    stat_dataWrites                 = registerStatistic<uint64_t>("memory_requests_data_write");
    stat_dirEntryReads              = registerStatistic<uint64_t>("memory_requests_directory_entry_read");
    stat_dirEntryWrites             = registerStatistic<uint64_t>("memory_requests_directory_entry_write");
    stat_InvSent                    = registerStatistic<uint64_t>("requests_sent_Inv");
    stat_FetchInvSent               = registerStatistic<uint64_t>("requests_sent_FetchInv");
    stat_FetchInvXSent              = registerStatistic<uint64_t>("requests_sent_FetchInvX");
    stat_NACKRespSent               = registerStatistic<uint64_t>("responses_sent_NACK");
    stat_GetSRespSent               = registerStatistic<uint64_t>("responses_sent_GetSResp");
    stat_GetXRespSent               = registerStatistic<uint64_t>("responses_sent_GetXResp");
}



MESIDirectory::~MESIDirectory(){
    for(std::unordered_map<Addr, DirEntry*>::iterator i = directory.begin(); i != directory.end() ; ++i){
        delete i->second;
    }
    directory.clear();
}



/**
 * Profile requests sent to directory controller
 */
inline void MESIDirectory::profileRequestRecv(MemEvent * event, DirEntry * entry) {
    Command cmd = event->getCmd();
    switch (cmd) {
        case Command::GetX:
            stat_GetXReqReceived->addData(1);
            break;
        case Command::GetSX:
            stat_GetSXReqReceived->addData(1);
            break;
        case Command::GetS:
            stat_GetSReqReceived->addData(1);
            break;
        case Command::PutM:
            stat_PutMReqReceived->addData(1);
            break;
        case Command::PutE:
            stat_PutEReqReceived->addData(1);
            break;
        case Command::PutS:
            stat_PutSReqReceived->addData(1);
            break;
        default:
            break;

    }
    if (!entry || entry->isCached()) {
        stat_cacheHits->addData(1);
    }
}
/**
 * Profile requests sent from directory controller to memory or other caches
 */
inline void MESIDirectory::profileRequestSent(MemEvent * event) {
    Command cmd = event->getCmd();
    switch(cmd) {
        case Command::PutM:
        if (event->getAddr() == 0) {
            stat_dirEntryWrites->addData(1);
        } else {
            stat_dataWrites->addData(1);
        }
        break;
        case Command::GetX:
        if (event->queryFlag(MemEvent::F_NONCACHEABLE)) {
            stat_dataWrites->addData(1);
            break;
        }
        case Command::GetSX:
        case Command::GetS:
        if (event->getAddr() == 0) {
            stat_dirEntryReads->addData(1);
        } else {
            stat_dataReads->addData(1);
        }
        break;
        case Command::FetchInv:
        stat_FetchInvSent->addData(1);
        break;
        case Command::FetchInvX:
        stat_FetchInvXSent->addData(1);
        break;
        case Command::Inv:
        stat_InvSent->addData(1);
        break;
    default:
        break;

    }
}

/**
 * Profile responses sent from directory controller to caches
 */
inline void MESIDirectory::profileResponseSent(MemEvent * event) {
    Command cmd = event->getCmd();
    switch(cmd) {
        case Command::GetSResp:
        stat_GetSRespSent->addData(1);
        break;
        case Command::GetXResp:
        stat_GetXRespSent->addData(1);
        break;
    case Command::NACK:
        stat_NACKRespSent->addData(1);
        break;
    default:
        break;
    }
}

/**
 * Profile responses received by directory controller from caches
 */
inline void MESIDirectory::profileResponseRecv(MemEvent * event) {
    Command cmd = event->getCmd();
    switch(cmd) {
        case Command::FetchResp:
        stat_FetchRespReceived->addData(1);
        break;
        case Command::FetchXResp:
        stat_FetchXRespReceived->addData(1);
        break;
    case Command::PutM:
        stat_PutMRespReceived->addData(1);
        break;
    case Command::PutE:
        stat_PutERespReceived->addData(1);
        break;
    case Command::PutS:
        stat_PutSRespReceived->addData(1);
        break;
    case Command::NACK:
        stat_NACKRespReceived->addData(1);
        break;
    default:
        break;
    }
}

/** GetS */
void MESIDirectory::handleGetS(MemEvent * event, bool replay) {
    /* Locate directory entry and allocate if needed */
    DirEntry * entry = getDirEntry(ev->getBaseAddr());
    /* Put request in MSHR (all GetS requests need to be buffered while they wait for data) and stall if there are waiting requests ahead of us */
    if (!(mshr->elementIsHit(ev->getBaseAddr(), ev))) {
        bool conflict = mshr->isHit(ev->getBaseAddr());
        if (!mshr->insert(ev->getBaseAddr(), ev)) {
            mshrNACKRequest(ev);
            return;
        }
        if (conflict) {
            return; // stall event until conflicting request finishes
        } else {
            profileRequestRecv(ev, entry);
        }
    } else if (!replay) {
        profileRequestRecv(ev, entry);
    }

    if (!entry->isCached()) {

        if (is_debug_addr(entry->getBaseAddr())) dbg.debug(_L6_, "Entry %" PRIx64 " not in cache.  Requesting from memory.\n", entry->getBaseAddr());

        getDirEntryFromMemory(entry);
        return;
    }

    State state = entry->getState();
    switch (state) {
        case I:
            entry->setState(IS);
            issueMemoryRequest(ev, entry);
            break;
        case S:
            entry->setState(S_D);
            issueMemoryRequest(ev, entry);
            break;
        case M:
            entry->setState(M_InvX);
            issueFetch(ev, entry, Command::FetchInvX);
            break;
        default:
            dbg.fatal(CALL_INFO, -1, "Directory %s received GetS but state is %s. Event: %s\n", getName().c_str(), StateString[state], ev->getVerboseString().c_str());
    }
}


/** GetX */
void MESIDirectory::handleGetX(MemEvent * ev, bool replay) {
    /* Locate directory entry and allocate if needed */
    DirEntry * entry = getDirEntry(ev->getBaseAddr());

    /* Put request in MSHR (all GetS requests need to be buffered while they wait for data) and stall if there are waiting requests ahead of us */
    if (!(mshr->elementIsHit(ev->getBaseAddr(), ev))) {
        bool conflict = mshr->isHit(ev->getBaseAddr());
        if (!mshr->insert(ev->getBaseAddr(), ev)) {
            mshrNACKRequest(ev);
            return;
        }
        if (conflict) {
            return; // stall event until conflicting request finishes
        } else {
            profileRequestRecv(ev, entry);
        }
    } else if (!replay)
        profileRequestRecv(ev, entry);

    if (!entry->isCached()) {
        if (is_debug_addr(entry->getBaseAddr())) dbg.debug(_L6_, "Entry %" PRIx64 " not in cache.  Requesting from memory.\n", entry->getBaseAddr());

        getDirEntryFromMemory(entry);
        return;
    }

    MemEvent * respEv;
    State state = entry->getState();
    switch (state) {
        case I:
            entry->setState(IM);
            issueMemoryRequest(ev, entry);
            break;
        case S:
            if (entry->getSharerCount() == 1 && entry->isSharer(node_id(ev->getSrc()))) {   // Special case: upgrade
                mshr->removeFront(ev->getBaseAddr());

                if (is_debug_event(ev)) dbg.debug(_L10_, "\t%s\tMSHR remove event <%s, %" PRIx64 ">\n", getName().c_str(), CommandString[(int)ev->getCmd()], ev->getBaseAddr());

                entry->setState(M);
                entry->removeSharer(node_name_to_id(ev->getSrc()));
                entry->setOwner(node_name_to_id(ev->getSrc()));
                respEv = ev->makeResponse();
                respEv->setSize(cacheLineSize);
                profileResponseSent(respEv);
                sendEventToCaches(respEv, timestamp + accessLatency);

                if (is_debug_event(ev)) {
                    dbg.debug(_L4_, "Sending response for 0x%" PRIx64 " to %s, send time: %" PRIu64 "\n", entry->getBaseAddr(), respEv->getDst().c_str(), timestamp + accessLatency);
                }

                postRequestProcessing(ev, entry, true);
                replayWaitingEvents(entry->getBaseAddr());
                updateCache(entry);
            } else {
                entry->setState(S_Inv);
                issueInvalidates(ev, entry, Command::Inv);
            }
            break;
        case M:
            entry->setState(M_Inv);
            issueFetch(ev, entry, Command::FetchInv);
            break;
        default:
            dbg.fatal(CALL_INFO, -1, "Directory %s received %s but state is %s. Event: %s\n", getName().c_str(), CommandString[(int)ev->getCmd()], StateString[state], ev->getVerboseString().c_str());
    }
}


/*
 * Handle PutS request - either a request or a response to an Inv
 */
void MESIDirectory::handlePutS(MemEvent * ev) {
    DirEntry * entry = getDirEntry(ev->getBaseAddr());

    entry->removeSharer(node_name_to_id(ev->getSrc()));
    if (mshr->elementIsHit(ev->getBaseAddr(), ev)) mshr->removeElement(ev->getBaseAddr(), ev);

    State state = entry->getState();
    Addr addr = entry->getBaseAddr();
    switch (state) {
        case S:
            profileRequestRecv(ev, entry);
            if (entry->getSharerCount() == 0) {
                entry->setState(I);
            }
            sendAckPut(ev);
            postRequestProcessing(ev, entry, true);   // profile & delete ev
            replayWaitingEvents(addr);
            updateCache(entry);             // update dir cache
            break;
        case S_D:
            profileRequestRecv(ev, entry);
            sendAckPut(ev);
            postRequestProcessing(ev, entry, false);
            break;
        case S_Inv:
        case SD_Inv:
            profileResponseRecv(ev);
            entry->decrementWaitingAcks();
            delete ev;
            if (entry->getWaitingAcks() == 0) {
                if (state == S_Inv) (entry->getSharerCount() > 0) ? entry->setState(S) : entry->setState(I);
                else (entry->getSharerCount() > 0) ? entry->setState(S_D) : entry->setState(IS);
                replayWaitingEvents(addr);
            }
            break;
        default:
            dbg.fatal(CALL_INFO, -1, "%s, Error: Directory received PutS but state is %s. Event = %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], ev->getVerboseString().c_str(), getCurrentSimTimeNano());
    }
}


/* Handle PutE */
void MESIDirectory::handlePutE(MemEvent * ev) {
    DirEntry * entry = getDirEntry(ev->getBaseAddr());

    /* Error checking */
    if (!((uint32_t)entry->getOwner() == node_name_to_id(ev->getSrc()))) {
	dbg.fatal(CALL_INFO, -1, "%s, Error: received PutE from a node who does not own the block. Event = %s. Time = %" PRIu64 "ns\n",
                getName().c_str(), ev->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    if (!(entry->isCached())) {
        if (!(mshr->elementIsHit(ev->getBaseAddr(),ev))) {
            stat_mshrHits->addData(1);
            bool inserted = mshr->insert(ev->getBaseAddr(),ev);

            if (is_debug_event(ev)) {
                dbg.debug(_L8_, "Inserting request in mshr. %s. MSHR size: %d\n", ev->getBriefString().c_str(), mshr->getSize());
            }

            if (!inserted) {
                if (is_debug_event(ev)) dbg.debug(_L8_, "MSHR is full. NACKing request\n");

                mshrNACKRequest(ev);
                return;
            }
        }

        if (is_debug_event(ev)) dbg.debug(_L6_, "Entry %" PRIx64 " not in cache.  Requesting from memory.\n", entry->getBaseAddr());

        getDirEntryFromMemory(entry);
        return;
    }

    /* Update owner state */
    profileRequestRecv(ev, entry);
    entry->clearOwner();

    State state = entry->getState();
    switch  (state) {
        case M:
            entry->setState(I);
            sendAckPut(ev);
            postRequestProcessing(ev, entry, true);  // profile & delete ev
            replayWaitingEvents(entry->getBaseAddr());
            updateCache(entry);         // update cache;
            break;
        case M_Inv:     /* If PutE comes with data then we can handle this immediately but otherwise */
            entry->setState(IM);
            issueMemoryRequest(static_cast<MemEvent*>(mshr->lookupFront(ev->getBaseAddr())), entry);
            postRequestProcessing(ev, entry, false);  // profile & delete ev
            break;
        case M_InvX:
            entry->setState(IS);
            issueMemoryRequest(static_cast<MemEvent*>(mshr->lookupFront(ev->getBaseAddr())), entry);
            postRequestProcessing(ev, entry, false);  // profile & delete ev
            break;
        default:
            dbg.fatal(CALL_INFO, -1, "%s, Error: Directory received PutE but state is %s. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], ev->getVerboseString().c_str(), getCurrentSimTimeNano());
    }
}


/* Handle PutM */
void MESIDirectory::handlePutM(MemEvent * ev) {
    DirEntry * entry = getDirEntry(ev->getBaseAddr());

    /* Error checking */
    if (!((uint32_t)entry->getOwner() == node_name_to_id(ev->getSrc()))) {
	dbg.fatal(CALL_INFO, -1, "%s, Error: received PutM from a node who does not own the block. Event: %s. Time = %" PRIu64 "ns\n",
                getName().c_str(), ev->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    if (!(entry->isCached())) {
        if (!(mshr->elementIsHit(ev->getBaseAddr(),ev))) {
            stat_mshrHits->addData(1);
            bool inserted = mshr->insert(ev->getBaseAddr(),ev);

            if (is_debug_event(ev)) {
                dbg.debug(_L8_, "Inserting request in mshr. Cmd = %s, BaseAddr = 0x%" PRIx64 ", Addr = 0x%" PRIx64 ", MSHR size: %d\n", CommandString[(int)ev->getCmd()], ev->getBaseAddr(), ev->getAddr(), mshr->getSize());
            }

            if (!inserted) {
                if (is_debug_event(ev)) dbg.debug(_L8_, "MSHR is full. NACKing request\n");

                mshrNACKRequest(ev);
                return;
            }
        }

        if (is_debug_event(ev)) dbg.debug(_L6_, "Entry %" PRIx64 " not in cache.  Requesting from memory.\n", entry->getBaseAddr());

        getDirEntryFromMemory(entry);
        return;
    }

    State state = entry->getState();
    switch  (state) {
        case M:
            profileRequestRecv(ev, entry);
            writebackData(ev, Command::PutM);
            entry->clearOwner();
            entry->setState(I);
            sendAckPut(ev);
            postRequestProcessing(ev, entry, true);  // profile & delete event
            replayWaitingEvents(entry->getBaseAddr());
            updateCache(entry);
            break;
        case M_Inv:
        case M_InvX:
            handleFetchResp(ev, false);
            break;
        default:
            dbg.fatal(CALL_INFO, -1, "%s, Error: Directory received PutM but state is %s. Event: %s. Time = %" PRIu64 "ns, %" PRIu64 " cycles.\n",
                    getName().c_str(), StateString[state], ev->getVerboseString().c_str(), getCurrentSimTimeNano(), timestamp);
    }
}


/*
 *  Handle FlushLine
 *  Only flush dirty data
 */
void MESIDirectory::handleFlushLine(MemEvent * ev) {
    DirEntry * entry = getDirEntry(ev->getBaseAddr());
    if (!entry->isCached()) {

        if (is_debug_addr(entry->getBaseAddr())) dbg.debug(_L6_, "Entry %" PRIx64 " not in cache.  Requesting from memory.\n", entry->getBaseAddr());

        getDirEntryFromMemory(entry);
        return;
    }

    bool shouldNACK = false;
    bool inMSHR = mshr->elementIsHit(ev->getBaseAddr(), ev);
    bool mshrConflict = !inMSHR && mshr->isHit(ev->getBaseAddr());

    int srcID = node_id(ev->getSrc());
    State state = entry->getState();

    switch(state) {
        case I:
            if (!inMSHR) {
                if (!mshr->insert(ev->getBaseAddr(), ev)) {
                    mshrNACKRequest(ev);
                    break;
                } else profileRequestRecv(ev, entry);
                if (mshrConflict) break;
            }
            forwardFlushRequest(ev);
            break;
        case S:
            if (!inMSHR && !mshr->insert(ev->getBaseAddr(), ev)) {
                mshrNACKRequest(ev);
                break;
            }
            forwardFlushRequest(ev);
            break;
        case M:
            if (!inMSHR && !mshr->insert(ev->getBaseAddr(), ev)) {
                mshrNACKRequest(ev);
                break;
            }
            if (entry->getOwner() == srcID) {
                entry->clearOwner();
                entry->addSharer(srcID);
                entry->setState(S);
                if (ev->getDirty()) {
                    writebackData(ev, Command::FlushLine);
                } else {
                    forwardFlushRequest(ev);
                }
            } else {
                entry->setState(M_InvX);
                issueFetch(ev, entry, Command::FetchInvX);
            }
            break;
        case IS:        // Sharer in progress
        case IM:        // Owner in progress
        case S_D:       // Sharer in progress
        case SD_Inv:    // Sharer and inv in progress
        case S_Inv:     // Owner in progress, waiting on AckInv
            if (!inMSHR && !mshr->insert(ev->getBaseAddr(), ev)) mshrNACKRequest(ev);
            break;
        case M_Inv:     // Owner in progress, waiting on FetchInvResp
            // Assume that the sender will treat our outstanding FetchInv as an Inv and clean up accordingly
            if (entry->getOwner() == srcID) {
                entry->clearOwner();
                entry->addSharer(srcID);
                entry->setState(S_Inv);
                entry->incrementWaitingAcks();
                if (ev->getDirty()) {
                    writebackData(ev, Command::PutM);
                    ev->setPayload(0, NULL);
                    ev->setDirty(false);
                }
            }
            if (!inMSHR && !mshr->insert(ev->getBaseAddr(), ev)) mshrNACKRequest(ev);
            break;
        case M_InvX:    // Sharer in progress, waiting on FetchInvXResp
            // Clear owner, add owner as sharer
            if (entry->getOwner() == srcID) {
                if (ev->getDirty()) {
                    handleFetchXResp(ev, true);
                    ev->setPayload(0, NULL);
                    ev->setDirty(false);
                } else {
                    entry->clearOwner();
                    entry->addSharer(srcID);
                    entry->setState(S);
                }
                if (!inMSHR && !mshr->insert(ev->getBaseAddr(), ev)) mshrNACKRequest(ev);
                replayWaitingEvents(entry->getBaseAddr());
                updateCache(entry);
            } else if (!inMSHR && !mshr->insert(ev->getBaseAddr(), ev)) mshrNACKRequest(ev);
            break;
        default:
            dbg.fatal(CALL_INFO, -1, "%s, Error: Directory received FlushLine but state is %s. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], ev->getVerboseString().c_str(), getCurrentSimTimeNano());
    }
}


/*
 *  Handle FlushLineInv
 *  Wait for mem ack if dirty, else local OK.
 */
void MESIDirectory::handleFlushLineInv(MemEvent * ev) {
    /* Get directory entry (create if it didn't exist, or pull from memory */
    DirEntry * entry = getDirEntry(ev->getBaseAddr());

    if (!entry->isCached()) {
        if (is_debug_addr(entry->getBaseAddr())) dbg.debug(_L6_, "Entry %" PRIx64 " not in cache.  Requesting from memory.\n", entry->getBaseAddr());

        getDirEntryFromMemory(entry);
        return;
    }

    bool shouldNACK = false;
    bool inMSHR = mshr->elementIsHit(ev->getBaseAddr(), ev);
    bool mshrConflict = !inMSHR && mshr->isHit(ev->getBaseAddr());

    int srcID = node_id(ev->getSrc());
    State state = entry->getState();

    switch (state) {
        case I:
            if (!inMSHR) {
                if (!mshr->insert(ev->getBaseAddr(), ev)) {
                    mshrNACKRequest(ev);
                    break;
                } else profileRequestRecv(ev, entry);
                if (mshrConflict) break;
            }
            forwardFlushRequest(ev);
            break;
        case S:
            if (!inMSHR && !mshr->insert(ev->getBaseAddr(), ev)) {
                mshrNACKRequest(ev);
                break;
            }
            if (entry->isSharer(srcID)) entry->removeSharer(srcID);
            if (entry->getSharerCount() == 0) {
                entry->setState(I);
                forwardFlushRequest(ev);
            } else {
                entry->setState(S_Inv);
                issueInvalidates(ev, entry, Command::Inv);
            }
            break;
        case M:
            if (!inMSHR && !mshr->insert(ev->getBaseAddr(), ev)) {
                mshrNACKRequest(ev);
                break;
            }
            if (entry->getOwner() == srcID) {
                entry->clearOwner();
                entry->setState(I);
                if (ev->getDirty()) {
                    writebackData(ev, Command::FlushLine);
                } else {
                    forwardFlushRequest(ev);
                }
            } else {
                entry->setState(M_Inv);
                issueFetch(ev, entry, Command::FetchInv);
            }
            break;
        case IS:        // Sharer in progress
        case IM:        // Owner in progress
        case S_D:       // Sharer in progress
            if (!inMSHR && !mshr->insert(ev->getBaseAddr(), ev)) mshrNACKRequest(ev);
            break;
        case S_Inv:     // Owner in progress, waiting on AckInv
        case SD_Inv:
            if (!inMSHR && !mshr->insert(ev->getBaseAddr(), ev)) mshrNACKRequest(ev);
            if (entry->isSharer(srcID)) {
                profileResponseRecv(ev);
                entry->removeSharer(srcID);
                entry->decrementWaitingAcks();
                if (entry->getWaitingAcks() == 0) {
                    if (state == S_Inv) entry->getSharerCount() > 0 ? entry->setState(S) : entry->setState(I);
                    else entry->getSharerCount() > 0 ? entry->setState(S_D) : entry->setState(IS);
                    replayWaitingEvents(ev->getBaseAddr());
                    updateCache(entry);
                }
            }
            break;
        case M_Inv:     // Owner in progress, waiting on FetchInvResp
        case M_InvX:    // Sharer in progress, waiting on FetchInvXResp
            if (entry->getOwner() == srcID) {
                if (ev->getDirty()) {
                    handleFetchResp(ev, true); // don't delete the event! we need it!
                    ev->setDirty(false);        // Don't writeback data when event is replayed
                    ev->setPayload(0, NULL);
                } else {
                    entry->clearOwner();
                    // Don't have data, set state to I & retry
                    entry->setState(I);
                }
                if (!inMSHR && !mshr->insert(ev->getBaseAddr(), ev)) mshrNACKRequest(ev);
                replayWaitingEvents(entry->getBaseAddr());
                updateCache(entry);
            } else if (!inMSHR && !mshr->insert(ev->getBaseAddr(), ev)) mshrNACKRequest(ev);
            break;
        default:
            dbg.fatal(CALL_INFO, -1, "%s, Error: Directory received FlushLineInv but state is %s. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], ev->getVerboseString().c_str(), getCurrentSimTimeNano());
    }
}



/* Handle the incoming event as a fetch Response (FetchResp, FetchXResp, PutM) */
void MESIDirectory::handleFetchResp(MemEvent * ev, bool keepEvent) {

    DirEntry * entry = getDirEntry(ev->getBaseAddr());
    MemEvent * reqEv = mshr->removeFront(ev->getBaseAddr());

    if (is_debug_event(ev)) dbg.debug(_L4_, "Finishing Fetch for reqEv = %s.\n", reqEv->getBriefString().c_str());

    /* Error checking */
    if (!((uint32_t)entry->getOwner() == node_name_to_id(ev->getSrc()))) {
	dbg.fatal(CALL_INFO, -1, "%s, Error: received FetchResp from a node who does not own the block. Event: %s. Time = %" PRIu64 "ns, %" PRIu64 " cycles\n",
                getName().c_str(), ev->getVerboseString().c_str(), getCurrentSimTimeNano(), timestamp);
    }

    /* Profile response */
    profileResponseRecv(ev);

    /* Clear previous owner state and writeback block. */
    entry->clearOwner();

    MemEvent * respEv = nullptr;
    State state = entry->getState();

    /* Handle request */
    switch (state) {
        case M_Inv:
            if (reqEv->getCmd() != Command::FetchInv && reqEv->getCmd() != Command::ForceInv) { // GetX request, not back invalidation
                writebackData(ev, Command::PutM);
                entry->setOwner(node_id(reqEv->getSrc()));
                entry->setState(M);
            } else entry->setState(I);
            respEv = reqEv->makeResponse();
            break;
        case M_InvX:    // GetS request
            if (reqEv->getCmd() == Command::FetchInv || reqEv->getCmd() == Command::ForceInv) {
                // TODO should probably just handle the Get* that caused a FetchInvX first and then do the FetchInv/ForceInv
                // Actually should only do FetchInv/ForceInv first if it required a request to memory (IS or IM) -> everything else should
                // have the FetchInv/ForceInv go second
            }
            writebackData(ev, Command::PutM);
            if (protocol == CoherenceProtocol::MESI && entry->getSharerCount() == 0) {
                entry->setOwner(node_id(reqEv->getSrc()));
                respEv = reqEv->makeResponse(Command::GetXResp);
                entry->setState(M);
            } else {
                entry->addSharer(node_id(reqEv->getSrc()));
                respEv = reqEv->makeResponse();
                entry->setState(S);
            }
            break;
        default:
            dbg.fatal(CALL_INFO, -1, "%s, Error: Directory received %s but state is %s. Event: %s. Time = %" PRIu64 "ns, %" PRIu64 " cycles\n",
                    getName().c_str(), CommandString[(int)ev->getCmd()], StateString[state], ev->getVerboseString().c_str(), getCurrentSimTimeNano(), timestamp);
    }
    respEv->setPayload(ev->getPayload());
    profileResponseSent(respEv);
    if (reqEv->getCmd() == Command::FetchInv || reqEv->getCmd() == Command::ForceInv)
        memMsgQueue.insert(std::make_pair(timestamp + mshrLatency, respEv));
    else
        sendEventToCaches(respEv, timestamp + mshrLatency);

    if (!keepEvent) delete ev;

    postRequestProcessing(reqEv, entry, true);
    replayWaitingEvents(entry->getBaseAddr());
    updateCache(entry);
}


/* FetchXResp */
void MESIDirectory::handleFetchXResp(MemEvent * ev, bool keepEvent) {
    if (is_debug_event(ev)) dbg.debug(_L4_, "Finishing Fetch.\n");

    DirEntry * entry = getDirEntry(ev->getBaseAddr());
    MemEvent * reqEv = mshr->removeFront(ev->getBaseAddr());

    /* Error checking */
    if (!((uint32_t)entry->getOwner() == node_name_to_id(ev->getSrc()))) {
	dbg.fatal(CALL_INFO, -1, "%s, Error: received FetchResp from a node who does not own the block. Event = %s. Time = %" PRIu64 "ns, %" PRIu64 " cycles\n",
                getName().c_str(), ev->getVerboseString().c_str(), getCurrentSimTimeNano(), timestamp );
    }

    /* Profile response */
    profileResponseRecv(ev);

    /* Clear previous owner state and writeback block. */
    entry->clearOwner();
    entry->addSharer(node_name_to_id(ev->getSrc()));
    entry->setState(S);
    if (ev->getDirty()) writebackData(ev, Command::PutM);

    MemEvent * respEv = reqEv->makeResponse();
    entry->addSharer(node_id(reqEv->getSrc()));

    respEv->setPayload(ev->getPayload());
    profileResponseSent(respEv);
    sendEventToCaches(respEv, timestamp + mshrLatency);

    if (!keepEvent) delete ev;

    postRequestProcessing(reqEv, entry, true);
    replayWaitingEvents(entry->getBaseAddr());
    updateCache(entry);

}


/* AckInv */
void MESIDirectory::handleAckInv(MemEvent * ev) {
    DirEntry * entry = getDirEntry(ev->getBaseAddr());

    if (entry->isSharer(node_id(ev->getSrc())))
        entry->removeSharer(node_name_to_id(ev->getSrc()));
    if ((uint32_t)entry->getOwner() == node_name_to_id(ev->getSrc()))
        entry->clearOwner();

    if (mshr->elementIsHit(ev->getBaseAddr(), ev)) mshr->removeElement(ev->getBaseAddr(), ev);

    State state = entry->getState();
    Addr addr = entry->getBaseAddr();
    switch (state) {
        case S_Inv:
        case SD_Inv:
            profileResponseRecv(ev);
            entry->decrementWaitingAcks();
            delete ev;
            if (entry->getWaitingAcks() == 0) {
                if (state == S_Inv) entry->getSharerCount() > 0 ? entry->setState(S) : entry->setState(I);
                else entry->getSharerCount() > 0 ? entry->setState(S_D) : entry->setState(IS);
                replayWaitingEvents(addr);
            }
            break;
        case M_Inv:
            profileResponseRecv(ev);
            delete ev;
            entry->setState(I);
            replayWaitingEvents(addr);
            break;
        default:
            dbg.fatal(CALL_INFO, -1, "%s, Error: Directory received AckInv but state is %s. Event = %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], ev->getVerboseString().c_str(), getCurrentSimTimeNano());
    }
}


/*
 *  Handle a backwards invalidation (shootdown)
 */
void MESIDirectory::handleBackInv(MemEvent* ev) {
    DirEntry * entry = getDirEntry(ev->getBaseAddr());

    /* Put invalidation in mshr if needed -> resolve conflicts
     * so that invalidation takes precedence over
     * memory requests */
    bool noreplay = false;
    if (!(mshr->elementIsHit(ev->getBaseAddr(), ev))) {
        /*
         *  GetS/X/etc: Jump ahead and resolve now
         *  Flush: to-do
         *  PutE,M: Wait, only happens when the dir entry i snot cached
         *  Could also be WB Ack
         */
        bool conflict = mshr->isHit(ev->getBaseAddr());
        bool inserted = false;
        if (conflict) {
            if (mshr->exists(ev->getBaseAddr())) { // front entry is an event
                /* If state is IS, IM, or S_D, we are waiting on memory so resolve this Inv first to avoid deadlock */
                if (entry->getState() == IS || entry->getState() == IM || entry->getState() == S_D) {
                    inserted = mshr->insertInv(ev->getBaseAddr(), ev, false); // insert front
                    conflict = false; // don't stall
                    noreplay = true; // don't replay second event if this one completes
                } else {
                    inserted = mshr->insertInv(ev->getBaseAddr(), ev, true); // insert second
                }
            } else {
                inserted = mshr->insertInv(ev->getBaseAddr(), ev, true); // insert second
            }
        } else {
            inserted = mshr->insert(ev->getBaseAddr(), ev);
        }

        if (!inserted) {
            mshrNACKRequest(ev, true);
            return;
        }
        if (conflict) return; // stall until conflicting request finishes
        else profileRequestRecv(ev, entry);
    }

    if (!entry->isCached()) {
        if (is_debug_addr(entry->getBaseAddr())) dbg.debug(_L6_, "Entry %" PRIx64 " not in cache.  Requesting from memory.\n", entry->getBaseAddr());

        getDirEntryFromMemory(entry);
        return;
    }

    MemEvent * respEv;
    State state = entry->getState();
    switch (state) {
        case I:
        case IS:
        case IM:
            respEv = ev->makeResponse(Command::AckInv);
            respEv->setDst(memoryName);
            memMsgQueue.insert(std::make_pair(timestamp + accessLatency, respEv));
            if (mshr->elementIsHit(ev->getBaseAddr(), ev)) mshr->removeElement(ev->getBaseAddr(), ev);
            if (!noreplay) replayWaitingEvents(entry->getBaseAddr());
            break;
        case S:
            entry->setState(S_Inv);
            issueInvalidates(ev, entry, Command::Inv); // Fwd inv for forceInv and fetchInv
            break;
        case M:
            entry->setState(M_Inv);
            issueFetch(ev, entry, ev->getCmd());
            break;
        case S_D:
            // Need to order inv ahead of read and end up in IS; won't get data response
            // until we end up in IS so no need to figure out races
            entry->setState(SD_Inv);
            issueInvalidates(ev, entry, Command::Inv);
            break;
        case I_d:
        case S_d:
        case M_d:
            break; // Stall for entry response
        default:
            dbg.fatal(CALL_INFO, -1, "%s, Error: No handler for event in state %s. Event = %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], ev->getVerboseString().c_str(), getCurrentSimTimeNano());
    }
}


/* handle NACK */
void MESIDirectory::handleNACK(MemEvent * ev) {
    MemEvent* origEvent = ev->getNACKedEvent();
    profileResponseRecv(ev);

    DirEntry *entry = getDirEntry(origEvent->getBaseAddr());
    if (is_debug_event(ev)) {
        dbg.debug(_L5_, "Orig resp ID = (%" PRIu64 ",%d), Nack resp ID = (%" PRIu64 ",%d), last req ID = (%" PRIu64 ",%d)\n",
	        origEvent->getResponseToID().first, origEvent->getResponseToID().second, ev->getResponseToID().first,
        	ev->getResponseToID().second, entry->lastRequest.first, entry->lastRequest.second);
    }

    /* Retry request if it has not already been handled */
    if ((ev->getResponseToID() == entry->lastRequest) || origEvent->getCmd() == Command::Inv) {
	/* Re-send request */
	sendEventToCaches(origEvent, timestamp + mshrLatency);

        if (is_debug_event(ev)) dbg.debug(_L5_,"Orig Cmd NACKed, retry = %s \n", CommandString[(int)origEvent->getCmd()]);
    } else {
	if (is_debug_event(ev)) dbg.debug(_L5_,"Orig Cmd NACKed, no retry = %s \n", CommandString[(int)origEvent->getCmd()]);
    }

    delete ev;
    return;
}


/* Memory response handler - calls handler for each event from memory */
void MESIDirectory::handleMemoryResponse(SST::Event *event){
    MemEvent *ev = static_cast<MemEvent*>(event);

    if (is_debug_event(ev)) {
        dbg.debug(_L3_, "\n%" PRIu64 " (%s) Received: %s\n",
                getCurrentSimTimeNano(), getName().c_str(), ev->getVerboseString().c_str());
    }

    if (!clockOn) {
        turnClockOn();
    }

    if (ev->queryFlag(MemEvent::F_NONCACHEABLE)) {
        if (noncacheMemReqs.find(ev->getResponseToID()) != noncacheMemReqs.end()) {
            ev->setDst(noncacheMemReqs[ev->getResponseToID()]);
            ev->setSrc(getName());

            noncacheMemReqs.erase(ev->getResponseToID());
            profileResponseSent(ev);

            network->send(ev);

            return;
        }
        dbg.fatal(CALL_INFO, -1, "%s, Error: Received unexpected noncacheable response from memory. %s. Time = %" PRIu64 "ns\n",
                getName().c_str(), ev->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    /* Cacheable response - figure out what it is for */
    if (ev->getCmd() == Command::ForceInv || ev->getCmd() == Command::FetchInv) {
        handleBackInv(ev);
        return; // don't delete
    } else if (ev->getCmd() == Command::AckPut) {
        mshr->removeWriteback(memReqs[ev->getResponseToID()]);
        memReqs.erase(ev->getResponseToID());
        replayWaitingEvents(ev->getBaseAddr());
    } else if (ev->getBaseAddr() == 0 && dirEntryMiss.find(ev->getResponseToID()) != dirEntryMiss.end()) {    // directory entry miss
        ev->setBaseAddr(dirEntryMiss[ev->getResponseToID()]);
        dirEntryMiss.erase(ev->getResponseToID());
        handleDirEntryMemoryResponse(ev);
    } else if (memReqs.find(ev->getResponseToID()) != memReqs.end()) {
        ev->setBaseAddr(memReqs[ev->getResponseToID()]);
        memReqs.erase(ev->getResponseToID());
        if (ev->getCmd() == Command::FlushLineResp) handleFlushLineResponse(ev);
        else handleDataResponse(ev);

    } else {
        dbg.fatal(CALL_INFO, -1, "%s, Error: Received unexpected response from memory - matching request not found. %s. Time = %" PRIu64 "ns\n",
                getName().c_str(), ev->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    delete ev;
}


/* Handle GetSResp or GetXResp from memory */
void MESIDirectory::handleDataResponse(MemEvent * ev) {
    DirEntry * entry = getDirEntry(ev->getBaseAddr());

    State state = entry->getState();

    MemEvent * reqEv = mshr->removeFront(ev->getBaseAddr());
    if (is_debug_event(ev)) dbg.debug(_L10_, "\t%s\tMSHR remove event <%s, %" PRIx64 ">\n", getName().c_str(), CommandString[(int)reqEv->getCmd()], reqEv->getBaseAddr());
    //dbg.debug(_L9_, "\t%s\tHandling stalled event: %s, %s\n", CommandString[(int)reqEv->getCmd()], reqEv->getSrc().c_str());

    MemEvent * respEv = nullptr;
    switch (state) {
        case IS:
        case S_D:
            if (protocol == CoherenceProtocol::MESI && entry->getSharerCount() == 0) {
                respEv = reqEv->makeResponse(Command::GetXResp);
                entry->setState(M);
                entry->setOwner(node_id(reqEv->getSrc()));
            } else {
                respEv = reqEv->makeResponse();
                entry->setState(S);
                entry->addSharer(node_id(reqEv->getSrc()));
            }
            break;
        case IM:
        case SM:
            respEv = reqEv->makeResponse();
            entry->setState(M);
            entry->setOwner(node_id(reqEv->getSrc()));
            entry->clearSharers();  // Case SM: new owner was a sharer
            break;
        default:
            dbg.fatal(CALL_INFO,1,"Directory %s received Get Response for addr 0x%" PRIx64 " but state is %s. Event: %s\n",
                    getName().c_str(), ev->getBaseAddr(), StateString[state], ev->getVerboseString().c_str());
    }

    respEv->setSize(cacheLineSize);
    respEv->setPayload(ev->getPayload());
    respEv->setMemFlags(ev->getMemFlags());
    profileResponseSent(respEv);
    sendEventToCaches(respEv, timestamp + mshrLatency);

    if (is_debug_event(ev)) {
        dbg.debug(_L4_, "\tSending requested data for 0x%" PRIx64 " to %s\n", entry->getBaseAddr(), respEv->getDst().c_str());
    }

    postRequestProcessing(reqEv, entry, true);
    replayWaitingEvents(entry->getBaseAddr());
    updateCache(entry);
}


/* Handle FlushLineResp from memory */
void MESIDirectory::handleFlushLineResponse(MemEvent * ev) {
    MemEvent * reqEv = mshr->removeFront(ev->getBaseAddr());

    if (is_debug_event(ev)) dbg.debug(_L10_, "\t%s\tMSHR remove event <%s, %" PRIx64 ">\n", getName().c_str(), CommandString[(int)reqEv->getCmd()], reqEv->getBaseAddr());
    //dbg.debug(_L9_, "\t%s\tHandling stalled event: %s, %s\n", CommandString[(int)reqEv->getCmd()], reqEv->getSrc().c_str());

    reqEv->setMemFlags(ev->getMemFlags()); // Copy anything back up that needs to be

    MemEvent * me = reqEv->makeResponse();
    me->setDst(reqEv->getSrc());
    me->setRqstr(reqEv->getRqstr());
    me->setSuccess(ev->queryFlag(MemEvent::F_SUCCESS));
    me->setMemFlags(reqEv->getMemFlags());

    profileResponseSent(me);
    uint64_t deliveryTime = timestamp + accessLatency;
    netMsgQueue.insert(std::make_pair(deliveryTime, me));

    replayWaitingEvents(reqEv->getBaseAddr());

    delete reqEv;
}


/* Handle response for directory entry */
void MESIDirectory::handleDirEntryMemoryResponse(MemEvent * ev) {
    Addr dirAddr = ev->getBaseAddr();

    DirEntry * entry = getDirEntry(dirAddr);
    entry->setCached(true);

    State st = entry->getState();
    switch (st) {
        case I_d:
            entry->setState(I);
            break;
        case S_d:
            entry->setState(S);
            break;
        case M_d:
            entry->setState(M);
            break;
        default:
            dbg.fatal(CALL_INFO, -1, "Directory Controller %s: DirEntry response received for addr 0x%" PRIx64 " but state is %s. Event: %s\n",
                    getName().c_str(), entry->getBaseAddr(), StateString[st], ev->getVerboseString().c_str());
    }
    MemEvent * reqEv = static_cast<MemEvent*>(mshr->lookupFront(dirAddr));
    processPacket(reqEv, true);
}









void MESIDirectory::issueInvalidates(MemEvent * ev, DirEntry * entry, Command cmd) {
    uint32_t rqst_id = node_id(ev->getSrc());
    for (uint32_t i = 0; i < numTargets; i++) {
        if (i == rqst_id) continue;
        if (entry->isSharer(i)) {
            sendInvalidate(i, ev, entry, cmd);
            entry->incrementWaitingAcks();
        }
    }
    entry->lastRequest = DirEntry::NO_LAST_REQUEST;

    if (is_debug_addr(entry->getBaseAddr())) dbg.debug(_L4_, "Sending Invalidates to fulfill request for exclusive, BsAddr = %" PRIx64 ".\n", entry->getBaseAddr());
}

/* Send Fetch to owner */
void MESIDirectory::issueFetch(MemEvent * ev, DirEntry * entry, Command cmd) {
    MemEvent * fetch = new MemEvent(getName(), ev->getAddr(), ev->getBaseAddr(), cmd, cacheLineSize);
    fetch->setDst(nodeid_to_name[entry->getOwner()]);
    entry->lastRequest = fetch->getID();
    profileRequestSent(fetch);
    sendEventToCaches(fetch, timestamp + accessLatency);
}

/* Send Get* request to memory */
void MESIDirectory::issueMemoryRequest(MemEvent * ev, DirEntry * entry) {
    MemEvent *reqEv         = new MemEvent(*ev);
    reqEv->setSrc(getName());
    reqEv->setDst(memoryName);
    memReqs[reqEv->getID()] = ev->getBaseAddr();
    profileRequestSent(reqEv);

    uint64_t deliveryTime = timestamp + accessLatency;

    memMsgQueue.insert(std::make_pair(deliveryTime, reqEv));

    if (is_debug_addr(entry->getBaseAddr())) {
        dbg.debug(_L5_, "\tRequesting data from memory.  Cmd = %s, BaseAddr = x%" PRIx64 ", Size = %u, noncacheable = %s\n",
                CommandString[(int)reqEv->getCmd()], reqEv->getBaseAddr(), reqEv->getSize(), reqEv->queryFlag(MemEvent::F_NONCACHEABLE) ? "true" : "false");
    }
}











/* Send request for directory entry to memory */
void MESIDirectory::getDirEntryFromMemory(DirEntry * entry) {
    State st = entry->getState();
    switch (st) {
        case I:
            entry->setState(I_d);
            break;
        case S:
            entry->setState(S_d);
            break;
        case M:
            entry->setState(M_d);
            break;
        case I_d:
        case S_d:
        case M_d:
            return; // Already requested, just stall
        default:
            dbg.fatal(CALL_INFO,-1,"Direcctory Controller %s: cache miss for addr 0x%" PRIx64 " but state is %s\n",getName().c_str(),entry->getBaseAddr(), StateString[st]);
    }

    Addr entryAddr       = 0; /* Dummy addr reused for dir cache misses */
    MemEvent *me         = new MemEvent(getName(), entryAddr, entryAddr, Command::GetS, cacheLineSize);
    me->setAddrGlobal(false);
    me->setSize(entrySize);
    me->setDst(memoryName);
    dirEntryMiss[me->getID()] = entry->getBaseAddr();
    profileRequestSent(me);

    uint64_t deliveryTime = timestamp + accessLatency;

    memMsgQueue.insert(std::make_pair(deliveryTime, me));

    if (is_debug_addr(entry->getBaseAddr())) dbg.debug(_L10_, "Requesting Entry from memory for 0x%" PRIx64 "(%" PRIu64 ", %d)\n", entry->getBaseAddr(), me->getID().first, me->getID().second);
}



void MESIDirectory::mshrNACKRequest(MemEvent* ev, bool mem) {
    MemEvent * nackEv = ev->makeNACKResponse(ev);
    profileResponseSent(nackEv);
    if (mem)
        memMsgQueue.insert(std::make_pair(timestamp + 1, nackEv));
    else
        sendEventToCaches(nackEv, timestamp + 1);
}

void MESIDirectory::printStatus(Output &statusOut) {
}

void MESIDirectory::emergencyShutdown() {
    if (out.getVerboseLevel() > 1) {
        if (out.getOutputLocation() == Output::STDOUT)
            out.setOutputLocation(Output::STDERR);
        printStatus(out);
    }
}


MESIDirectory::DirEntry* MESIDirectory::getDirEntry(Addr baseAddr){
    std::unordered_map<Addr,DirEntry*>::iterator i = directory.find(baseAddr);
    DirEntry *entry;
    if (directory.end() == i) {
        entry = new DirEntry(baseAddr, numTargets, &dbg);
        entry->cacheIter = entryCache.end();
        directory[baseAddr] = entry;
        entry->setCached(true);   // TODO fix this so new entries go to memory if we're caching, little bit o cheatin here
    } else {
        entry = i->second;
    }
    return entry;
}


void MESIDirectory::sendInvalidate(int target, MemEvent * reqEv, DirEntry* entry, Command cmd){
    MemEvent *me = new MemEvent(getName(), entry->getBaseAddr(), entry->getBaseAddr(), cmd, cacheLineSize);
    me->setDst(nodeid_to_name[target]);
    me->setRqstr(reqEv->getRqstr());

    if (is_debug_event(reqEv)) dbg.debug(_L4_, "Sending Invalidate.  Dst: %s\n", nodeid_to_name[target].c_str());
    profileRequestSent(me);

    uint64_t deliveryTime = timestamp + accessLatency;
    netMsgQueue.insert(std::make_pair(deliveryTime, me));
}

void MESIDirectory::sendAckPut(MemEvent * event) {
    MemEvent * me = event->makeResponse(Command::AckPut);
    me->setDst(event->getSrc());
    me->setRqstr(event->getRqstr());
    me->setPayload(0, nullptr);
    me->setSize(cacheLineSize);

    profileResponseSent(me);

    uint64_t deliveryTime = timestamp + accessLatency;
    netMsgQueue.insert(std::make_pair(deliveryTime, me));

}


void MESIDirectory::forwardFlushRequest(MemEvent * event) {
    MemEvent *reqEv     = new MemEvent(getName(), event->getAddr(), event->getBaseAddr(), Command::FlushLine, cacheLineSize);
    reqEv->setRqstr(event->getRqstr());
    reqEv->setVirtualAddress(event->getVirtualAddress());
    reqEv->setInstructionPointer(event->getInstructionPointer());
    reqEv->setMemFlags(event->getMemFlags());
    memReqs[reqEv->getID()] = event->getBaseAddr();
    profileRequestSent(reqEv);


    uint64_t deliveryTime = timestamp + accessLatency;
    reqEv->setDst(memoryName);

    memMsgQueue.insert(std::make_pair(deliveryTime, reqEv));

    if (is_debug_event(event)) {
        dbg.debug(_L5_, "Forwarding FlushLine to memory. BaseAddr = x%" PRIx64 ", Size = %u, noncacheable = %s\n",
                reqEv->getBaseAddr(), reqEv->getSize(), reqEv->queryFlag(MemEvent::F_NONCACHEABLE) ? "true" : "false");
    }
}

uint32_t MESIDirectory::node_id(const std::string &name){
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



uint32_t MESIDirectory::node_name_to_id(const std::string &name){
    std::map<std::string, uint32_t>::iterator i = node_lookup.find(name);

    if(node_lookup.end() == i) {
	dbg.fatal(CALL_INFO, -1, "%s, Error: Attempt to lookup node ID but name not found: %s. Time = %" PRIu64 "ns\n",
                getName().c_str(), name.c_str(), getCurrentSimTimeNano());
    }

    uint32_t id = i->second;
    return id;
}

void MESIDirectory::updateCache(DirEntry *entry){
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
        if (entry->getState() == I){
            if (is_debug_addr(entry->getBaseAddr())) dbg.debug(_L10_, "Entry for 0x%" PRIx64 " has no references - purging\n", entry->getBaseAddr());

            directory.erase(entry->getBaseAddr());
            delete entry;
            return;
        } else {
            entryCache.push_front(entry);
            entry->cacheIter = entryCache.begin();
            ++entryCacheSize;

            while(entryCacheSize > entryCacheMaxSize){
                DirEntry *oldEntry = entryCache.back();
                // If the oldest entry is still in progress, everything is in progress
                if(mshr->isHit(oldEntry->getBaseAddr())) break;

                if (is_debug_addr(entry->getBaseAddr())) dbg.debug(_L10_, "entryCache too large.  Evicting entry for 0x%" PRIx64 "\n", oldEntry->getBaseAddr());

                entryCache.pop_back();
                --entryCacheSize;
                oldEntry->cacheIter = entryCache.end();
                oldEntry->setCached(false);
                sendEntryToMemory(oldEntry);
            }
        }
    }
}



void MESIDirectory::sendEntryToMemory(DirEntry *entry){
    Addr entryAddr = 0; // Always use local address 0 for directory entries
    MemEvent *me   = new MemEvent(getName(), entryAddr, entryAddr, Command::PutE, cacheLineSize); // MemController discards PutE's without writeback so this is safe
    me->setSize(entrySize);
    profileRequestSent(me);

    uint64_t deliveryTime = timestamp + accessLatency;
    me->setDst(memoryName);

    memMsgQueue.insert(std::make_pair(deliveryTime, me));
}



MemEvent::id_type MESIDirectory::writebackData(MemEvent *data_event, Command wbCmd) {
    MemEvent *ev       = new MemEvent(getName(), data_event->getBaseAddr(), data_event->getBaseAddr(), wbCmd, cacheLineSize);

    if(data_event->getPayload().size() != cacheLineSize) {
	dbg.fatal(CALL_INFO, -1, "%s, Error: Writing back data request but payload does not match cache line size of %uB. Event: %s. Time = %" PRIu64 "ns\n",
                getName().c_str(), cacheLineSize, ev->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    ev->setSize(data_event->getPayload().size());
    ev->setPayload(data_event->getPayload());
    ev->setDst(memoryName);
    profileRequestSent(ev);

    /* We will get a response if this is a flush request or if we are talking to an endpoint that sends WB Acks */
    if (!ev->isWriteback() || waitWBAck) memReqs[ev->getID()] = data_event->getBaseAddr();
    if (ev->isWriteback() && waitWBAck) {
        mshr->insertWriteback(data_event->getBaseAddr());
    }

    uint64_t deliveryTime = timestamp + accessLatency;
    memMsgQueue.insert(std::make_pair(deliveryTime, ev));

    if (is_debug_event(ev)) {
        dbg.debug(_L5_, "\tWriting back data. %s.\n", ev->getBriefString().c_str());
    }

    return ev->getID();
}

void MESIDirectory::postRequestProcessing(MemEvent * ev, DirEntry * entry, bool stable) {
    Command cmd = ev->getCmd();
    if (cmd == Command::GetS || cmd == Command::GetX || cmd == Command::GetSX) {
        stat_getRequestLatency->addData(getCurrentSimTimeNano() - ev->getDeliveryTime());
    } else {
        stat_replacementRequestLatency->addData(getCurrentSimTimeNano() - ev->getDeliveryTime());
    }

    delete ev;
    if (stable)
        entry->setToSteadyState();
}

void MESIDirectory::replayWaitingEvents(Addr addr) {
    if (mshr->isHit(addr)) {
        list<MSHREntry> * replayEntries = mshr->getAll(addr);
        if (replayEntries->begin()->elem.isEvent()) {
            MemEvent *ev = (replayEntries->begin()->elem).getEvent();

            if (is_debug_addr(addr)) dbg.debug(_L5_, "\tReactivating event. %s\n", ev->getBriefString().c_str());

            retryBuffer.push_back(ev);
        }
    }
}

void MESIDirectory::sendEventToCaches(MemEventBase *ev, uint64_t deliveryTime){
    netMsgQueue.insert(std::make_pair(deliveryTime, ev));
}


void MESIDirectory::sendEventToMem(MemEventBase *ev) {
    if (memLink) memLink->send(ev);
    else network->send(ev);
}


bool MESIDirectory::isRequestAddressValid(Addr addr){
    if (!memLink) return network->isRequestAddressValid(addr);
    else return memLink->isRequestAddressValid(addr);

    if(0 == region.interleaveSize) {
        return (addr >= region.start && addr < region.end);
    } else {
        if (addr < region.start) return false;
        if (addr >= region.end) return false;

        addr        = addr - region.start;
        Addr offset = addr % region.interleaveStep;

        if (offset >= region.interleaveSize) return false;
        return true;
    }

}

/*static char dirEntStatus[1024] = {0};
const char* MESIDirectory::printDirectoryEntryStatus(Addr baseAddr){
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
}*/

