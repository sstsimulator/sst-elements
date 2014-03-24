// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2013, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

/*
 * File:   cache.h
 * Author: Caesar De la Paz III
 * Email:  caesar.sst@gmail.com
 */

#ifndef _CACHECONTROLLER_H_
#define _CACHECONTROLLER_H_

#include <boost/assert.hpp>
#include <queue>
#include <map>

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include <sst/core/output.h>

#include "cacheArray.h"
#include "replacementManager.h"
#include "coherenceControllers.h"
#include "util.h"
#include "cacheListener.h"
#include "memNIC.h"

#define assert_msg BOOST_ASSERT_MSG

namespace SST { namespace MemHierarchy {

using namespace std;
using namespace SST::Interfaces;

class stallException : public exception{
  public:
  const char* what () const throw (){
    return "description";
  }
};

class mshrException : public exception{
  public:
  const char* what () const throw (){
    return "description";
  }
};
    
class Cache
: public SST::Component {
public:
    typedef CacheArray::CacheLine CacheLine;
    typedef TopCacheController::CCLine CCLine;
    typedef map<Addr, vector<mshrType*> > mshrTable;
    typedef unsigned int uint;
    typedef long long unsigned int uint64;
    class MSHR{
    public:
        mshrTable map_ ;
        Cache* cache_;
        int size_;
        int maxSize_;
        
        MSHR(Cache*, int);
        void insertFront(Addr baseAddr, MemEvent* event);
        bool insertAll(Addr, vector<mshrType*>) throw (mshrException);
        bool insert(Addr baseAddr, MemEvent* event);
        bool insertPointer(Addr keyAddr, Addr pointerAddr);
        bool insert(Addr baseAddr, Addr pointer);
        bool insert(Addr baseAddr, mshrType* mshrEntry) throw (mshrException);
        void removeElement(Addr baseAddr, MemEvent* event);
        void removeElement(Addr baseAddr, Addr pointer);
        void removeElement(Addr baseAddr, mshrType* mshrEntry);
        vector<mshrType*> removeAll(Addr);
        const vector<mshrType*> lookup(Addr baseAddr);
        bool isHit(Addr baseAddr);
        bool isHitAndStallNeeded(Addr baseAddr, Command cmd);
        uint getSize(){ return size_; }
        void printEntry(Addr baseAddr);
        void printEntry2(vector<MemEvent*> events);
    };
    

    
    bool clockTick(Cycle_t);
    virtual void init(unsigned int);
    virtual void setup(void);
    virtual void finish(void){
        bottomCC_->printStats(stats_, STAT_GetSExReceived_, STAT_InvalidateWaitingForUserLock_, STAT_TotalInstructionsRecieved_, STAT_NonCoherenceReqsReceived_);
        topCC_->printStats(stats_);
        listener_->printStats(*d_);
    }
    
    static Cache* cacheFactory(SST::ComponentId_t id, SST::Params& params);
    void setParents(vector<Link*>* _parentLinks) { parentLinks_ = _parentLinks;}
    void setChildren(vector<Link*>* _childrenLinks) { childrenLinks_ = _childrenLinks; }
    int getParentId(Addr baseAddr);
    int getParentId(MemEvent* event);
    Addr toBaseAddr(Addr addr){
        Addr baseAddr = (addr) & ~(cArray_->getLineSize() - 1);
        return baseAddr;
    }

private:
    Cache(SST::ComponentId_t id, SST::Params& params, string _cacheFrequency, CacheArray* _cacheArray, uint _protocol, 
           uint _numParents, uint _numChildren, Output* _d_, LRUReplacementMgr* _rm, uint _numLines, uint lineSize,
           uint MSHRSize, bool _L1, bool _dirControllerExists);


    uint                    ID_;
    CacheArray*             cArray_;
    CacheListener*          listener_;
    uint                    protocol_;
    uint                    numParents_;
    uint                    numChildren_;
    vector<Link*>*          parentLinks_;
    uint                    parentLinksListSize_;
    vector<Link*>*          childrenLinks_;
    Link*                   selfLink_;
    MemNIC*                 directoryLink_;
    Output*                 d_;
    LRUReplacementMgr*      replacementMgr_;
    uint                    numLines_;
    uint                    lineSize_;
    uint                    MSHRSize_;
    string                  nextLevelCacheName_;
    bool                    L1_;
    bool                    dirControllerExists_;
    MSHR*                   mshr_;
    MSHR*                   mshrUncached_;
    TopCacheController*     topCC_;
    MESIBottomCC*           bottomCC_;
    bool                    sharersAware_;
    vector<MemEvent*>       retryQueue_;
    vector<MemEvent*>       retryQueueNext_;
    queue<pair<SST::Event*, uint64> >   incomingEventQueue_;
    uint64                  accessLatency_;
    uint64                  STAT_GetSExReceived_;
    uint64                  STAT_InvalidateWaitingForUserLock_;
    uint64                  STAT_TotalInstructionsRecieved_;
    uint64                  STAT_NonCoherenceReqsReceived_;
    uint64                  timestamp_;
    map<MemEvent::id_type, SimTime_t> latencyTimes;
    int                     stats_;
 	std::map<string, LinkId_t>     nameMap_;
    std::map<LinkId_t, SST::Link*> linkIdMap_;
    
    void initStats();
    void errorChecking();
    void pMembers();
    bool shouldThisInvalidateRequestProceed(MemEvent *event, CacheLine* cacheLine, Addr baseAddr, bool reActivation);
    bool invalidatesInProgress(int lineIndex);
    bool isCacheLineStable(CacheLine* cacheLine, Command cmd);
    void processEvent(SST::Event* ev, bool reActivation);
    inline void allocateCacheLine(MemEvent *event, Addr baseAddr, int& lineIndex) throw(stallException);
    void processIncomingEvent(SST::Event *event);
    void processAccess(MemEvent *event, Command cmd, Addr baseAddr, bool reActivation) throw(stallException);
    void processInvalidate(MemEvent *event, Command cmd, Addr baseAddr, bool reActivation);
    void processAccessAcknowledge(MemEvent* ackEvent, Addr baseAddr);
    void processInvalidateAcknowledge(MemEvent* event, Addr baseAddr, bool reActivation);
    void processUncached(MemEvent* event, Command cmd, Addr baseAddr);
    void processFetch(MemEvent* event, Addr baseAddr, bool reActivation);
    bool isPowerOfTwo(uint x){ return (x & (x - 1)) == 0; }
    inline void activatePrevEvents(Addr baseAddr);
    inline bool activatePrevEvent(MemEvent* event, vector<mshrType*>& mshrEntry, Addr addr, vector<mshrType*>::iterator it, int i);
    void handleEvent(SST::Event *event);
    Addr getIndex(Addr addr){ return (addr >> log2Of(cArray_->getLineSize())); }
    inline CacheLine* getCacheLine(Addr baseAddr);
    inline CacheLine* getCacheLine(int lineIndex);
    void configureLinks();
    void handlePrefetchEvent(SST::Event *event);
    void handleSelfEvent(SST::Event *event);
    void registerNewRequest(MemEvent::id_type id){ latencyTimes[id] = getCurrentSimTimeNano(); }

    inline bool isCacheMiss(int lineIndex);
    inline bool isCachelineLockedByUser(CacheLine* cacheLine);
    inline bool checkRequestValidity(MemEvent* event, Addr baseAddr) throw(stallException);

    inline void evictInHigherLevelCaches(CacheLine* wbCacheLine, Addr requestBaseAddr) throw (stallException);
    inline bool isCandidateInTransition(CacheLine* wbCacheLine);
    inline void candidacyCheck(MemEvent* event, CacheLine* wbCacheLine, Addr requestBaseAddr) throw(stallException);
    inline bool writebackToLowerLevelCaches(MemEvent *event, CacheLine* wbCacheLine, Addr baseAddr);
    inline void replaceCacheLine(int replacementCacheLineIndex, int& newCacheLineIndex, Addr newBaseAddr);
    inline CacheLine* findReplacementCacheLine(Addr baseAddr);
    inline bool isCacheLineAllocated(int lineIndex);
    inline void postRequestProcessing(MemEvent* event, CacheLine* cacheLine, bool requestCompleted, bool reActivation);

    inline void retryRequestLater(MemEvent* event, Addr baseAddr);
    inline TopCacheController::CCLine* getCCLine(int index);
    inline bool isCacheLineValidAndWaitingForAck(Addr baseAddr, int lineIndex);
    inline void reActivateEventWaitingForUserLock(CacheLine* cacheLine, bool reActivation);
    inline LinkId_t lookupNode(const std::string &name);
    inline void mapNodeEntry(const std::string &name, LinkId_t id);

};

}}

#endif
