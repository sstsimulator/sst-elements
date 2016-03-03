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
#include "mshr.h"
#include "sst/core/output.h"
#include "cacheListener.h"
#include <boost/assert.hpp>
#include <list>
#include <vector>
#include "memNIC.h"

using namespace std;
using namespace SST::MemHierarchy;

namespace SST { namespace MemHierarchy {

class Cache;

/*  Coherence-related defines 
 *  These are defines rather than parameters because they are intended to be rarely changed
 *  and should be changed with caution. Deadlocks, races, incoherence, and in general, chaos, can result!
 */

/* Writeback acks are used to avoid races between Inv and Put events and are required if any of the following are true:
 * 1) A Put may be stalled (and hence NACKed), e.g., with a directory cache
 * 2) Non-inclusive caches exist in the system. Here, it isn't clear whether a cache miss is due to exclusion or a pending replacement
 * Acks can be disabled for inclusive hierarchies where the directory cannot miss on a Put
 */
#define DISABLE_WRITEBACK_ACK 0

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
    
    MemNIC*     bottomNetworkLink_; // Ptr to memNIC for our lower link (if network-connected)
    MemNIC*     topNetworkLink_;    // Ptr to memNIC for our upper link (if network-connected)
    uint64_t    timestamp_;         // Local timestamp (cycles)
    uint64_t    accessLatency_;     // Cache access latency
    uint64_t    tagLatency_;        // Cache tag access latency
    uint64_t    mshrLatency_;       // MSHR lookup latency
    string      name_;              // Name of cache we are associated with
    bool        LLC_;               // True if this is the last-level-cache in the system
    bool        LL_;                // True if this is both the last-level-cache AND there is no other coherence entity (e.g., a directory) below us
    MSHR *      mshr_;              // Pointer to cache's MSHR, coherence controllers are responsible for managing writeback acks

    list<Response> outgoingEventQueue_;
    list<Response> outgoingEventQueueUp_;

    bool        DEBUG_ALL;
    Addr        DEBUG_ADDR;


    // Pure virtual functions
    virtual int isCoherenceMiss(MemEvent * event, CacheLine * line) =0;
    virtual CacheAction handleRequest(MemEvent * event, CacheLine * line, bool replay) =0;
    virtual CacheAction handleReplacement(MemEvent * event, CacheLine * line, MemEvent * reqEvent, bool replay) =0;
    virtual CacheAction handleInvalidationRequest(MemEvent * event, CacheLine * line, bool replay) =0;
    virtual CacheAction handleEviction(CacheLine * line, string rqstr, bool fromDataCache=false) =0;
    virtual CacheAction handleResponse(MemEvent * event, CacheLine * line, MemEvent * request) =0;
    
    virtual bool isRetryNeeded(MemEvent * event, CacheLine * line) =0;

    // Let cache update timestamp
    void updateTimestamp(uint64_t newTS) { timestamp_ = newTS; }

    // Send NACK in response to a request. Could be made virtual if needed.
    void sendNACK(MemEvent * event, bool up) {
        MemEvent *NACKevent = event->makeNACKResponse((Component*)owner_, event);
    
        uint64 deliveryTime      = timestamp_ + tagLatency_;
        Response resp = {NACKevent, deliveryTime, true};
        if (up) {
            addToOutgoingQueueUp(resp);
        } else {
            addToOutgoingQueue(resp);
        }
#ifdef __SST_DEBUG_OUTPUT__
        if (DEBUG_ALL || DEBUG_ADDR == event->getBaseAddr()) d_->debug(_L3_,"Sending NACK at cycle = %" PRIu64 "\n", deliveryTime);
#endif
    }


    // Non-L1s can inherit this version, L1s should implement a different version to split out the requested block
    virtual uint64_t sendResponseUp(MemEvent * event, State grantedState, vector<uint8_t>* data, bool replay, uint64_t baseTime, bool atomic=false) {
        MemEvent * responseEvent = event->makeResponse(grantedState);
        responseEvent->setDst(event->getSrc());
        responseEvent->setSize(event->getSize());
        if (data != NULL) responseEvent->setPayload(*data);
    
        if (baseTime < timestamp_) baseTime = timestamp_;
        uint64_t deliveryTime = baseTime + (replay ? mshrLatency_ : accessLatency_);
        Response resp = {responseEvent, deliveryTime, true};
        addToOutgoingQueueUp(resp);
    
#ifdef __SST_DEBUG_OUTPUT__
        if (DEBUG_ALL || DEBUG_ADDR == event->getBaseAddr()) d_->debug(_L3_,"Sending Response at cycle = %" PRIu64 ". Current Time = %" PRIu64 ", Addr = %" PRIx64 ", Dst = %s, Payload Bytes = %i, Granted State = %s\n", 
                deliveryTime, timestamp_, event->getAddr(), responseEvent->getDst().c_str(), responseEvent->getPayloadSize(), StateString[responseEvent->getGrantedState()]);
#endif
        return deliveryTime;
    }
    
  
    void resendEvent(MemEvent * event, bool up) {
        // Add some backoff latency to avoid unneccessary traffic
        int retries = event->getRetries();
        if (retries > 10) retries = 10;
        uint64 backoff = ( 0x1 << retries);
        event->incrementRetries();

        uint64 deliveryTime =  timestamp_ + mshrLatency_ + backoff;
        Response resp = {event, deliveryTime, false};
        if (!up) addToOutgoingQueue(resp);
        else addToOutgoingQueueUp(resp);
#ifdef __SST_DEBUG_OUTPUT__
        if (DEBUG_ALL || DEBUG_ADDR == event->getBaseAddr()) d_->debug(_L3_,"Sending request: Addr = %" PRIx64 ", BaseAddr = %" PRIx64 ", Cmd = %s\n", 
                event->getAddr(), event->getBaseAddr(), CommandString[event->getCmd()]);
#endif
    }
  

    // Could make this virtual if needed
    uint64_t forwardMessage(MemEvent * event, Addr baseAddr, unsigned int requestSize, uint64_t baseTime, vector<uint8_t>* data) {
        /* Create event to be forwarded */
        MemEvent* forwardEvent;
        forwardEvent = new MemEvent(*event);
        forwardEvent->setSrc(name_);
        forwardEvent->setDst(getDestination(baseAddr));
        forwardEvent->setSize(requestSize);
    
        if (data != NULL) forwardEvent->setPayload(*data);

        /* Determine latency in cycles */
        uint64_t deliveryTime;
        if (baseTime < timestamp_) baseTime = timestamp_;
        if (event->queryFlag(MemEvent::F_NONCACHEABLE)) {
            forwardEvent->setFlag(MemEvent::F_NONCACHEABLE);
            deliveryTime = timestamp_ + mshrLatency_;
        } else deliveryTime = baseTime + tagLatency_; 
    
        Response fwdReq = {forwardEvent, deliveryTime, false};
        addToOutgoingQueue(fwdReq);
#ifdef __SST_DEBUG_OUTPUT__
        if (DEBUG_ALL || DEBUG_ADDR == event->getBaseAddr()) d_->debug(_L3_,"Forwarding request at cycle = %" PRIu64 "\n", deliveryTime);        
#endif
        return deliveryTime;
    }
    

