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


#ifndef MESIINTERNALDIRCONTROLLER_H
#define MESIINTERNALDIRCONTROLLER_H

#include <iostream>
#include "coherenceControllers.h"


namespace SST { namespace MemHierarchy {

class MESIInternalDirectory : public CoherencyController {
public:
    /** Constructor for MESIInternalDirectory. */
    MESIInternalDirectory(const Cache* directory, string ownerName, Output* dbg, vector<Link*>* parentLinks, Link* childLink, CacheListener* listener, 
            unsigned int lineSize, uint64 accessLatency, uint64 tagLatency, uint64 mshrLatency, bool LLC, bool LL, MSHR * mshr, bool protocol,
            bool wbClean, MemNIC* bottomNetworkLink, MemNIC* topNetworkLink, bool debugAll, Addr debugAddr) :
                 CoherencyController(directory, dbg, ownerName, lineSize, accessLatency, tagLatency, mshrLatency, LLC, LL, parentLinks, childLink, 
                         bottomNetworkLink, topNetworkLink, listener, mshr, wbClean, debugAll, debugAddr) {
        d_->debug(_INFO_,"--------------------------- Initializing [MESI + Directory Controller] ... \n\n");
        protocol_           = protocol;

    }

    ~MESIInternalDirectory() {}
    
    
    /** Init funciton */
    void init(const char* name){}
 
/*----------------------------------------------------------------------------------------------------------------------
 *  Public functions form external interface to the coherence controller
 *---------------------------------------------------------------------------------------------------------------------*/  

/* Event handlers */
    /** Send cache line data to the lower level caches */
    CacheAction handleEviction(CacheLine* replacementLine, string origRqstr, bool fromDataCache);

    /** Process cache request:  GetX, GetS, GetSEx */
    CacheAction handleRequest(MemEvent* event, CacheLine* dirLine, bool replay);
    
    /** Process replacement request - PutS, PutE, PutM. May also resolve an outstanding/racing request event */
    CacheAction handleReplacement(MemEvent* event, CacheLine* dirLine, MemEvent * reqEvent, bool replay);
    
    /** Process invalidation requests - Inv, FetchInv, FetchInvX */
    CacheAction handleInvalidationRequest(MemEvent *event, CacheLine* dirLine, bool replay);

    /** Process responses - GetSResp, GetXResp, FetchResp */
    CacheAction handleResponse(MemEvent* responseEvent, CacheLine* dirLine, MemEvent* origRequest);
    

/* Miscellaneous */
    
    /** Determine in advance if a request will miss (and what kind of miss). Used for stats */
    int isCoherenceMiss(MemEvent* event, CacheLine* dirLine);

    /** Determine whether a NACKed event should be retried */
    bool isRetryNeeded(MemEvent * event, CacheLine * cacheLine);

private:
/* Private data members */
    bool                protocol_;  // True for MESI, false for MSI

/* Private event handlers */
    /** Handle GetX request. Request upgrade if needed */
    CacheAction handleGetXRequest(MemEvent* event, CacheLine* dirLine, bool replay);

    /** Handle GetS request. Request block if needed */
    CacheAction handleGetSRequest(MemEvent* event, CacheLine* dirLine, bool replay);
    
    /** Handle PutS request. Possibly complete a waiting request if it raced with the PutS */
    CacheAction handlePutSRequest(MemEvent* event, CacheLine * dirLine, MemEvent * origReq);
    
    /** Handle PutM request. Possibly complete a waiting request if it raced with the PutM */
    CacheAction handlePutMRequest(MemEvent* event, CacheLine * dirLine, MemEvent * origReq);

    /** Handle Inv */
    CacheAction handleInv(MemEvent * event, CacheLine * dirLine, bool replay, MemEvent * collisionEvent);
    
    /** Handle Fetch */
    CacheAction handleFetch(MemEvent * event, CacheLine * dirLine, bool replay, MemEvent * collisionEvent);
    
    /** Handle FetchInv */
    CacheAction handleFetchInv(MemEvent * event, CacheLine * dirLine, bool replay, MemEvent * collisionEvent);
    
    /** Handle FetchInvX */
    CacheAction handleFetchInvX(MemEvent * event, CacheLine * dirLine, bool replay, MemEvent * collisionEvent);

    /** Process GetSResp/GetXResp */
    CacheAction handleDataResponse(MemEvent* responseEvent, CacheLine * dirLine, MemEvent * reqEvent);
    
    /** Handle FetchResp */
    CacheAction handleFetchResp(MemEvent * responseEvent, CacheLine* dirLine, MemEvent * reqEvent);
    
    /** Handle Ack */
    CacheAction handleAckInv(MemEvent * responseEvent, CacheLine* dirLine, MemEvent * reqEvent);

/* Private methods for sending events */
    void sendResponseDown(MemEvent* event, CacheLine* dirLine, std::vector<uint8_t>* data, bool dirty, bool replay);
   
    /** Send response to lower level cache using 'event' instead of dirLine */
    void sendResponseDownFromMSHR(MemEvent* event, bool dirty);

    /** Send writeback request to lower level caches */
    void sendWriteback(Command cmd, CacheLine* dirLine, string origRqstr);
    
    /** Send writeback request to lower level cache using data from cache */
    void sendWritebackFromCache(Command cmd, CacheLine* dirLine, string origRqstr);

    /** Send writeback request to lower level cache using data from MSHR */
    void sendWritebackFromMSHR(Command cmd, CacheLine* dirLine, string origRqstr, std::vector<uint8_t>* data);
    
    /** Send writeback ack */
    void sendWritebackAck(MemEvent * event);

    /** Send AckInv to lower level cache */
    void sendAckInv(Addr baseAddr, string origRqstr);

    /** Fetch data from owner and invalidate their copy of the line */
    void sendFetchInv(CacheLine * dirLine, string rqstr, bool replay);
    
    /** Fetch data from owner and downgrade owner to sharer */
    void sendFetchInvX(CacheLine * dirLine, string rqstr, bool replay);

    /** Fetch data from sharer */
    void sendFetch(CacheLine * dirLine, string rqstr, bool replay);

    /** Invalidate all sharers of a block. Used for invalidations and evictions */
    void invalidateAllSharers(CacheLine * dirLine, string rqstr, bool replay);
    
    /** Invalidate all sharers of a block and fetch block from one of them. Used for invalidations and evictions */
    void invalidateAllSharersAndFetch(CacheLine * dirLine, string rqstr, bool replay);
    
    /** Invalidate all sharers of a block except the requestor (rqstr). If requestor is not a sharer, may fetch data from a sharer. Used for upgrade requests. */
    bool invalidateSharersExceptRequestor(CacheLine * dirLine, string rqstr, string origRqstr, bool replay, bool checkFetch);


/* Miscellaneous */
   
    void printData(vector<uint8_t> * data, bool set);

};


}}


#endif

