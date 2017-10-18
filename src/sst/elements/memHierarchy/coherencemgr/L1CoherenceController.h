// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2017, Sandia Corporation
// All rights reserved.
// 
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef L1COHERENCECONTROLLER_H
#define L1COHERENCECONTROLLER_H

#include <iostream>
#include "coherencemgr/coherenceController.h"


namespace SST { namespace MemHierarchy {

class L1CoherenceController : public CoherenceController {
public:
    /** Constructor for L1CoherenceController */
    L1CoherenceController(Component* comp, Params& params) : CoherenceController(comp, params) {
        debug->debug(_INFO_,"--------------------------- Initializing [L1Controller] ... \n\n");
        
        snoopL1Invs_ = params.find<bool>("snoop_l1_invalidations", false);
        protocol_ = params.find<bool>("protocol", 1);
    
        // Register statistics
        stat_eventStalledForLock =      registerStatistic<uint64_t>("EventStalledForLockedCacheline");
        stat_evict_S =                  registerStatistic<uint64_t>("evict_S");
        stat_evict_SM =                 registerStatistic<uint64_t>("evict_SM");
        stat_stateEvent_GetS_I =        registerStatistic<uint64_t>("stateEvent_GetS_I");
        stat_stateEvent_GetS_S =        registerStatistic<uint64_t>("stateEvent_GetS_S");
        stat_stateEvent_GetS_M =        registerStatistic<uint64_t>("stateEvent_GetS_M");
        stat_stateEvent_GetX_I =        registerStatistic<uint64_t>("stateEvent_GetX_I");
        stat_stateEvent_GetX_S =        registerStatistic<uint64_t>("stateEvent_GetX_S");
        stat_stateEvent_GetX_M =        registerStatistic<uint64_t>("stateEvent_GetX_M");
        stat_stateEvent_GetSX_I =      registerStatistic<uint64_t>("stateEvent_GetSX_I");
        stat_stateEvent_GetSX_S =      registerStatistic<uint64_t>("stateEvent_GetSX_S");
        stat_stateEvent_GetSX_M =      registerStatistic<uint64_t>("stateEvent_GetSX_M");
        stat_stateEvent_GetSResp_IS =   registerStatistic<uint64_t>("stateEvent_GetSResp_IS");
        stat_stateEvent_GetXResp_IS =   registerStatistic<uint64_t>("stateEvent_GetXResp_IS");
        stat_stateEvent_GetXResp_IM =   registerStatistic<uint64_t>("stateEvent_GetXResp_IM");
        stat_stateEvent_GetXResp_SM =   registerStatistic<uint64_t>("stateEvent_GetXResp_SM");
        stat_stateEvent_Inv_I =         registerStatistic<uint64_t>("stateEvent_Inv_I");
        stat_stateEvent_Inv_S =         registerStatistic<uint64_t>("stateEvent_Inv_S");
        stat_stateEvent_Inv_IS =        registerStatistic<uint64_t>("stateEvent_Inv_IS");
        stat_stateEvent_Inv_IM =        registerStatistic<uint64_t>("stateEvent_Inv_IM");
        stat_stateEvent_Inv_SM =        registerStatistic<uint64_t>("stateEvent_Inv_SM");
        stat_stateEvent_Inv_SB =        registerStatistic<uint64_t>("stateEvent_Inv_SB");
        stat_stateEvent_Inv_IB =        registerStatistic<uint64_t>("stateEvent_Inv_IB");
        stat_stateEvent_FetchInvX_I =   registerStatistic<uint64_t>("stateEvent_FetchInvX_I");
        stat_stateEvent_FetchInvX_M =   registerStatistic<uint64_t>("stateEvent_FetchInvX_M");
        stat_stateEvent_FetchInvX_IS =  registerStatistic<uint64_t>("stateEvent_FetchInvX_IS");
        stat_stateEvent_FetchInvX_IM =  registerStatistic<uint64_t>("stateEvent_FetchInvX_IM");
        stat_stateEvent_FetchInvX_SB =  registerStatistic<uint64_t>("stateEvent_FetchInvX_SB");
        stat_stateEvent_FetchInvX_IB =  registerStatistic<uint64_t>("stateEvent_FetchInvX_IB");
        stat_stateEvent_Fetch_I =       registerStatistic<uint64_t>("stateEvent_Fetch_I");
        stat_stateEvent_Fetch_S =       registerStatistic<uint64_t>("stateEvent_Fetch_S");
        stat_stateEvent_Fetch_IS =      registerStatistic<uint64_t>("stateEvent_Fetch_IS");
        stat_stateEvent_Fetch_IM =      registerStatistic<uint64_t>("stateEvent_Fetch_IM");
        stat_stateEvent_Fetch_SM =      registerStatistic<uint64_t>("stateEvent_Fetch_SM");
        stat_stateEvent_Fetch_IB =      registerStatistic<uint64_t>("stateEvent_Fetch_IB");
        stat_stateEvent_Fetch_SB =      registerStatistic<uint64_t>("stateEvent_Fetch_SB");
        stat_stateEvent_FetchInv_I =    registerStatistic<uint64_t>("stateEvent_FetchInv_I");
        stat_stateEvent_FetchInv_S =    registerStatistic<uint64_t>("stateEvent_FetchInv_S");
        stat_stateEvent_FetchInv_M =    registerStatistic<uint64_t>("stateEvent_FetchInv_M");
        stat_stateEvent_FetchInv_IS =   registerStatistic<uint64_t>("stateEvent_FetchInv_IS");
        stat_stateEvent_FetchInv_IM =   registerStatistic<uint64_t>("stateEvent_FetchInv_IM");
        stat_stateEvent_FetchInv_SM =   registerStatistic<uint64_t>("stateEvent_FetchInv_SM");
        stat_stateEvent_FetchInv_SB =   registerStatistic<uint64_t>("stateEvent_FetchInv_SB");
        stat_stateEvent_FetchInv_IB =   registerStatistic<uint64_t>("stateEvent_FetchInv_IB");
        stat_stateEvent_FlushLine_I =   registerStatistic<uint64_t>("stateEvent_FlushLine_I");
        stat_stateEvent_FlushLine_S =   registerStatistic<uint64_t>("stateEvent_FlushLine_S");
        stat_stateEvent_FlushLine_M =   registerStatistic<uint64_t>("stateEvent_FlushLine_M");
        stat_stateEvent_FlushLine_IS =  registerStatistic<uint64_t>("stateEvent_FlushLine_IS");
        stat_stateEvent_FlushLine_IM =  registerStatistic<uint64_t>("stateEvent_FlushLine_IM");
        stat_stateEvent_FlushLine_SM =  registerStatistic<uint64_t>("stateEvent_FlushLine_SM");
        stat_stateEvent_FlushLine_IB =  registerStatistic<uint64_t>("stateEvent_FlushLine_IB");
        stat_stateEvent_FlushLine_SB =  registerStatistic<uint64_t>("stateEvent_FlushLine_SB");
        stat_stateEvent_FlushLineInv_I =    registerStatistic<uint64_t>("stateEvent_FlushLineInv_I");
        stat_stateEvent_FlushLineInv_S =    registerStatistic<uint64_t>("stateEvent_FlushLineInv_S");
        stat_stateEvent_FlushLineInv_M =    registerStatistic<uint64_t>("stateEvent_FlushLineInv_M");
        stat_stateEvent_FlushLineInv_IS =   registerStatistic<uint64_t>("stateEvent_FlushLineInv_IS");
        stat_stateEvent_FlushLineInv_IM =   registerStatistic<uint64_t>("stateEvent_FlushLineInv_IM");
        stat_stateEvent_FlushLineInv_SM =   registerStatistic<uint64_t>("stateEvent_FlushLineInv_SM");
        stat_stateEvent_FlushLineInv_IB =   registerStatistic<uint64_t>("stateEvent_FlushLineInv_IB");
        stat_stateEvent_FlushLineInv_SB =   registerStatistic<uint64_t>("stateEvent_FlushLineInv_SB");
        stat_stateEvent_FlushLineResp_I =   registerStatistic<uint64_t>("stateEvent_FlushLineResp_I");
        stat_stateEvent_FlushLineResp_IB =  registerStatistic<uint64_t>("stateEvent_FlushLineResp_IB");
        stat_stateEvent_FlushLineResp_SB =  registerStatistic<uint64_t>("stateEvent_FlushLineResp_SB");
        stat_eventSent_GetS =           registerStatistic<uint64_t>("eventSent_GetS");
        stat_eventSent_GetX =           registerStatistic<uint64_t>("eventSent_GetX");
        stat_eventSent_GetSX =         registerStatistic<uint64_t>("eventSent_GetSX");
        stat_eventSent_PutM =           registerStatistic<uint64_t>("eventSent_PutM");
        stat_eventSent_NACK_down =      registerStatistic<uint64_t>("eventSent_NACK_down");
        stat_eventSent_FlushLine =      registerStatistic<uint64_t>("eventSent_FlushLine");
        stat_eventSent_FlushLineInv =   registerStatistic<uint64_t>("eventSent_FlushLineInv");
        stat_eventSent_FetchResp =      registerStatistic<uint64_t>("eventSent_FetchResp");
        stat_eventSent_FetchXResp =     registerStatistic<uint64_t>("eventSent_FetchXResp");
        stat_eventSent_AckInv =         registerStatistic<uint64_t>("eventSent_AckInv");
        stat_eventSent_GetSResp =       registerStatistic<uint64_t>("eventSent_GetSResp");
        stat_eventSent_GetXResp =       registerStatistic<uint64_t>("eventSent_GetXResp");
        stat_eventSent_FlushLineResp =  registerStatistic<uint64_t>("eventSent_FlushLineResp");
   
        /* Only for caches that expect writeback acks but don't know yet and can't register statistics later. Always enabled for now. */
        stat_stateEvent_AckPut_I =      registerStatistic<uint64_t>("stateEvent_AckPut_I");

        /* Only for caches that don't silently drop clean blocks but don't know yet and can't register statistics later. Always enabled for now. */
        stat_eventSent_PutS =           registerStatistic<uint64_t>("eventSent_PutS");
        stat_eventSent_PutE =           registerStatistic<uint64_t>("eventSent_PutE");

        // Only for caches that forward invs to the processor
        if (snoopL1Invs_) {
            stat_eventSent_Inv =            registerStatistic<uint64_t>("eventSent_Inv");
        }

        /* MESI-specific statistics (as opposed to MSI) */
        if (protocol_) {
            stat_stateEvent_GetS_E =        registerStatistic<uint64_t>("stateEvent_GetS_E");
            stat_stateEvent_GetX_E =        registerStatistic<uint64_t>("stateEvent_GetX_E");
            stat_stateEvent_GetSX_E =      registerStatistic<uint64_t>("stateEvent_GetSX_E");
            stat_stateEvent_FlushLine_E =   registerStatistic<uint64_t>("stateEvent_FlushLine_E");
            stat_stateEvent_FlushLineInv_E =    registerStatistic<uint64_t>("stateEvent_FlushLineInv_E");
            stat_stateEvent_FetchInv_E =    registerStatistic<uint64_t>("stateEvent_FetchInv_E");
            stat_stateEvent_FetchInvX_E =   registerStatistic<uint64_t>("stateEvent_FetchInvX_E");
        }
    }