    // Set upper and lower level cache names for addressing
    void setLowerLevelCache(vector<string>* nlc) {
        lowerLevelCacheNames_ = *(nlc);   
    }
    
    
    void setUpperLevelCache(vector<string>* nlc) {
        upperLevelCacheNames_ = *(nlc);    
    }
    

    /**
     *  Send outgoing commands if the arey ready (according to timestamp)
     *  @return Whether queue is empty or not
     */
    virtual bool sendOutgoingCommands(SimTime_t curTime) {
        // Update timestamp
        timestamp_++;
        
        // Send events down
        while(!outgoingEventQueue_.empty() && outgoingEventQueue_.front().deliveryTime <= timestamp_) {
            MemEvent *outgoingEvent = outgoingEventQueue_.front().event;
            recordEventSentDown(outgoingEvent->getCmd());
#ifdef __SST_DEBUG_OUTPUT__
            if (DEBUG_ALL || outgoingEvent->getBaseAddr() == DEBUG_ADDR) {
                d_->debug(_L4_,"SEND. Cmd: %s, BsAddr: %" PRIx64 ", Addr: %" PRIx64 ", Rqstr: %s, Src: %s, Dst: %s, PreF:%s, Rqst size = %u, Payload size = %u, time: (%" PRIu64 ", %" PRIu64 ")\n",
                   CommandString[outgoingEvent->getCmd()], outgoingEvent->getBaseAddr(), outgoingEvent->getAddr(), outgoingEvent->getRqstr().c_str(), outgoingEvent->getSrc().c_str(), 
                   outgoingEvent->getDst().c_str(), outgoingEvent->isPrefetch() ? "true" : "false", outgoingEvent->getSize(), outgoingEvent->getPayloadSize(), timestamp_, curTime);
            }
#endif

            if(bottomNetworkLink_) {
                outgoingEvent->setDst(bottomNetworkLink_->findTargetDestination(outgoingEvent->getBaseAddr()));
                bottomNetworkLink_->send(outgoingEvent);
            } else {
                lowNetPorts_->at(0)->send(outgoingEvent);
            }
            outgoingEventQueue_.pop_front();
        }

        // Send events up
        while(!outgoingEventQueueUp_.empty() && outgoingEventQueueUp_.front().deliveryTime <= timestamp_) {
            MemEvent * outgoingEvent = outgoingEventQueueUp_.front().event;
            recordEventSentUp(outgoingEvent->getCmd());
#ifdef __SST_DEBUG_OUTPUT__
            if (DEBUG_ALL || outgoingEvent->getBaseAddr() == DEBUG_ADDR) {
                d_->debug(_L4_,"SEND. Cmd: %s, BsAddr: %" PRIx64 ", Addr: %" PRIx64 ", Rqstr: %s, Src: %s, Dst: %s, PreF:%s, Rqst size = %u, Payload size = %u, time: (%" PRIu64 ", %" PRIu64 ")\n",
                   CommandString[outgoingEvent->getCmd()], outgoingEvent->getBaseAddr(), outgoingEvent->getAddr(), outgoingEvent->getRqstr().c_str(), outgoingEvent->getSrc().c_str(), 
                   outgoingEvent->getDst().c_str(), outgoingEvent->isPrefetch() ? "true" : "false", outgoingEvent->getSize(), outgoingEvent->getPayloadSize(), timestamp_, curTime);
            }
#endif

            if (topNetworkLink_) {
                topNetworkLink_->send(outgoingEvent);
            } else {
                highNetPort_->send(outgoingEvent);
            }
            outgoingEventQueueUp_.pop_front();
        }
        return (outgoingEventQueue_.empty() && outgoingEventQueueUp_.empty());
    }
    
    // Add a message to the outgoing queue down in timestamp order 
    // Do not re-order events for the same address. Cache banks mostly take care of this
    // except when we invalidate a block and then re-request it, the requests could be inverted.
    void addToOutgoingQueue(Response& resp) {
        list<Response>::reverse_iterator rit;
        for (rit = outgoingEventQueue_.rbegin(); rit!= outgoingEventQueue_.rend(); rit++) {
            if (resp.deliveryTime >= (*rit).deliveryTime) break;
            if (resp.event->getBaseAddr() == (*rit).event->getBaseAddr()) break;
        }
        outgoingEventQueue_.insert(rit.base(), resp);
    }
   
