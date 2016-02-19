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

#ifndef L1INCOHERENTCONTROLLER_H
#define L1INCOHERENTCONTROLLER_H

#include <iostream>
#include "coherenceControllers.h"


namespace SST { namespace MemHierarchy {

class L1IncoherentController : public CoherencyController {
public:
    /** Constructor for L1IncoherentController */
    L1IncoherentController(const Cache* cache, string ownerName, Output* dbg, vector<Link*>* parentLinks, Link* childLink, CacheListener* listener, 
            unsigned int lineSize, uint64 accessLatency, uint64 tagLatency, uint64 mshrLatency, bool LLC, bool LL, MSHR * mshr, bool wbClean,
            MemNIC* bottomNetworkLink, MemNIC* topNetworkLink, bool debugAll, Addr debugAddr) :
                 CoherencyController(cache, dbg, ownerName, lineSize, accessLatency, tagLatency, mshrLatency, LLC, LL, parentLinks, childLink, 
                         bottomNetworkLink, topNetworkLink, listener, mshr, wbClean, debugAll, debugAddr) {
        d_->debug(_INFO_,"--------------------------- Initializing [L1Controller] ... \n\n");
        
    }

    ~L1IncoherentController() {}
    
    
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
    CacheAction handleReplacement(MemEvent* event, CacheLine* cacheLine, MemEvent * reqEvent, bool replay);
    
    /** Process Inv */
    CacheAction handleInvalidationRequest(MemEvent *event, CacheLine* cacheLine, bool replay);

    /** Process responses */
    CacheAction handleResponse(MemEvent* responseEvent, CacheLine* cacheLine, MemEvent* origRequest);

    /* Methods for sending events, called by cache controller */
    /** Send response up (to processor) */
    uint64_t sendResponseUp(MemEvent * event, State grantedState, vector<uint8_t>* data, bool replay, uint64_t baseTime, bool atomic = false);
    
/* Miscellaneous */
    /** Determine whether a NACKed event should be retried */
    bool isRetryNeeded(MemEvent * event, CacheLine * cacheLine);


    void printData(vector<uint8_t> * data, bool set);

private:
    /* Private event handlers */
    /** Handle GetX request. Request upgrade if needed */
    CacheAction handleGetXRequest(MemEvent* event, CacheLine* cacheLine, bool replay);
    
    /** Handle GetS request. Request block if needed */
    CacheAction handleGetSRequest(MemEvent* event, CacheLine* cacheLine, bool replay);
    
    /** Handle data response - GetSResp or GetXResp */
    void handleDataResponse(MemEvent* responseEvent, CacheLine * cacheLine, MemEvent * reqEvent);

    
    /* Methods for sending events */
    /** Send writeback request to lower level caches */
    void sendWriteback(Command cmd, CacheLine* cacheLine, string origRqstr);

};


}}


#endif

