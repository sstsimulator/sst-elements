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

#ifndef INCOHERENTCONTROLLER_H
#define INCOHERENTCONTROLLER_H

#include <iostream>
#include "coherenceController.h"


namespace SST { namespace MemHierarchy {

class IncoherentController : public CoherenceController{
public:
    /** Constructor for IncoherentController. */
    IncoherentController(SST::Component* comp, Params& params) : CoherenceController (comp, params) {
        debug->debug(_INFO_,"--------------------------- Initializing [Incoherent Controller] ... \n\n");
        inclusive_ = params.find<bool>("inclusive", true);
    
        /* Register statistics */
        stat_stateEvent_GetS_I = registerStatistic<uint64_t>("stateEvent_GetS_I");
        stat_stateEvent_GetS_E = registerStatistic<uint64_t>("stateEvent_GetS_E");
        stat_stateEvent_GetS_M = registerStatistic<uint64_t>("stateEvent_GetS_M");
        stat_stateEvent_GetX_I = registerStatistic<uint64_t>("stateEvent_GetX_I");
        stat_stateEvent_GetX_E = registerStatistic<uint64_t>("stateEvent_GetX_E");
        stat_stateEvent_GetX_M = registerStatistic<uint64_t>("stateEvent_GetX_M");
        stat_stateEvent_GetSEx_I = registerStatistic<uint64_t>("stateEvent_GetSEx_I");
        stat_stateEvent_GetSEx_E = registerStatistic<uint64_t>("stateEvent_GetSEx_E");
        stat_stateEvent_GetSEx_M = registerStatistic<uint64_t>("stateEvent_GetSEx_M");
        stat_stateEvent_GetSResp_IS = registerStatistic<uint64_t>("stateEvent_GetSResp_IS");
        stat_stateEvent_GetSResp_IM = registerStatistic<uint64_t>("stateEvent_GetSResp_IM");
        stat_stateEvent_PutE_I = registerStatistic<uint64_t>("stateEvent_PutE_I");
        stat_stateEvent_PutE_E = registerStatistic<uint64_t>("stateEvent_PutE_E");
        stat_stateEvent_PutE_M = registerStatistic<uint64_t>("stateEvent_PutE_M");
        stat_stateEvent_PutE_IS = registerStatistic<uint64_t>("stateEvent_PutE_IS");
        stat_stateEvent_PutE_IM = registerStatistic<uint64_t>("stateEvent_PutE_IM");
        stat_stateEvent_PutE_IB = registerStatistic<uint64_t>("stateEvent_PutE_IB");
        stat_stateEvent_PutE_SB = registerStatistic<uint64_t>("stateEvent_PutE_SB");
        stat_stateEvent_PutM_I = registerStatistic<uint64_t>("stateEvent_PutM_I");
        stat_stateEvent_PutM_E = registerStatistic<uint64_t>("stateEvent_PutM_E");
        stat_stateEvent_PutM_M = registerStatistic<uint64_t>("stateEvent_PutM_M");
        stat_stateEvent_PutM_IS = registerStatistic<uint64_t>("stateEvent_PutM_IS");
        stat_stateEvent_PutM_IM = registerStatistic<uint64_t>("stateEvent_PutM_IM");
        stat_stateEvent_PutM_IB = registerStatistic<uint64_t>("stateEvent_PutM_IB");
        stat_stateEvent_PutM_SB = registerStatistic<uint64_t>("stateEvent_PutM_SB");
        stat_stateEvent_FlushLine_I = registerStatistic<uint64_t>("stateEvent_FlushLine_I");
        stat_stateEvent_FlushLine_E = registerStatistic<uint64_t>("stateEvent_FlushLine_E");
        stat_stateEvent_FlushLine_M = registerStatistic<uint64_t>("stateEvent_FlushLine_M");
        stat_stateEvent_FlushLine_IS = registerStatistic<uint64_t>("stateEvent_FlushLine_IS");
        stat_stateEvent_FlushLine_IM = registerStatistic<uint64_t>("stateEvent_FlushLine_IM");
        stat_stateEvent_FlushLine_IB = registerStatistic<uint64_t>("stateEvent_FlushLine_IB");
        stat_stateEvent_FlushLine_SB = registerStatistic<uint64_t>("stateEvent_FlushLine_SB");
        stat_stateEvent_FlushLineInv_I = registerStatistic<uint64_t>("stateEvent_FlushLineInv_I");
        stat_stateEvent_FlushLineInv_E = registerStatistic<uint64_t>("stateEvent_FlushLineInv_E");
        stat_stateEvent_FlushLineInv_M = registerStatistic<uint64_t>("stateEvent_FlushLineInv_M");
        stat_stateEvent_FlushLineInv_IS = registerStatistic<uint64_t>("stateEvent_FlushLineInv_IS");
        stat_stateEvent_FlushLineInv_IM = registerStatistic<uint64_t>("stateEvent_FlushLineInv_IM");
        stat_stateEvent_FlushLineInv_IB = registerStatistic<uint64_t>("stateEvent_FlushLineInv_IB");
        stat_stateEvent_FlushLineInv_SB = registerStatistic<uint64_t>("stateEvent_FlushLineInv_SB");
        stat_stateEvent_FlushLineResp_I = registerStatistic<uint64_t>("stateEvent_FlushLineResp_I");
        stat_stateEvent_FlushLineResp_IB = registerStatistic<uint64_t>("stateEvent_FlushLineResp_IB");
        stat_stateEvent_FlushLineResp_SB = registerStatistic<uint64_t>("stateEvent_FlushLineResp_SB");
        stat_eventSent_GetS             = registerStatistic<uint64_t>("eventSent_GetS");
        stat_eventSent_GetX             = registerStatistic<uint64_t>("eventSent_GetX");
        stat_eventSent_GetSEx           = registerStatistic<uint64_t>("eventSent_GetSEx");
        stat_eventSent_PutE             = registerStatistic<uint64_t>("eventSent_PutE");
        stat_eventSent_PutM             = registerStatistic<uint64_t>("eventSent_PutM");
        stat_eventSent_FlushLine        = registerStatistic<uint64_t>("eventSent_FlushLine");
        stat_eventSent_FlushLineInv     = registerStatistic<uint64_t>("eventSent_FlushLineInv");
        stat_eventSent_NACK_down        = registerStatistic<uint64_t>("eventSent_NACK_down");
        stat_eventSent_GetSResp         = registerStatistic<uint64_t>("eventSent_GetSResp");
        stat_eventSent_GetXResp         = registerStatistic<uint64_t>("eventSent_GetXResp");
        stat_eventSent_FlushLineResp    = registerStatistic<uint64_t>("eventSent_FlushLineResp");
        stat_eventSent_NACK_up          = registerStatistic<uint64_t>("eventSent_NACK_up");
    }

