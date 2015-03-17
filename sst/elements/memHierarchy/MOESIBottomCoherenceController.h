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

/*
 * File:   MOESIBottomCoherenceController.h
 * Author: Caesar De la Paz III
 * Email:  caesar.sst@gmail.com
 */


#ifndef __SST__MOESIBottomCoherenceController__
#define __SST__MOESIBottomCoherenceController__

#include <iostream>
#include "MESIBottomCoherenceController.h"


namespace SST { namespace MemHierarchy {

class MOESIBottomCC : public MESIBottomCC{
private:

public:
    MOESIBottomCC(const SST::MemHierarchy::Cache* _cache, string _ownerName, Output* _dbg,
                 vector<Link*>* _parentLinks, CacheListener* _listener, unsigned int _lineSize,
                 uint64 _accessLatency, uint64 _tagLatency, uint64 _mshrLatency, bool _L1, MemNIC* _directoryLink,
                 bool _groupStats, vector<int> _statGroupIds) :
                 MESIBottomCC(_cache, _ownerName, _dbg, _parentLinks, _listener, _lineSize,
                 _accessLatency, _tagLatency, _mshrLatency, _L1, _directoryLink, _groupStats, _statGroupIds) {}

    virtual void handleEviction(CacheLine* wbCacheLine);
    virtual void handleRequest(MemEvent* event, CacheLine* cacheLine, Command cmd);
    virtual void handleResponse(MemEvent* ackEvent, CacheLine* cacheLine, MemEvent* _origRequest);
    virtual void handleWritebackOnAccess(Addr lineAddr, CacheLine* cacheLine, Command type);
    virtual void handleInvalidate(MemEvent *event, CacheLine* cacheLine, Command cmd);
    virtual void handleFetch(MemEvent *event, CacheLine* cacheLine, int _parentId);
    virtual void handleFetchInvalidate(MemEvent* _event, CacheLine* _cacheLine, int _parentId);

};

}}



#endif /* defined(__SST__MOESIBottomCoherenceController__) */