    // Add a message to the outgoing queue up in timestamp order
    void addToOutgoingQueueUp(Response& resp) {
        list<Response>::reverse_iterator rit;
        for (rit = outgoingEventQueueUp_.rbegin(); rit != outgoingEventQueueUp_.rend(); rit++) {
            if (resp.deliveryTime >= (*rit).deliveryTime) break;
            if (resp.event->getBaseAddr() == (*rit).event->getBaseAddr()) break;
        }
        outgoingEventQueueUp_.insert(rit.base(), resp);
    }


    
    void recordLatency(Command cmd, State state, uint64_t latency) {
        switch (state) {
            case IS:
                stat_latency_GetS_IS->addData(latency);
                break;
            case IM:
                if (cmd == GetX) stat_latency_GetX_IM->addData(latency);
                else stat_latency_GetSEx_IM->addData(latency);
                break;
            case SM:
                if (cmd == GetX) stat_latency_GetX_SM->addData(latency);
                else stat_latency_GetSEx_SM->addData(latency);
                break;
            case M:
                if (cmd == GetS) stat_latency_GetS_M->addData(latency);
                if (cmd == GetX) stat_latency_GetX_M->addData(latency);
                else stat_latency_GetSEx_M->addData(latency);
                break;
            default:
                break;
        }
    }


protected:
    CoherencyController(const Cache* cache, Output* dbg, string name, uint lineSize, uint64_t accessLatency, uint64_t tagLatency, uint64_t mshrLatency, bool LLC, bool LL, 
            vector<Link*>* parentLinks, Link* childLink, MemNIC* bottomNetworkLink, MemNIC* topNetworkLink, CacheListener* listener, MSHR * mshr, bool wbClean, 
            bool debugAll, Addr debugAddr):
                        timestamp_(0), accessLatency_(1), tagLatency_(1), owner_(cache), d_(dbg), lineSize_(lineSize), sentEvents_(0) {
        name_                   = name;
        accessLatency_          = accessLatency;
        tagLatency_             = tagLatency;
        mshrLatency_            = mshrLatency;
        LLC_                    = LLC;
        LL_                     = LL;
        mshr_                   = mshr;
        DEBUG_ALL               = debugAll;
        DEBUG_ADDR              = debugAddr;
        bottomNetworkLink_      = bottomNetworkLink;
        topNetworkLink_         = topNetworkLink;
        lowNetPorts_            = parentLinks;
        highNetPort_            = childLink;
        listener_               = listener;
        writebackCleanBlocks_   = wbClean;

        // Register statistics - TODO register in a protocol-specific way??
        stat_evict_I = ((Component *)owner_)->registerStatistic<uint64_t>("evict_I");
        stat_evict_S = ((Component *)owner_)->registerStatistic<uint64_t>("evict_S");
        stat_evict_E = ((Component *)owner_)->registerStatistic<uint64_t>("evict_E");
        stat_evict_M = ((Component *)owner_)->registerStatistic<uint64_t>("evict_M");
        stat_evict_IS = ((Component *)owner_)->registerStatistic<uint64_t>("evict_IS");
        stat_evict_IM = ((Component *)owner_)->registerStatistic<uint64_t>("evict_IM");
        stat_evict_SM = ((Component *)owner_)->registerStatistic<uint64_t>("evict_SM");
        stat_evict_SInv = ((Component *)owner_)->registerStatistic<uint64_t>("evict_SInv");
        stat_evict_EInv = ((Component *)owner_)->registerStatistic<uint64_t>("evict_EInv");
        stat_evict_MInv = ((Component *)owner_)->registerStatistic<uint64_t>("evict_MInv");
        stat_evict_SMInv = ((Component *)owner_)->registerStatistic<uint64_t>("evict_SMInv");
        stat_evict_EInvX = ((Component *)owner_)->registerStatistic<uint64_t>("evict_EInvX");
        stat_evict_MInvX = ((Component *)owner_)->registerStatistic<uint64_t>("evict_MInvX");
        stat_evict_SI = ((Component *)owner_)->registerStatistic<uint64_t>("evict_SI");

        stat_stateEvent_GetS_I = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_GetS_I");
        stat_stateEvent_GetS_S = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_GetS_S");
        stat_stateEvent_GetS_E = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_GetS_E");
        stat_stateEvent_GetS_M = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_GetS_M");
        stat_stateEvent_GetX_I = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_GetX_I");
        stat_stateEvent_GetX_S = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_GetX_S");
        stat_stateEvent_GetX_E = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_GetX_E");
        stat_stateEvent_GetX_M = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_GetX_M");
        stat_stateEvent_GetSEx_I = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_GetSEx_I");
        stat_stateEvent_GetSEx_S = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_GetSEx_S");
        stat_stateEvent_GetSEx_E = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_GetSEx_E");
        stat_stateEvent_GetSEx_M = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_GetSEx_M");
        stat_stateEvent_GetSResp_IS = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_GetSResp_IS");
        stat_stateEvent_GetSResp_IM = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_GetSResp_IM");
        stat_stateEvent_GetSResp_SM = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_GetSResp_SM");
        stat_stateEvent_GetSResp_SMInv = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_GetSResp_SMInv");
        stat_stateEvent_GetXResp_IM = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_GetXResp_IM");
        stat_stateEvent_GetXResp_SM = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_GetXResp_SM");
        stat_stateEvent_GetXResp_SMInv = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_GetXResp_SMInv");
        stat_stateEvent_PutS_I = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_PutS_I");
        stat_stateEvent_PutS_S = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_PutS_S");
        stat_stateEvent_PutS_E = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_PutS_E");
        stat_stateEvent_PutS_M = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_PutS_M");
        stat_stateEvent_PutS_SD = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_PutS_SD");
        stat_stateEvent_PutS_ED = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_PutS_ED");
        stat_stateEvent_PutS_MD = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_PutS_MD");
        stat_stateEvent_PutS_SMD = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_PutS_SMD");
        stat_stateEvent_PutS_SI = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_PutS_SI");
        stat_stateEvent_PutS_EI = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_PutS_EI");
        stat_stateEvent_PutS_MI = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_PutS_MI");
        stat_stateEvent_PutS_SInv = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_PutS_SInv");
        stat_stateEvent_PutS_EInv = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_PutS_EInv");
        stat_stateEvent_PutS_MInv = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_PutS_MInv");
        stat_stateEvent_PutS_SMInv = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_PutS_SMInv");
        stat_stateEvent_PutS_EInvX = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_PutS_EInvX");
        stat_stateEvent_PutE_I = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_PutE_I");
        stat_stateEvent_PutE_E = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_PutE_E");
        stat_stateEvent_PutE_M = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_PutE_M");
        stat_stateEvent_PutE_EI = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_PutE_EI");
        stat_stateEvent_PutE_MI = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_PutE_MI");
        stat_stateEvent_PutE_EInv = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_PutE_EInv");
        stat_stateEvent_PutE_MInv = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_PutE_MInv");
        stat_stateEvent_PutE_EInvX = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_PutE_EInvX");
        stat_stateEvent_PutE_MInvX = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_PutE_MInvX");
        stat_stateEvent_PutM_I = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_PutM_I");
        stat_stateEvent_PutM_E = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_PutM_E");
        stat_stateEvent_PutM_M = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_PutM_M");
        stat_stateEvent_PutM_EI = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_PutM_EI");
        stat_stateEvent_PutM_MI = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_PutM_MI");
        stat_stateEvent_PutM_EInv = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_PutM_EInv");
        stat_stateEvent_PutM_MInv = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_PutM_MInv");
        stat_stateEvent_PutM_EInvX = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_PutM_EInvX");
        stat_stateEvent_PutM_MInvX = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_PutM_MInvX");
        stat_stateEvent_Inv_I = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_Inv_I");
        stat_stateEvent_Inv_IS = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_Inv_IS");
        stat_stateEvent_Inv_IM = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_Inv_IM");
        stat_stateEvent_Inv_S = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_Inv_S");
        stat_stateEvent_Inv_SM = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_Inv_SM");
        stat_stateEvent_Inv_SInv = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_Inv_SInv");
        stat_stateEvent_Inv_SI = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_Inv_SI");
        stat_stateEvent_Inv_SMInv = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_Inv_SMInv");
        stat_stateEvent_Inv_SD = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_Inv_SD");
        stat_stateEvent_FetchInv_I = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_FetchInv_I");
        stat_stateEvent_FetchInv_IS = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_FetchInv_IS");
        stat_stateEvent_FetchInv_IM = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_FetchInv_IM");
        stat_stateEvent_FetchInv_SM = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_FetchInv_SM");
        stat_stateEvent_FetchInv_S = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_FetchInv_S");
        stat_stateEvent_FetchInv_E = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_FetchInv_E");
        stat_stateEvent_FetchInv_M = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_FetchInv_M");
        stat_stateEvent_FetchInv_EI = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_FetchInv_EI");
        stat_stateEvent_FetchInv_MI = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_FetchInv_MI");
        stat_stateEvent_FetchInv_EInv = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_FetchInv_EInv");
        stat_stateEvent_FetchInv_EInvX = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_FetchInv_EInvX");
        stat_stateEvent_FetchInv_MInv = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_FetchInv_MInv");
        stat_stateEvent_FetchInv_MInvX = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_FetchInv_MInvX");
        stat_stateEvent_FetchInv_SD = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_FetchInv_SD");
        stat_stateEvent_FetchInv_ED = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_FetchInv_ED");
        stat_stateEvent_FetchInv_MD = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_FetchInv_MD");
        stat_stateEvent_FetchInvX_I = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_FetchInvX_I");
        stat_stateEvent_FetchInvX_IS = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_FetchInvX_IS");
        stat_stateEvent_FetchInvX_IM = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_FetchInvX_IM");
        stat_stateEvent_FetchInvX_SM = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_FetchInvX_SM");
        stat_stateEvent_FetchInvX_E = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_FetchInvX_E");
        stat_stateEvent_FetchInvX_M = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_FetchInvX_M");
        stat_stateEvent_FetchInvX_EI = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_FetchInvX_EI");
        stat_stateEvent_FetchInvX_MI = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_FetchInvX_MI");
        stat_stateEvent_FetchInvX_EInv = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_FetchInvX_EInv");
        stat_stateEvent_FetchInvX_EInvX = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_FetchInvX_EInvX");
        stat_stateEvent_FetchInvX_MInv = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_FetchInvX_MInv");
        stat_stateEvent_FetchInvX_MInvX = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_FetchInvX_MInvX");
        stat_stateEvent_FetchInvX_ED = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_FetchInvX_ED");
        stat_stateEvent_FetchInvX_MD = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_FetchInvX_MD");
        stat_stateEvent_Fetch_I = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_Fetch_I");
        stat_stateEvent_Fetch_IS = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_Fetch_IS");
        stat_stateEvent_Fetch_IM = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_Fetch_IM");
        stat_stateEvent_Fetch_SM = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_Fetch_SM");
        stat_stateEvent_Fetch_S = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_Fetch_S");
        stat_stateEvent_Fetch_SInv = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_Fetch_SInv");
        stat_stateEvent_Fetch_SI = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_Fetch_SI");
        stat_stateEvent_Fetch_SD = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_Fetch_SD");
        stat_stateEvent_FetchResp_I = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_FetchResp_I");
        stat_stateEvent_FetchResp_SI = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_FetchResp_SI");
        stat_stateEvent_FetchResp_EI = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_FetchResp_EI");
        stat_stateEvent_FetchResp_MI = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_FetchResp_MI");
        stat_stateEvent_FetchResp_SInv = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_FetchResp_SInv");
        stat_stateEvent_FetchResp_SMInv = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_FetchResp_SMInv");
        stat_stateEvent_FetchResp_EInv = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_FetchResp_EInv");
        stat_stateEvent_FetchResp_MInv = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_FetchResp_MInv");
        stat_stateEvent_FetchResp_SD = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_FetchResp_SD");
        stat_stateEvent_FetchResp_SMD = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_FetchResp_SMD");
        stat_stateEvent_FetchResp_ED = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_FetchResp_ED");
        stat_stateEvent_FetchResp_MD = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_FetchResp_MD");
        stat_stateEvent_FetchXResp_I = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_FetchXResp_I");
        stat_stateEvent_FetchXResp_EInvX = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_FetchXResp_EInvX");
        stat_stateEvent_FetchXResp_MInvX = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_FetchXResp_MInvX");
        stat_stateEvent_AckInv_I = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_AckInv_I");
        stat_stateEvent_AckInv_SInv = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_AckInv_SInv");
        stat_stateEvent_AckInv_SMInv = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_AckInv_SMInv");
        stat_stateEvent_AckInv_SI = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_AckInv_SI");
        stat_stateEvent_AckInv_EI = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_AckInv_EI");
        stat_stateEvent_AckInv_MI = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_AckInv_MI");
        stat_stateEvent_AckInv_EInv = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_AckInv_EInv");
        stat_stateEvent_AckInv_MInv = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_AckInv_MInv");
        stat_stateEvent_AckPut_I = ((Component *)owner_)->registerStatistic<uint64_t>("stateEvent_AckPut_I");
        
        stat_latency_GetS_IS = ((Component *)owner_)->registerStatistic<uint64_t>("latency_GetS_IS");
        stat_latency_GetS_M = ((Component *)owner_)->registerStatistic<uint64_t>("latency_GetS_M");
        stat_latency_GetX_IM = ((Component *)owner_)->registerStatistic<uint64_t>("latency_GetX_IM");
        stat_latency_GetX_SM = ((Component *)owner_)->registerStatistic<uint64_t>("latency_GetX_SM");
        stat_latency_GetX_M = ((Component *)owner_)->registerStatistic<uint64_t>("latency_GetX_M");
        stat_latency_GetSEx_IM = ((Component *)owner_)->registerStatistic<uint64_t>("latency_GetSEx_IM");
        stat_latency_GetSEx_SM = ((Component *)owner_)->registerStatistic<uint64_t>("latency_GetSEx_SM");
        stat_latency_GetSEx_M = ((Component *)owner_)->registerStatistic<uint64_t>("latency_GetSEx_M");

        stat_eventSent_GetS = ((Component *)owner_)->registerStatistic<uint64_t>("eventSent_GetS");
        stat_eventSent_GetX = ((Component *)owner_)->registerStatistic<uint64_t>("eventSent_GetX");
        stat_eventSent_GetSEx = ((Component *)owner_)->registerStatistic<uint64_t>("eventSent_GetSEx");
        stat_eventSent_GetSResp = ((Component *)owner_)->registerStatistic<uint64_t>("eventSent_GetSResp");
        stat_eventSent_GetXResp = ((Component *)owner_)->registerStatistic<uint64_t>("eventSent_GetXResp");
        stat_eventSent_PutS = ((Component *)owner_)->registerStatistic<uint64_t>("eventSent_PutS");
        stat_eventSent_PutE = ((Component *)owner_)->registerStatistic<uint64_t>("eventSent_PutE");
        stat_eventSent_PutM = ((Component *)owner_)->registerStatistic<uint64_t>("eventSent_PutM");
        stat_eventSent_Inv = ((Component *)owner_)->registerStatistic<uint64_t>("eventSent_Inv");
        stat_eventSent_Fetch = ((Component *)owner_)->registerStatistic<uint64_t>("eventSent_Fetch");
        stat_eventSent_FetchInv = ((Component *)owner_)->registerStatistic<uint64_t>("eventSent_FetchInv");
        stat_eventSent_FetchInvX = ((Component *)owner_)->registerStatistic<uint64>("eventSent_FetchInvX");
        stat_eventSent_FetchResp = ((Component *)owner_)->registerStatistic<uint64_t>("eventSent_FetchResp");
        stat_eventSent_FetchXResp = ((Component *)owner_)->registerStatistic<uint64_t>("eventSent_FetchXResp");
        stat_eventSent_AckInv = ((Component *)owner_)->registerStatistic<uint64_t>("eventSent_AckInv");
        stat_eventSent_AckPut = ((Component *)owner_)->registerStatistic<uint64_t>("eventSent_AckPut");
        stat_eventSent_NACK_up = ((Component *)owner_)->registerStatistic<uint64_t>("eventSent_NACK_up");
        stat_eventSent_NACK_down = ((Component *)owner_)->registerStatistic<uint64_t>("eventSent_NACK_down");
        
        stat_eventStalledForLock = ((Component *)owner_)->registerStatistic<uint64_t>("EventStalledForLockedCacheline");
}
   