    ~L1CoherenceController() {}
    
    /** Used to determine in advance if an event will be a miss (and which kind of miss)
     * Used for statistics only
     */
    int isCoherenceMiss(MemEvent* event, CacheLine* cacheLine);
    
    /* Event handlers called by cache controller */
    /** Send cache line data to the lower level caches */
    CacheAction handleEviction(CacheLine* wbCacheLine, string origRqstr, bool ignoredParam=false);

    /** Process new cache request:  GetX, GetS, GetSX */
    CacheAction handleRequest(MemEvent* event, CacheLine* cacheLine, bool replay);
   
    /** Process replacement - implemented for compatibility with CoherenceController but L1s do not receive replacements */
    CacheAction handleReplacement(MemEvent* event, CacheLine* cacheLine, MemEvent * origRequest, bool replay);
    
    /** Process Inv */
    CacheAction handleInvalidationRequest(MemEvent *event, CacheLine* cacheLine, MemEvent * collisionEvent, bool replay);

    /** Process responses */
    CacheAction handleResponse(MemEvent* responseEvent, CacheLine* cacheLine, MemEvent* origRequest);


    /* Methods for sending events, called by cache controller */
    /** Send response up (to processor) */
    uint64_t sendResponseUp(MemEvent * event, vector<uint8_t>* data, bool replay, uint64_t baseTime, bool atomic = false);
    