    ~IncoherentController() {}
    
 
/*----------------------------------------------------------------------------------------------------------------------
 *  Public functions form external interface to the coherence controller
 *---------------------------------------------------------------------------------------------------------------------*/    

/* Event handlers */
    /* Public event handlers called by cache controller */
    /** Send cache line data to the lower level caches */
    CacheAction handleEviction(CacheLine* _wbCacheLine, string _origRqstr, bool ignoredParameter=false);

    /** Process cache request:  GetX, GetS, GetSEx */
    CacheAction handleRequest(MemEvent* event, CacheLine* cacheLine, bool replay);
    
    /** Process replacement request - PutS, PutE, PutM */
    CacheAction handleReplacement(MemEvent* event, CacheLine* cacheLine, MemEvent * reqEvent, bool replay);
    
    /** Process invalidation requests - Inv, FetchInv, FetchInvX */
    CacheAction handleInvalidationRequest(MemEvent *event, CacheLine* cacheLine, MemEvent * collisonEvent, bool replay);

    /** Process responses - GetSResp, GetXResp, FetchResp */
    CacheAction handleResponse(MemEvent* _responseEvent, CacheLine* cacheLine, MemEvent* _origRequest);

/* Miscellaneous */

    /** Determine in advance if a request will miss (and what kind of miss). Used for stats */
    int isCoherenceMiss(MemEvent* event, CacheLine* cacheLine);
    
    /** Determine whether a retry of a NACKed event is needed */
    bool isRetryNeeded(MemEvent* event, CacheLine* cacheLine);
   
