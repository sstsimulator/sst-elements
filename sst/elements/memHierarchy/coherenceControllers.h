// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

/*
 * File:   coherenceControllers.h
 * Author: Caesar De la Paz III
 * Email:  caesar.sst@gmail.com
 */


#ifndef COHERENCECONTROLLERS_H
#define COHERENCECONTROLLERS_H

#include "assert.h"
#include "sst/core/link.h"
#include <bitset>
#include <cstring>
#include "cacheArray.h"
#include "sst/core/output.h"
#include "cacheListener.h"
#include <boost/assert.hpp>
#include <list>
#include "memNIC.h"

using namespace std;
using namespace SST::MemHierarchy;

namespace SST { namespace MemHierarchy {

class Cache;

class CoherencyController{
public:
    typedef CacheArray::CacheLine CacheLine;
    typedef unsigned int uint;
    typedef uint64_t uint64;

    struct Response {
        MemEvent* event;
        uint64_t deliveryTime;
        bool cpuResponse;
    };
    
    MemNIC*    directoryLink_;
    bool       groupStats_;
    uint64_t   timestamp_;
    uint64_t   accessLatency_;
    uint64_t   mshrLatency_;
    string     name_;
    virtual void sendOutgoingCommands() = 0;
    list<Response> outgoingEventQueue_;
    
    bool queueBusy(){
        return (!outgoingEventQueue_.empty());
    }
    
    
    void addToOutgoingQueue(Response& resp){
        list<Response>::iterator it;
        for(it = outgoingEventQueue_.begin(); it != outgoingEventQueue_.end(); it++){
            if(resp.deliveryTime < (*it).deliveryTime){
                break;
            }
        }
        outgoingEventQueue_.insert(it, resp);
    }
    
    void setName(string _name){ name_ = _name; }
    
protected:
    CoherencyController(const Cache* _cache, Output* _dbg, uint _lineSize, uint64_t _accessLatency, uint64_t _mshrLatency):
                        timestamp_(0), accessLatency_(1), owner_(_cache), d_(_dbg), lineSize_(_lineSize), sentEvents_(0){
        accessLatency_ = _accessLatency;
        mshrLatency_   = _mshrLatency;
    }
    ~CoherencyController(){}
    const Cache* owner_;
    Output*    d_;
    uint       lineSize_;
    int        sentEvents_;
    bool       L1_;
    
    struct Stats{
        uint64_t GETSMissIS_,
                GETXMissSM_,
                GETXMissIM_,
                GETSHit_,
                GETXHit_,
                PUTSReqsReceived_,
                PUTEReqsReceived_,
                PUTMReqsReceived_,
                PUTXReqsReceived_,
                GetSExReqsReceived_,
                GetXReqsReceived_,
                GetSReqsReceived_,
                EvictionPUTSReqSent_,
                EvictionPUTMReqSent_,
                EvictionPUTEReqSent_,
                InvalidatePUTMReqSent_,
                InvalidatePUTEReqSent_,
                InvalidatePUTXReqSent_,
                InvalidatePUTSReqSent_,
                FetchInvReqSent_,
                FetchInvXReqSent_,
                NACKsSent_;
        
        Stats(){
            initialize();
        }
        
        void initialize(){
            GETSMissIS_ = GETXMissSM_ = GETXMissIM_ = GETSHit_ = GETXHit_ = 0;
            PUTSReqsReceived_ = PUTEReqsReceived_ = PUTMReqsReceived_ = PUTXReqsReceived_ = 0;
            EvictionPUTSReqSent_ = EvictionPUTMReqSent_ = EvictionPUTEReqSent_ = 0;
            InvalidatePUTMReqSent_ = InvalidatePUTEReqSent_ = InvalidatePUTXReqSent_ = InvalidatePUTSReqSent_ = 0;
            GetSExReqsReceived_ = GetSReqsReceived_ = GetXReqsReceived_ = 0;
            NACKsSent_ = 0;
            FetchInvReqSent_ = FetchInvXReqSent_ = 0;
        }
    };
};




}}

#endif	/* COHERENCECONTROLLERS_H */