    /** Call through to coherenceController with statistic recording */
    void addToOutgoingQueue(Response& resp);
    void addToOutgoingQueueUp(Response& resp);

/* Miscellaneous */
    /** Determine whether a retry of a NACKed event is needed */
    bool isRetryNeeded(MemEvent * event, CacheLine * cacheLine);

    void printData(vector<uint8_t> * data, bool set);

private:
    bool                protocol_;  // True for MESI, false for MSI
    bool                snoopL1Invs_;

    /* Statistics */
    Statistic<uint64_t>* stat_eventStalledForLock;
    Statistic<uint64_t>* stat_evict_S;
    Statistic<uint64_t>* stat_evict_SM;
    Statistic<uint64_t>* stat_stateEvent_GetS_I;
    Statistic<uint64_t>* stat_stateEvent_GetS_S;
    Statistic<uint64_t>* stat_stateEvent_GetS_E;
    Statistic<uint64_t>* stat_stateEvent_GetS_M;
    Statistic<uint64_t>* stat_stateEvent_GetX_I;
    Statistic<uint64_t>* stat_stateEvent_GetX_S;
    Statistic<uint64_t>* stat_stateEvent_GetX_E;
    Statistic<uint64_t>* stat_stateEvent_GetX_M;
    Statistic<uint64_t>* stat_stateEvent_GetSX_I;
    Statistic<uint64_t>* stat_stateEvent_GetSX_S;
    Statistic<uint64_t>* stat_stateEvent_GetSX_E;
    Statistic<uint64_t>* stat_stateEvent_GetSX_M;
    Statistic<uint64_t>* stat_stateEvent_GetSResp_IS;
    Statistic<uint64_t>* stat_stateEvent_GetXResp_IS;
    Statistic<uint64_t>* stat_stateEvent_GetXResp_IM;
    Statistic<uint64_t>* stat_stateEvent_GetXResp_SM;
    Statistic<uint64_t>* stat_stateEvent_Inv_I;
    Statistic<uint64_t>* stat_stateEvent_Inv_S;
    Statistic<uint64_t>* stat_stateEvent_Inv_IS;
    Statistic<uint64_t>* stat_stateEvent_Inv_IM;
    Statistic<uint64_t>* stat_stateEvent_Inv_SM;
    Statistic<uint64_t>* stat_stateEvent_Inv_SB;
    Statistic<uint64_t>* stat_stateEvent_Inv_IB;
    Statistic<uint64_t>* stat_stateEvent_FetchInvX_I;
    Statistic<uint64_t>* stat_stateEvent_FetchInvX_E;
    Statistic<uint64_t>* stat_stateEvent_FetchInvX_M;
    Statistic<uint64_t>* stat_stateEvent_FetchInvX_IS;
    Statistic<uint64_t>* stat_stateEvent_FetchInvX_IM;
    Statistic<uint64_t>* stat_stateEvent_FetchInvX_SB;
    Statistic<uint64_t>* stat_stateEvent_FetchInvX_IB;
    Statistic<uint64_t>* stat_stateEvent_Fetch_I;
    Statistic<uint64_t>* stat_stateEvent_Fetch_S;
    Statistic<uint64_t>* stat_stateEvent_Fetch_IS;
    Statistic<uint64_t>* stat_stateEvent_Fetch_IM;
    Statistic<uint64_t>* stat_stateEvent_Fetch_SM;
    Statistic<uint64_t>* stat_stateEvent_Fetch_IB;
    Statistic<uint64_t>* stat_stateEvent_Fetch_SB;
    Statistic<uint64_t>* stat_stateEvent_FetchInv_I;
    Statistic<uint64_t>* stat_stateEvent_FetchInv_S;
    Statistic<uint64_t>* stat_stateEvent_FetchInv_E;
    Statistic<uint64_t>* stat_stateEvent_FetchInv_M;
    Statistic<uint64_t>* stat_stateEvent_FetchInv_IS;
    Statistic<uint64_t>* stat_stateEvent_FetchInv_IM;
    Statistic<uint64_t>* stat_stateEvent_FetchInv_SM;
    Statistic<uint64_t>* stat_stateEvent_FetchInv_SB;
    Statistic<uint64_t>* stat_stateEvent_FetchInv_IB;
    Statistic<uint64_t>* stat_stateEvent_AckPut_I;
    Statistic<uint64_t>* stat_stateEvent_FlushLine_I;
    Statistic<uint64_t>* stat_stateEvent_FlushLine_S;
    Statistic<uint64_t>* stat_stateEvent_FlushLine_E;
    Statistic<uint64_t>* stat_stateEvent_FlushLine_M;
    Statistic<uint64_t>* stat_stateEvent_FlushLine_IS;
    Statistic<uint64_t>* stat_stateEvent_FlushLine_IM;
    Statistic<uint64_t>* stat_stateEvent_FlushLine_SM;
    Statistic<uint64_t>* stat_stateEvent_FlushLine_IB;
    Statistic<uint64_t>* stat_stateEvent_FlushLine_SB;
    Statistic<uint64_t>* stat_stateEvent_FlushLineInv_I;
    Statistic<uint64_t>* stat_stateEvent_FlushLineInv_S;
    Statistic<uint64_t>* stat_stateEvent_FlushLineInv_E;
    Statistic<uint64_t>* stat_stateEvent_FlushLineInv_M;
    Statistic<uint64_t>* stat_stateEvent_FlushLineInv_IS;
    Statistic<uint64_t>* stat_stateEvent_FlushLineInv_IM;
    Statistic<uint64_t>* stat_stateEvent_FlushLineInv_SM;
    Statistic<uint64_t>* stat_stateEvent_FlushLineInv_IB;
    Statistic<uint64_t>* stat_stateEvent_FlushLineInv_SB;
    Statistic<uint64_t>* stat_stateEvent_FlushLineResp_I;
    Statistic<uint64_t>* stat_stateEvent_FlushLineResp_IB;
    Statistic<uint64_t>* stat_stateEvent_FlushLineResp_SB;
    Statistic<uint64_t>* stat_eventSent_GetS;
    Statistic<uint64_t>* stat_eventSent_GetX;
    Statistic<uint64_t>* stat_eventSent_GetSX;
    Statistic<uint64_t>* stat_eventSent_PutS;
    Statistic<uint64_t>* stat_eventSent_PutE;
    Statistic<uint64_t>* stat_eventSent_PutM;
    Statistic<uint64_t>* stat_eventSent_NACK_down;
    Statistic<uint64_t>* stat_eventSent_FlushLine;
    Statistic<uint64_t>* stat_eventSent_FlushLineInv;
    Statistic<uint64_t>* stat_eventSent_FetchResp;
    Statistic<uint64_t>* stat_eventSent_FetchXResp;
    Statistic<uint64_t>* stat_eventSent_AckInv;
    Statistic<uint64_t>* stat_eventSent_GetSResp;
    Statistic<uint64_t>* stat_eventSent_GetXResp;
    Statistic<uint64_t>* stat_eventSent_FlushLineResp;
    Statistic<uint64_t>* stat_eventSent_Inv;