    /** Message send: Call through to coherenceController with statistic recording */
    void addToOutgoingQueue(Response& resp);
    void addToOutgoingQueueUp(Response& resp);

private:
/* Private data members */
    bool                inclusive_;
    
/* Statistics */
    Statistic<uint64_t>* stat_stateEvent_GetS_I;
    Statistic<uint64_t>* stat_stateEvent_GetS_E;
    Statistic<uint64_t>* stat_stateEvent_GetS_M;
    Statistic<uint64_t>* stat_stateEvent_GetX_I;
    Statistic<uint64_t>* stat_stateEvent_GetX_E;
    Statistic<uint64_t>* stat_stateEvent_GetX_M;
    Statistic<uint64_t>* stat_stateEvent_GetSEx_I;
    Statistic<uint64_t>* stat_stateEvent_GetSEx_E;
    Statistic<uint64_t>* stat_stateEvent_GetSEx_M;
    Statistic<uint64_t>* stat_stateEvent_GetSResp_IS;
    Statistic<uint64_t>* stat_stateEvent_GetSResp_IM;
    Statistic<uint64_t>* stat_stateEvent_PutE_I;
    Statistic<uint64_t>* stat_stateEvent_PutE_E;
    Statistic<uint64_t>* stat_stateEvent_PutE_M;
    Statistic<uint64_t>* stat_stateEvent_PutE_IS;
    Statistic<uint64_t>* stat_stateEvent_PutE_IM;
    Statistic<uint64_t>* stat_stateEvent_PutE_IB;
    Statistic<uint64_t>* stat_stateEvent_PutE_SB;
    Statistic<uint64_t>* stat_stateEvent_PutM_I;
    Statistic<uint64_t>* stat_stateEvent_PutM_E;
    Statistic<uint64_t>* stat_stateEvent_PutM_M;
    Statistic<uint64_t>* stat_stateEvent_PutM_IS;
    Statistic<uint64_t>* stat_stateEvent_PutM_IM;
    Statistic<uint64_t>* stat_stateEvent_PutM_IB;
    Statistic<uint64_t>* stat_stateEvent_PutM_SB;
    Statistic<uint64_t>* stat_stateEvent_FlushLine_I;
    Statistic<uint64_t>* stat_stateEvent_FlushLine_E;
    Statistic<uint64_t>* stat_stateEvent_FlushLine_M;
    Statistic<uint64_t>* stat_stateEvent_FlushLine_IS;
    Statistic<uint64_t>* stat_stateEvent_FlushLine_IM;
    Statistic<uint64_t>* stat_stateEvent_FlushLine_IB;
    Statistic<uint64_t>* stat_stateEvent_FlushLine_SB;
    Statistic<uint64_t>* stat_stateEvent_FlushLineInv_I;
    Statistic<uint64_t>* stat_stateEvent_FlushLineInv_E;
    Statistic<uint64_t>* stat_stateEvent_FlushLineInv_M;
    Statistic<uint64_t>* stat_stateEvent_FlushLineInv_IS;
    Statistic<uint64_t>* stat_stateEvent_FlushLineInv_IM;
    Statistic<uint64_t>* stat_stateEvent_FlushLineInv_IB;
    Statistic<uint64_t>* stat_stateEvent_FlushLineInv_SB;
    Statistic<uint64_t>* stat_stateEvent_FlushLineResp_I;
    Statistic<uint64_t>* stat_stateEvent_FlushLineResp_IB;
    Statistic<uint64_t>* stat_stateEvent_FlushLineResp_SB;
    Statistic<uint64_t>* stat_eventSent_GetS;
    Statistic<uint64_t>* stat_eventSent_GetX;
    Statistic<uint64_t>* stat_eventSent_GetSEx;
    Statistic<uint64_t>* stat_eventSent_PutE;
    Statistic<uint64_t>* stat_eventSent_PutM;
    Statistic<uint64_t>* stat_eventSent_FlushLine;
    Statistic<uint64_t>* stat_eventSent_FlushLineInv;
    Statistic<uint64_t>* stat_eventSent_NACK_down;
    Statistic<uint64_t>* stat_eventSent_GetSResp;
    Statistic<uint64_t>* stat_eventSent_GetXResp;
    Statistic<uint64_t>* stat_eventSent_FlushLineResp;
    Statistic<uint64_t>* stat_eventSent_NACK_up;
    
/* Private event handlers */
    /** Handle GetX request. Request upgrade if needed */
    CacheAction handleGetXRequest(MemEvent* event, CacheLine* _cacheLine, bool replay);

    /** Handle GetS request. Request block if needed */
    CacheAction handleGetSRequest(MemEvent* event, CacheLine* cacheLine, bool replay);
    
    /** Handle PutM request. Write data to cache line.  Update E->M state if necessary */
    CacheAction handlePutMRequest(MemEvent* event, CacheLine* cacheLine);
    
    /** Handle FlushLine request. Forward if needed */
    CacheAction handleFlushLineRequest(MemEvent * event, CacheLine * cacheLine, MemEvent * reqEvent, bool replay);

    /** Handle FlushLineInv request. Forward if needed */
    CacheAction handleFlushLineInvRequest(MemEvent * event, CacheLine * cacheLine, MemEvent * reqEvent, bool replay);

    /** Process GetSResp/GetXResp.  Update the cache line */
    CacheAction handleDataResponse(MemEvent* responseEvent, CacheLine * cacheLine, MemEvent * reqEvent);
    
/* Private methods for sending events */
    /** Send writeback request to lower level caches */
    void sendWriteback(Command cmd, CacheLine* cacheLine, string origRqstr);
    
    /** Send a flush response */
    void sendFlushResponse(MemEvent * reqEent, bool success);
    
    /** Forward a FlushLine request with or without data */
    void forwardFlushLine(Addr baseAddr, string origRqstr, CacheLine * cacheLine, Command cmd);

/* Helper methods */
   
    void printData(vector<uint8_t> * data, bool set);

/* Statistics */
    void recordStateEventCount(Command cmd, State state);
    void recordEventSentDown(Command cmd);
    void recordEventSentUp(Command cmd);
};


}}


#endif