    ~CoherencyController(){}
    
    // Protected members of the coherenceController class
    const Cache*    owner_;
    CacheListener*  listener_;

    Output*         d_;
    uint            lineSize_;
    bool            writebackCleanBlocks_;
    
    vector<Link*>*  lowNetPorts_;
    Link*           highNetPort_;
    vector<string>  lowerLevelCacheNames_;
    vector<string>  upperLevelCacheNames_;


    // Statistics //
    
    int             sentEvents_;
    
    // Eviction statistics, count how many times we attempted to evict a block in a particular state
    Statistic<uint64_t>* stat_evict_I;
    Statistic<uint64_t>* stat_evict_S;
    Statistic<uint64_t>* stat_evict_E;
    Statistic<uint64_t>* stat_evict_M;
    Statistic<uint64_t>* stat_evict_IS;
    Statistic<uint64_t>* stat_evict_IM;
    Statistic<uint64_t>* stat_evict_SM;
    Statistic<uint64_t>* stat_evict_SInv;
    Statistic<uint64_t>* stat_evict_EInv;
    Statistic<uint64_t>* stat_evict_MInv;
    Statistic<uint64_t>* stat_evict_SMInv;
    Statistic<uint64_t>* stat_evict_EInvX;
    Statistic<uint64_t>* stat_evict_MInvX;
    Statistic<uint64_t>* stat_evict_SI;