    /* Private event handlers */
    /** Handle GetX request. Request upgrade if needed */
    CacheAction handleGetXRequest(MemEvent* event, CacheLine* cacheLine, bool replay);
    
    /** Handle GetS request. Request block if needed */
    CacheAction handleGetSRequest(MemEvent* event, CacheLine* cacheLine, bool replay);
    
    /** Handle FlushLine request. */
    CacheAction handleFlushLineRequest(MemEvent *event, CacheLine* cacheLine, MemEvent* reqEvent, bool replay);
    
    /** Handle FlushLineInv request */
    CacheAction handleFlushLineInvRequest(MemEvent *event, CacheLine* cacheLine, MemEvent* reqEvent, bool replay);

    /** Handle Inv request */
    CacheAction handleInv(MemEvent * event, CacheLine * cacheLine, bool replay);
    
    /** Handle ForceInv request */
    CacheAction handleForceInv(MemEvent * event, CacheLine * cacheLine, bool replay);
    
    /** Handle Fetch */
    CacheAction handleFetch(MemEvent * event, CacheLine * cacheLine, bool replay);
    
    /** Handle FetchInv */
    CacheAction handleFetchInv(MemEvent * event, CacheLine * cacheLine, bool replay);
    
    /** Handle FetchInvX */
    CacheAction handleFetchInvX(MemEvent * event, CacheLine * cacheLine, bool replay);

    /** Handle data response - GetSResp or GetXResp */
    void handleDataResponse(MemEvent* responseEvent, CacheLine * cacheLine, MemEvent * reqEvent);

    
    /* Methods for sending events */
    /** Send response memEvent to lower level caches */
    void sendResponseDown(MemEvent* event, CacheLine* cacheLine, bool replay);

    /** Send writeback request to lower level caches */
    void sendWriteback(Command cmd, CacheLine* cacheLine, bool dirty, string origRqstr);

    /** Send AckInv response to lower level caches */
    void sendAckInv(MemEvent * request, CacheLine * cacheLine);

    /** Forward a flush line request, with or without data */
    void forwardFlushLine(Addr baseAddr, Command cmd, string origRqstr, CacheLine * cacheLine);
    
    /** Send response to a flush request */
    void sendFlushResponse(MemEvent * requestEvent, bool success, uint64_t baseTime, bool replay);

    /* Methods for recording statistics */
    void recordEvictionState(State state);
    void recordStateEventCount(Command cmd, State state);
    void recordEventSentDown(Command cmd);
    void recordEventSentUp(Command cmd);
};


}}


#endif

