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
    
    MemNIC*    bottomNetworkLink_;
    MemNIC*    topNetworkLink_;
    bool       groupStats_;
    uint64_t   timestamp_;
    uint64_t   accessLatency_;
    uint64_t   tagLatency_;
    uint64_t   mshrLatency_;
    string     name_;
    virtual void sendOutgoingCommands(SimTime_t curTime) = 0;
    list<Response> outgoingEventQueue_;
    
    bool queueBusy(){
        return (!outgoingEventQueue_.empty());
    }
    
    
    void addToOutgoingQueue(Response& resp){
        list<Response>::iterator it, it2;
        /* if the request is an MSHR hit, the response time will be shorter.  Therefore
           the out event queue needs to be maintained in delivery time order */
        for(it = outgoingEventQueue_.begin(); it != outgoingEventQueue_.end(); it++){
            if(resp.deliveryTime < (*it).deliveryTime){
                break;
            }
        }
        
        /*
        Addr bsAddr = resp.event->getBaseAddr();
        
        for(it2 = it; it2 != outgoingEventQueue_.end(); it2++){
            if((*it2).event->getBaseAddr() == bsAddr){
                cout << "Inserting before own event" << endl;
                cout << hex << bsAddr << " - " << hex << (*it2).event->getBaseAddr() << endl;
                cout << "Event Cmd: " << CommandString[(*it2).event->getCmd()] << endl;
                cout << "delivery time: " << (*it2).deliveryTime << endl;
                break;
            }
        }
        */
        outgoingEventQueue_.insert(it, resp);
        
        //outgoingEventQueue_.push_back(resp);
    }
    
    void setName(string _name){ name_ = _name; }
    
protected:
    CoherencyController(const Cache* _cache, Output* _dbg, uint _lineSize, uint64_t _accessLatency, uint64_t _tagLatency, uint64_t _mshrLatency):
                        timestamp_(0), accessLatency_(1), tagLatency_(1), owner_(_cache), d_(_dbg), lineSize_(_lineSize), sentEvents_(0){
        accessLatency_  = _accessLatency;
        tagLatency_     = _tagLatency;
        mshrLatency_    = _mshrLatency;
    }
    ~CoherencyController(){}
    const Cache* owner_;
    Output*    d_;
    uint       lineSize_;
    int        sentEvents_;
    bool       L1_;
    
    struct Stats{
        uint64_t GETSMissIS_,
                GETSMissBlocked_,
                GETXMissSM_,
                GETXMissIM_,
                GETXMissBlocked_,
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
            GETSMissIS_ = GETSMissBlocked_ = GETXMissSM_ = GETXMissIM_ = GETXMissBlocked_ = GETSHit_ = GETXHit_ = 0;
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