    // State/event combinations for Stats API TODO is there a cleaner way to enumerate & declare these?
    Statistic<uint64_t>* stat_stateEvent_GetS_I;
    Statistic<uint64_t>* stat_stateEvent_GetS_S;
    Statistic<uint64_t>* stat_stateEvent_GetS_E;
    Statistic<uint64_t>* stat_stateEvent_GetS_M;
    Statistic<uint64_t>* stat_stateEvent_GetX_I;
    Statistic<uint64_t>* stat_stateEvent_GetX_S;
    Statistic<uint64_t>* stat_stateEvent_GetX_E;
    Statistic<uint64_t>* stat_stateEvent_GetX_M;
    Statistic<uint64_t>* stat_stateEvent_GetSEx_I;
    Statistic<uint64_t>* stat_stateEvent_GetSEx_S;
    Statistic<uint64_t>* stat_stateEvent_GetSEx_E;
    Statistic<uint64_t>* stat_stateEvent_GetSEx_M;
    Statistic<uint64_t>* stat_stateEvent_GetSResp_IS;
    Statistic<uint64_t>* stat_stateEvent_GetSResp_IM;
    Statistic<uint64_t>* stat_stateEvent_GetSResp_SM;
    Statistic<uint64_t>* stat_stateEvent_GetSResp_SMInv;
    Statistic<uint64_t>* stat_stateEvent_GetXResp_IM;
    Statistic<uint64_t>* stat_stateEvent_GetXResp_SM;
    Statistic<uint64_t>* stat_stateEvent_GetXResp_SMInv;
    Statistic<uint64_t>* stat_stateEvent_PutS_I;
    Statistic<uint64_t>* stat_stateEvent_PutS_S;
    Statistic<uint64_t>* stat_stateEvent_PutS_E;
    Statistic<uint64_t>* stat_stateEvent_PutS_M;
    Statistic<uint64_t>* stat_stateEvent_PutS_SD;
    Statistic<uint64_t>* stat_stateEvent_PutS_ED;
    Statistic<uint64_t>* stat_stateEvent_PutS_MD;
    Statistic<uint64_t>* stat_stateEvent_PutS_SMD;
    Statistic<uint64_t>* stat_stateEvent_PutS_SInv;
    Statistic<uint64_t>* stat_stateEvent_PutS_SI;
    Statistic<uint64_t>* stat_stateEvent_PutS_EI;
    Statistic<uint64_t>* stat_stateEvent_PutS_MI;
    Statistic<uint64_t>* stat_stateEvent_PutS_EInvX;
    Statistic<uint64_t>* stat_stateEvent_PutS_EInv;
    Statistic<uint64_t>* stat_stateEvent_PutS_MInv;
    Statistic<uint64_t>* stat_stateEvent_PutS_SMInv;
    Statistic<uint64_t>* stat_stateEvent_PutE_I;
    Statistic<uint64_t>* stat_stateEvent_PutE_E;
    Statistic<uint64_t>* stat_stateEvent_PutE_M;
    Statistic<uint64_t>* stat_stateEvent_PutE_EI;
    Statistic<uint64_t>* stat_stateEvent_PutE_MI;
    Statistic<uint64_t>* stat_stateEvent_PutE_EInv;
    Statistic<uint64_t>* stat_stateEvent_PutE_EInvX;
    Statistic<uint64_t>* stat_stateEvent_PutE_MInv;
    Statistic<uint64_t>* stat_stateEvent_PutE_MInvX;
    Statistic<uint64_t>* stat_stateEvent_PutM_I;
    Statistic<uint64_t>* stat_stateEvent_PutM_E;
    Statistic<uint64_t>* stat_stateEvent_PutM_M;
    Statistic<uint64_t>* stat_stateEvent_PutM_EI;
    Statistic<uint64_t>* stat_stateEvent_PutM_MI;
    Statistic<uint64_t>* stat_stateEvent_PutM_EInv;
    Statistic<uint64_t>* stat_stateEvent_PutM_EInvX;
    Statistic<uint64_t>* stat_stateEvent_PutM_MInv;
    Statistic<uint64_t>* stat_stateEvent_PutM_MInvX;
    Statistic<uint64_t>* stat_stateEvent_Inv_I;
    Statistic<uint64_t>* stat_stateEvent_Inv_IS;
    Statistic<uint64_t>* stat_stateEvent_Inv_IM;
    Statistic<uint64_t>* stat_stateEvent_Inv_S;
    Statistic<uint64_t>* stat_stateEvent_Inv_SM;
    Statistic<uint64_t>* stat_stateEvent_Inv_SInv;
    Statistic<uint64_t>* stat_stateEvent_Inv_SI;
    Statistic<uint64_t>* stat_stateEvent_Inv_SMInv;
    Statistic<uint64_t>* stat_stateEvent_Inv_SD;
    Statistic<uint64_t>* stat_stateEvent_FetchInv_I;
    Statistic<uint64_t>* stat_stateEvent_FetchInv_IS;
    Statistic<uint64_t>* stat_stateEvent_FetchInv_IM;
    Statistic<uint64_t>* stat_stateEvent_FetchInv_SM;
    Statistic<uint64_t>* stat_stateEvent_FetchInv_S;
    Statistic<uint64_t>* stat_stateEvent_FetchInv_E;
    Statistic<uint64_t>* stat_stateEvent_FetchInv_M;
    Statistic<uint64_t>* stat_stateEvent_FetchInv_EI;
    Statistic<uint64_t>* stat_stateEvent_FetchInv_MI;
    Statistic<uint64_t>* stat_stateEvent_FetchInv_EInv;
    Statistic<uint64_t>* stat_stateEvent_FetchInv_EInvX;
    Statistic<uint64_t>* stat_stateEvent_FetchInv_MInv;
    Statistic<uint64_t>* stat_stateEvent_FetchInv_MInvX;
    Statistic<uint64_t>* stat_stateEvent_FetchInv_SD;
    Statistic<uint64_t>* stat_stateEvent_FetchInv_ED;
    Statistic<uint64_t>* stat_stateEvent_FetchInv_MD;
    Statistic<uint64_t>* stat_stateEvent_FetchInvX_I;
    Statistic<uint64_t>* stat_stateEvent_FetchInvX_IS;
    Statistic<uint64_t>* stat_stateEvent_FetchInvX_IM;
    Statistic<uint64_t>* stat_stateEvent_FetchInvX_SM;
    Statistic<uint64_t>* stat_stateEvent_FetchInvX_E;
    Statistic<uint64_t>* stat_stateEvent_FetchInvX_M;
    Statistic<uint64_t>* stat_stateEvent_FetchInvX_EI;
    Statistic<uint64_t>* stat_stateEvent_FetchInvX_MI;
    Statistic<uint64_t>* stat_stateEvent_FetchInvX_EInv;
    Statistic<uint64_t>* stat_stateEvent_FetchInvX_EInvX;
    Statistic<uint64_t>* stat_stateEvent_FetchInvX_MInv;
    Statistic<uint64_t>* stat_stateEvent_FetchInvX_MInvX;
    Statistic<uint64_t>* stat_stateEvent_FetchInvX_ED;
    Statistic<uint64_t>* stat_stateEvent_FetchInvX_MD;
    Statistic<uint64_t>* stat_stateEvent_Fetch_I;
    Statistic<uint64_t>* stat_stateEvent_Fetch_IS;
    Statistic<uint64_t>* stat_stateEvent_Fetch_IM;
    Statistic<uint64_t>* stat_stateEvent_Fetch_SM;
    Statistic<uint64_t>* stat_stateEvent_Fetch_S;
    Statistic<uint64_t>* stat_stateEvent_Fetch_SInv;
    Statistic<uint64_t>* stat_stateEvent_Fetch_SI;
    Statistic<uint64_t>* stat_stateEvent_Fetch_SD;
    Statistic<uint64_t>* stat_stateEvent_FetchResp_I;
    Statistic<uint64_t>* stat_stateEvent_FetchResp_SI;
    Statistic<uint64_t>* stat_stateEvent_FetchResp_EI;
    Statistic<uint64_t>* stat_stateEvent_FetchResp_MI;
    Statistic<uint64_t>* stat_stateEvent_FetchResp_SInv;
    Statistic<uint64_t>* stat_stateEvent_FetchResp_SMInv;
    Statistic<uint64_t>* stat_stateEvent_FetchResp_EInv;
    Statistic<uint64_t>* stat_stateEvent_FetchResp_MInv;
    Statistic<uint64_t>* stat_stateEvent_FetchResp_SD;
    Statistic<uint64_t>* stat_stateEvent_FetchResp_SMD;
    Statistic<uint64_t>* stat_stateEvent_FetchResp_ED;
    Statistic<uint64_t>* stat_stateEvent_FetchResp_MD;
    Statistic<uint64_t>* stat_stateEvent_FetchXResp_I;
    Statistic<uint64_t>* stat_stateEvent_FetchXResp_EInvX;
    Statistic<uint64_t>* stat_stateEvent_FetchXResp_MInvX;
    Statistic<uint64_t>* stat_stateEvent_AckInv_I;
    Statistic<uint64_t>* stat_stateEvent_AckInv_SInv;
    Statistic<uint64_t>* stat_stateEvent_AckInv_EInv;
    Statistic<uint64_t>* stat_stateEvent_AckInv_MInv;
    Statistic<uint64_t>* stat_stateEvent_AckInv_SMInv;
    Statistic<uint64_t>* stat_stateEvent_AckInv_SI;
    Statistic<uint64_t>* stat_stateEvent_AckInv_EI;
    Statistic<uint64_t>* stat_stateEvent_AckInv_MI;
    Statistic<uint64_t>* stat_stateEvent_AckPut_I;

