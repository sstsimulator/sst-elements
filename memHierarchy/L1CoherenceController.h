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

#ifndef L1COHERENCECONTROLLER_H
#define L1COHERENCECONTROLLER_H

#include <iostream>
#include "coherenceControllers.h"


namespace SST { namespace MemHierarchy {

class L1CoherenceController : public CoherencyController {
public:
    /** Constructor for L1CoherenceController */
    L1CoherenceController(const Cache* cache, string ownerName, Output* dbg, vector<Link*>* parentLinks, Link* childLink, CacheListener* listener, 
            unsigned int lineSize, uint64 accessLatency, uint64 tagLatency, uint64 mshrLatency, bool LLC, bool LL, MSHR * mshr, bool protocol, bool wbClean,
            MemNIC* bottomNetworkLink, MemNIC* topNetworkLink, bool debugAll, Addr debugAddr, bool snoopL1Invs) :
                 CoherencyController(cache, dbg, ownerName, lineSize, accessLatency, tagLatency, mshrLatency, LLC, LL, parentLinks, childLink, bottomNetworkLink, topNetworkLink, listener, mshr, 
                         wbClean, debugAll, debugAddr) {
        d_->debug(_INFO_,"--------------------------- Initializing [L1Controller] ... \n\n");
        snoopL1Invs_        = snoopL1Invs;
        protocol_           = protocol;
    }

    ~L1CoherenceController() {}
    
    /** Init funciton */
    void init(const char* name){}
    
    /** Used to determine in advance if an event will be a miss (and which kind of miss)
     * Used for statistics only
     */
    int isCoherenceMiss(MemEvent* event, CacheLine* cacheLine);
    
    /* Event handlers called by cache controller */
    /** Send cache line data to the lower level caches */
    CacheAction handleEviction(CacheLine* wbCacheLine, string origRqstr, bool ignoredParam=false);

    /** Process new cache request:  GetX, GetS, GetSEx */
    CacheAction handleRequest(MemEvent* event, CacheLine* cacheLine, bool replay);
   
    /** Process replacement - implemented for compatibility with CoherencyController but L1s do not receive replacements */
    CacheAction handleReplacement(MemEvent* event, CacheLine* cacheLine, MemEvent * origRequest, bool replay);
    
    /** Process Inv */
    CacheAction handleInvalidationRequest(MemEvent *event, CacheLine* cacheLine, bool replay);

    /** Process responses */
    CacheAction handleResponse(MemEvent* responseEvent, CacheLine* cacheLine, MemEvent* origRequest);


    /* Methods for sending events, called by cache controller */
    /** Send response up (to processor) */
    uint64_t sendResponseUp(MemEvent * event, State grantedState, vector<uint8_t>* data, bool replay, uint64_t baseTime, bool atomic = false);

/* Miscellaneous */
    /** Determine whether a retry of a NACKed event is needed */
    bool isRetryNeeded(MemEvent * event, CacheLine * cacheLine);

    void printData(vector<uint8_t> * data, bool set);

private:
    bool                protocol_;  // True for MESI, false for MSI
    bool                snoopL1Invs_;

    /* Private event handlers */
    /** Handle GetX request. Request upgrade if needed */
    CacheAction handleGetXRequest(MemEvent* event, CacheLine* cacheLine, bool replay);
    
    /** Handle GetS request. Request block if needed */
    CacheAction handleGetSRequest(MemEvent* event, CacheLine* cacheLine, bool replay);
    
    /** Handle Inv request */
    CacheAction handleInv(MemEvent * event, CacheLine * cacheLine, bool replay);
    
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
    void sendWriteback(Command cmd, CacheLine* cacheLine, string origRqstr);

    /** Send AckInv response to lower level caches */
    void sendAckInv(Addr baseAddr, string origRqstr, CacheLine * cacheLine);

};


}}


#endif

