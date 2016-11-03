// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
// 
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef INCOHERENTCONTROLLERS_H
#define INCOHERENTCONTROLLERS_H

#include <iostream>
#include "coherenceControllers.h"


namespace SST { namespace MemHierarchy {

class IncoherentController : public CoherencyController{
public:
    /** Constructor for IncoherentController. */
    IncoherentController(const Cache* cache, string ownerName, Output* dbg, vector<Link*>* parentLinks, Link* childLink, CacheListener* listener, 
            unsigned int lineSize, uint64 accessLatency, uint64 tagLatency, uint64 mshrLatency, MSHR * mshr, bool inclusive,
            MemNIC* bottomNetworkLink, MemNIC* topNetworkLink, bool debugAll, Addr debugAddr, unsigned int reqWidth,  unsigned int respWidth, unsigned int packetSize) :
                 CoherencyController(cache, dbg, ownerName, lineSize, accessLatency, tagLatency, mshrLatency, parentLinks, childLink, 
                         bottomNetworkLink, topNetworkLink, listener, mshr, debugAll, debugAddr, reqWidth, respWidth, packetSize) {
        d_->debug(_INFO_,"--------------------------- Initializing [Incoherent Controller] ... \n\n");
        inclusive_           = inclusive;

    }

    ~IncoherentController() {}
    
    /** Init function */
    void init(const char* name) {}
    
 
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
   
private:
/* Private data members */
    bool                inclusive_;
    
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

};


}}


#endif