    Statistic<uint64_t>* stat_latency_GetS_IS;
    Statistic<uint64_t>* stat_latency_GetS_M;
    Statistic<uint64_t>* stat_latency_GetX_IM;
    Statistic<uint64_t>* stat_latency_GetX_SM;
    Statistic<uint64_t>* stat_latency_GetX_M;
    Statistic<uint64_t>* stat_latency_GetSEx_IM;
    Statistic<uint64_t>* stat_latency_GetSEx_SM;
    Statistic<uint64_t>* stat_latency_GetSEx_M;
   
    // Count events sent
    Statistic<uint64_t>* stat_eventSent_GetS;
    Statistic<uint64_t>* stat_eventSent_GetX;
    Statistic<uint64_t>* stat_eventSent_GetSEx;
    Statistic<uint64_t>* stat_eventSent_GetSResp;
    Statistic<uint64_t>* stat_eventSent_GetXResp;
    Statistic<uint64_t>* stat_eventSent_PutS;
    Statistic<uint64_t>* stat_eventSent_PutE;
    Statistic<uint64_t>* stat_eventSent_PutM;
    Statistic<uint64_t>* stat_eventSent_Inv;
    Statistic<uint64_t>* stat_eventSent_Fetch;
    Statistic<uint64_t>* stat_eventSent_FetchInv;
    Statistic<uint64_t>* stat_eventSent_FetchInvX;
    Statistic<uint64_t>* stat_eventSent_FetchResp;
    Statistic<uint64_t>* stat_eventSent_FetchXResp;
    Statistic<uint64_t>* stat_eventSent_AckInv;
    Statistic<uint64_t>* stat_eventSent_AckPut;
    Statistic<uint64_t>* stat_eventSent_NACK_up;
    Statistic<uint64_t>* stat_eventSent_NACK_down;


    Statistic<uint64_t>* stat_eventStalledForLock;
    
    // General protected methods

    // For distributed caches, return which cache is home for a particular address
    string getDestination(Addr baseAddr) {
        if (lowerLevelCacheNames_.size() == 1) {
            return lowerLevelCacheNames_.front();
        } else if (lowerLevelCacheNames_.size() > 1) {
            // round robin for now
            int index = (baseAddr/lineSize_) % lowerLevelCacheNames_.size();
            return lowerLevelCacheNames_[index];
        } else {
            return "";
        }
    }

    // Statistics protected methods
    
