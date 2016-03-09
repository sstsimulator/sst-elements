// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef MESICOHERENCECONTROLLER_H
#define MESICOHERENCECONTROLLER_H

#include <iostream>
#include "coherenceControllers.h"


namespace SST { namespace MemHierarchy {

class MESIController : public CoherencyController{
public:
    /** Constructor for MESIController. Note that MESIController handles both MESI & MSI protocols */
    MESIController(const Cache* cache, string ownerName, Output* dbg, vector<Link*>* parentLinks, Link* childLink, CacheListener* listener, 
            unsigned int lineSize, uint64 accessLatency, uint64 tagLatency, uint64 mshrLatency, bool LLC, bool LL, MSHR * mshr, bool protocol, bool inclusive, bool wbClean,
            MemNIC* bottomNetworkLink, MemNIC* topNetworkLink, bool debugAll, Addr debugAddr) :
                 CoherencyController(cache, dbg, ownerName, lineSize, accessLatency, tagLatency, mshrLatency, LLC, LL, parentLinks, childLink, bottomNetworkLink, topNetworkLink, 
                         listener, mshr, wbClean, debugAll, debugAddr) {
        d_->debug(_INFO_,"--------------------------- Initializing [MESI Controller] ... \n\n");
        protocol_           = protocol;         // 1 for MESI, 0 for MSI
        inclusive_          = inclusive;
        
    }

    ~MESIController() {}
    
    /** Init funciton */
    void init(const char* name){}


/*----------------------------------------------------------------------------------------------------------------------
 *  Public functions form external interface to the coherence controller
 *---------------------------------------------------------------------------------------------------------------------*/  

/* Event handlers */
    /** Send cacheline data to the lower level caches */
    CacheAction handleEviction(CacheLine* wbCacheLine, string origRqstr, bool ignoredParam=false);

    /** Process cache request:  GetX, GetS, GetSEx */
    CacheAction handleRequest(MemEvent* event, CacheLine* cacheLine, bool replay);
    
    /** Process replacement request - PutS, PutE, PutM */
    CacheAction handleReplacement(MemEvent* event, CacheLine* cacheLine, MemEvent * reqEvent, bool replay);
    
    /** Process invalidation requests - Inv, FetchInv, FetchInvX */
    CacheAction handleInvalidationRequest(MemEvent *event, CacheLine* cacheLine, bool replay);

    /** Process responses - GetSResp, GetXResp, FetchResp */
    CacheAction handleResponse(MemEvent* responseEvent, CacheLine* cacheLine, MemEvent* origRequest);

/* Message send */
    /** Forward a message up, used for non-inclusive caches */
    void forwardMessageUp(MemEvent * event);

/* Miscellaneous */
    /** Determine in advance if a request will miss (and what kind of miss). Used for stats */
    int isCoherenceMiss(MemEvent* event, CacheLine* cacheLine);
    
    /** Determine whether a NACKed event should be retried */
    bool isRetryNeeded(MemEvent * event, CacheLine * cacheLine);
   
private:
/* Private data members */
    bool                protocol_;  // True for MESI, false for MSI
    bool                inclusive_;

    
/* Private event handlers */
    /** Handle GetX request. Request upgrade if needed */
    CacheAction handleGetXRequest(MemEvent* event, CacheLine* cacheLine, bool replay);

    /** Handle GetS request. Request block if needed */
    CacheAction handleGetSRequest(MemEvent* event, CacheLine* cacheLine, bool replay);
    
    /** Handle PutM request. Write data to cache line.  Update E->M state if necessary */
    CacheAction handlePutMRequest(MemEvent* event, CacheLine* cacheLine, MemEvent * reqEvent);
    
    /** Handle PutS requesst. Update sharer state */ 
    CacheAction handlePutSRequest(MemEvent* event, CacheLine* cacheLine, MemEvent * reqEvent);
    
    /** Handle Inv */
    CacheAction handleInv(MemEvent * event, CacheLine * cacheLine, bool replay);
    
    /** Handle Fetch */
    CacheAction handleFetch(MemEvent * event, CacheLine * cacheLine, bool replay);
    
    /** Handle FetchInv */
    CacheAction handleFetchInv(MemEvent * event, CacheLine * cacheLine, bool replay);
    
    /** Handle FetchInvX */
    CacheAction handleFetchInvX(MemEvent * event, CacheLine * cacheLine, bool replay);

    /** Process GetSResp/GetXResp.  Update the cache line */
    CacheAction handleDataResponse(MemEvent* responseEvent, CacheLine * cacheLine, MemEvent * reqEvent);
    
    /** Handle FetchResp */
    CacheAction handleFetchResp(MemEvent * responseEvent, CacheLine* cacheLine, MemEvent * reqEvent);
    
    /** Handle Ack */
    CacheAction handleAckInv(MemEvent * responseEvent, CacheLine* cacheLine, MemEvent * reqEvent);

/* Private methods for sending events */
    /** Send response to lower level cache */
    void sendResponseDown(MemEvent* event, CacheLine* cacheLine, bool dirty, bool replay);
    
    /** Send response to lower level cache, address is not cached */
    void sendResponseDownFromMSHR(MemEvent* response, MemEvent * request, bool dirty);

    /** Send writeback request to lower level caches */
    void sendWriteback(Command cmd, CacheLine* cacheLine, bool dirty, string origRqstr);
    
    /** Send AckPut to upper level cache */
    void sendWritebackAck(MemEvent * event);

    /** Send AckInv to lower level cache */
    void sendAckInv(Addr baseAddr, string origRqstr);

    /** Fetch data from owner and invalidate their copy of the line */
    void sendFetchInv(CacheLine * cacheLine, string rqstr, bool replay);
    
    /** Fetch data from owner and downgrade owner to sharer */
    void sendFetchInvX(CacheLine * cacheLine, string rqstr, bool replay);

    /** Invalidate all sharers of a block. Used for invalidations and evictions */
    void invalidateAllSharers(CacheLine * cacheLine, string rqstr, bool replay);
    
    /** Invalidate all sharers of a block except the requestor (rqstr). Used for upgrade requests. */
    bool invalidateSharersExceptRequestor(CacheLine * cacheLine, string rqstr, string origRqstr, bool replay);


/* Helper methods */
   
    void printData(vector<uint8_t> * data, bool set);

};


}}


#endif