    void recordStateEventCount(Command cmd, State state) {
        switch (cmd) {
            case GetS:
                if (state == I) stat_stateEvent_GetS_I->addData(1);
                else if (state == S) stat_stateEvent_GetS_S->addData(1);
                else if (state == E) stat_stateEvent_GetS_E->addData(1);
                else if (state == M) stat_stateEvent_GetS_M->addData(1);
                break;
            case GetX:
                if (state == I) stat_stateEvent_GetX_I->addData(1);
                else if (state == S) stat_stateEvent_GetX_S->addData(1);
                else if (state == E) stat_stateEvent_GetX_E->addData(1);
                else if (state == M) stat_stateEvent_GetX_M->addData(1);
                break;
            case GetSEx:
                if (state == I) stat_stateEvent_GetSEx_I->addData(1);
                else if (state == S) stat_stateEvent_GetSEx_S->addData(1);
                else if (state == E) stat_stateEvent_GetSEx_E->addData(1);
                else if (state == M) stat_stateEvent_GetSEx_M->addData(1);
                break;
            case GetSResp:
                if (state == IS) stat_stateEvent_GetSResp_IS->addData(1);
                else if (state == IM) stat_stateEvent_GetSResp_IM->addData(1);
                else if (state == SM) stat_stateEvent_GetSResp_SM->addData(1);
                else if (state == SM_Inv) stat_stateEvent_GetSResp_SMInv->addData(1);
                break;
            case GetXResp:
                if (state == IM) stat_stateEvent_GetXResp_IM->addData(1);
                else if (state == SM) stat_stateEvent_GetXResp_SM->addData(1);
                else if (state == SM_Inv) stat_stateEvent_GetXResp_SMInv->addData(1);
                break;
            case PutS:
                if (state == I) stat_stateEvent_PutS_I->addData(1);
                else if (state == S) stat_stateEvent_PutS_S->addData(1);
                else if (state == E) stat_stateEvent_PutS_E->addData(1);
                else if (state == M) stat_stateEvent_PutS_M->addData(1);
                else if (state == S_D) stat_stateEvent_PutS_SD->addData(1);
                else if (state == E_D) stat_stateEvent_PutS_ED->addData(1);
                else if (state == M_D) stat_stateEvent_PutS_MD->addData(1);
                else if (state == SM_D) stat_stateEvent_PutS_SMD->addData(1);
                else if (state == S_Inv) stat_stateEvent_PutS_SInv->addData(1);
                else if (state == SI) stat_stateEvent_PutS_SI->addData(1);
                else if (state == EI) stat_stateEvent_PutS_EI->addData(1);
                else if (state == MI) stat_stateEvent_PutS_MI->addData(1);
                else if (state == E_Inv) stat_stateEvent_PutS_EInv->addData(1);
                else if (state == E_InvX) stat_stateEvent_PutS_EInvX->addData(1);
                else if (state == M_Inv) stat_stateEvent_PutS_MInv->addData(1);
                else if (state == SM_Inv) stat_stateEvent_PutS_SMInv->addData(1);
                break;
            case PutE:
                if (state == I) stat_stateEvent_PutE_I->addData(1);
                else if (state == E) stat_stateEvent_PutE_E->addData(1);
                else if (state == M) stat_stateEvent_PutE_M->addData(1);
                else if (state == EI) stat_stateEvent_PutE_EI->addData(1);
                else if (state == MI) stat_stateEvent_PutE_MI->addData(1);
                else if (state == E_Inv) stat_stateEvent_PutE_EInv->addData(1);
                else if (state == M_Inv) stat_stateEvent_PutE_MInv->addData(1);
                else if (state == E_InvX) stat_stateEvent_PutE_EInvX->addData(1);
                else if (state == M_InvX) stat_stateEvent_PutE_MInvX->addData(1);
                break;
            case PutM:
                if (state == I) stat_stateEvent_PutM_I->addData(1);
                else if (state == E) stat_stateEvent_PutM_E->addData(1);
                else if (state == M) stat_stateEvent_PutM_M->addData(1);
                else if (state == EI) stat_stateEvent_PutM_EI->addData(1);
                else if (state == MI) stat_stateEvent_PutM_MI->addData(1);
                else if (state == E_Inv) stat_stateEvent_PutM_EInv->addData(1);
                else if (state == M_Inv) stat_stateEvent_PutM_MInv->addData(1);
                else if (state == E_InvX) stat_stateEvent_PutM_EInvX->addData(1);
                else if (state == M_InvX) stat_stateEvent_PutM_MInvX->addData(1);
                break;
            case Inv:
                if (state == I) stat_stateEvent_Inv_I->addData(1);
                else if (state == IS) stat_stateEvent_Inv_IS->addData(1);
                else if (state == IM) stat_stateEvent_Inv_IM->addData(1);
                else if (state == S) stat_stateEvent_Inv_S->addData(1);
                else if (state == SM) stat_stateEvent_Inv_SM->addData(1);
                else if (state == S_Inv) stat_stateEvent_Inv_SInv->addData(1);
                else if (state == SI) stat_stateEvent_Inv_SI->addData(1);
                else if (state == SM_Inv) stat_stateEvent_Inv_SMInv->addData(1);
                else if (state == S_D) stat_stateEvent_Inv_SD->addData(1);
                break;
            case FetchInv:
                if (state == I) stat_stateEvent_FetchInv_I->addData(1);
                else if (state == IS) stat_stateEvent_FetchInv_IS->addData(1);
                else if (state == IM) stat_stateEvent_FetchInv_IM->addData(1);
                else if (state == SM) stat_stateEvent_FetchInv_SM->addData(1);
                else if (state == S) stat_stateEvent_FetchInv_S->addData(1);
                else if (state == E) stat_stateEvent_FetchInv_E->addData(1);
                else if (state == M) stat_stateEvent_FetchInv_M->addData(1);
                else if (state == EI) stat_stateEvent_FetchInv_EI->addData(1);
                else if (state == MI) stat_stateEvent_FetchInv_MI->addData(1);
                else if (state == E_Inv) stat_stateEvent_FetchInv_EInv->addData(1);
                else if (state == E_InvX) stat_stateEvent_FetchInv_EInvX->addData(1);
                else if (state == M_Inv) stat_stateEvent_FetchInv_MInv->addData(1);
                else if (state == M_InvX) stat_stateEvent_FetchInv_MInvX->addData(1);
                else if (state == S_D) stat_stateEvent_FetchInv_SD->addData(1);
                else if (state == E_D) stat_stateEvent_FetchInv_ED->addData(1);
                else if (state == M_D) stat_stateEvent_FetchInv_MD->addData(1);
                break;
            case FetchInvX:
                if (state == I) stat_stateEvent_FetchInvX_I->addData(1);
                else if (state == IS) stat_stateEvent_FetchInvX_IS->addData(1);
                else if (state == IM) stat_stateEvent_FetchInvX_IM->addData(1);
                else if (state == SM) stat_stateEvent_FetchInvX_SM->addData(1);
                else if (state == E) stat_stateEvent_FetchInvX_E->addData(1);
                else if (state == M) stat_stateEvent_FetchInvX_M->addData(1);
                else if (state == EI) stat_stateEvent_FetchInvX_EI->addData(1);
                else if (state == MI) stat_stateEvent_FetchInvX_MI->addData(1);
                else if (state == E_Inv) stat_stateEvent_FetchInvX_EInv->addData(1);
                else if (state == E_InvX) stat_stateEvent_FetchInvX_EInvX->addData(1);
                else if (state == M_Inv) stat_stateEvent_FetchInvX_MInv->addData(1);
                else if (state == M_InvX) stat_stateEvent_FetchInvX_MInvX->addData(1);
                else if (state == E_D) stat_stateEvent_FetchInvX_ED->addData(1);
                else if (state == M_D) stat_stateEvent_FetchInvX_MD->addData(1);
                break;
            case Fetch:
                if (state == I) stat_stateEvent_Fetch_I->addData(1);
                else if (state == IS) stat_stateEvent_Fetch_IS->addData(1);
                else if (state == IM) stat_stateEvent_Fetch_IM->addData(1);
                else if (state == S) stat_stateEvent_Fetch_S->addData(1);
                else if (state == SM) stat_stateEvent_Fetch_SM->addData(1);
                else if (state == S_Inv) stat_stateEvent_Fetch_SInv->addData(1);
                else if (state == SI) stat_stateEvent_Fetch_SI->addData(1);
                else if (state == S_D) stat_stateEvent_Fetch_SD->addData(1);
                break;
            case FetchResp:
                if (state == I) stat_stateEvent_FetchResp_I->addData(1);
                else if (state == EI) stat_stateEvent_FetchResp_SI->addData(1);
                else if (state == EI) stat_stateEvent_FetchResp_EI->addData(1);
                else if (state == MI) stat_stateEvent_FetchResp_MI->addData(1);
                else if (state == S_Inv) stat_stateEvent_FetchResp_SInv->addData(1);
                else if (state == SM_Inv) stat_stateEvent_FetchResp_SMInv->addData(1);
                else if (state == E_Inv) stat_stateEvent_FetchResp_EInv->addData(1);
                else if (state == M_Inv) stat_stateEvent_FetchResp_MInv->addData(1);
                else if (state == S_D) stat_stateEvent_FetchResp_SD->addData(1);
                else if (state == SM_D) stat_stateEvent_FetchResp_SMD->addData(1);
                else if (state == E_D) stat_stateEvent_FetchResp_ED->addData(1);
                else if (state == M_D) stat_stateEvent_FetchResp_MD->addData(1);
                break;
            case FetchXResp:
                if (state == I) stat_stateEvent_FetchXResp_I->addData(1);
                else if (state == E_InvX) stat_stateEvent_FetchXResp_EInvX->addData(1);
                else if (state == M_InvX) stat_stateEvent_FetchXResp_MInvX->addData(1);
                break;
            case AckInv:
                if (state == I) stat_stateEvent_AckInv_I->addData(1);
                else if (state == SI) stat_stateEvent_AckInv_SI->addData(1);
                else if (state == EI) stat_stateEvent_AckInv_EI->addData(1);
                else if (state == MI) stat_stateEvent_AckInv_MI->addData(1);
                else if (state == S_Inv) stat_stateEvent_AckInv_SInv->addData(1);
                else if (state == E_Inv) stat_stateEvent_AckInv_EInv->addData(1);
                else if (state == M_Inv) stat_stateEvent_AckInv_MInv->addData(1);
                else if (state == SM_Inv) stat_stateEvent_AckInv_SMInv->addData(1);
                break;
            case AckPut:
                if (state == I) stat_stateEvent_AckPut_I->addData(1);
                break;
            default:
                break;
        } 
    }

    void recordEvictionState(State state) {
        switch (state) {
            case I:
                stat_evict_I->addData(1);
                break;
            case S:
                stat_evict_S->addData(1);
                break;
            case E:
                stat_evict_E->addData(1);
                break;
            case M:
                stat_evict_M->addData(1);
                break;
            case IS:
                stat_evict_IS->addData(1);
                break;
            case IM:
                stat_evict_IM->addData(1);
                break;
            case SM:
                stat_evict_SM->addData(1);
                break;
            case SI:
                stat_evict_SI->addData(1);
                break;
            case S_Inv:
                stat_evict_SInv->addData(1);
                break;
            case E_Inv:
                stat_evict_EInv->addData(1);
                break;
            case M_Inv:
                stat_evict_MInv->addData(1);
                break;
            case SM_Inv:
                stat_evict_SMInv->addData(1);
                break;
            case E_InvX:
                stat_evict_EInvX->addData(1);
                break;
            case M_InvX:
                stat_evict_MInvX->addData(1);
                break;
            default:
                break;
        }

    }

    void recordEventSentUp(Command cmd) {
        switch(cmd) {
            case GetSResp:
                stat_eventSent_GetSResp->addData(1);
                break;
            case GetXResp:
                stat_eventSent_GetXResp->addData(1);
                break;
            case Inv:
                stat_eventSent_Inv->addData(1);
                break;
            case Fetch:
                stat_eventSent_Fetch->addData(1);
                break;
            case FetchInv:
                stat_eventSent_FetchInv->addData(1);
                break;
            case FetchInvX:
                stat_eventSent_FetchInvX->addData(1);
                break;
            case AckPut:
                stat_eventSent_AckPut->addData(1);
                break;
            case NACK:
                stat_eventSent_NACK_up->addData(1);
                break;
            default: 
                break;
        }
    }

    void recordEventSentDown(Command cmd) {
        switch (cmd) {
            case GetS:
                stat_eventSent_GetS->addData(1);
                break;
            case GetX:
                stat_eventSent_GetX->addData(1);
                break;
            case GetSEx:
                stat_eventSent_GetSEx->addData(1);
                break;
            case PutS:
                stat_eventSent_PutS->addData(1);
                break;
            case PutE:
                stat_eventSent_PutE->addData(1);
                break;
            case PutM:
                stat_eventSent_PutM->addData(1);
                break;
            case FetchResp:
                stat_eventSent_FetchResp->addData(1);
                break;
            case FetchXResp:
                stat_eventSent_FetchXResp->addData(1);
                break;
            case AckInv:
                stat_eventSent_AckInv->addData(1);
                break;
            case NACK:
                stat_eventSent_NACK_down->addData(1);
                break;
            default: break;
        }
    }

    // Perform listener call back. 
    // These used to be replicated in each coherence class - they can be overwritten if needed
    virtual void notifyListenerOfAccess(MemEvent * event, NotifyAccessType accessT, NotifyResultType resultT) {
        if (!event->isPrefetch()) {
            CacheListenerNotification notify(event->getBaseAddr(), event->getVirtualAddress(),
                event->getInstructionPointer(), event->getSize(), accessT, resultT);
            listener_->notifyAccess(notify);
        }
    }


};




}}

#endif	/* COHERENCECONTROLLERS_H */
