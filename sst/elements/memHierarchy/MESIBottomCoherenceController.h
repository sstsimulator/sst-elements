/*
 * File:   MESIBottomCoherenceController.h
 * Author: Caesar De la Paz III
 * Email:  caesar.sst@gmail.com
 */

#ifndef MESIBOTTOMCOHERENCECONTROLLERS_H
#define MESIBOTTOMCOHERENCECONTROLLERS_H

#include <iostream>

namespace SST { namespace MemHierarchy {

class MESIBottomCC : public CoherencyController{
private:
    vector<Link*>* parentLinks_;

    CacheListener* listener_;
    uint GETSMissIS_;
    uint GETXMissSM_;
    uint GETXMissIM_;
    uint GETSHit_;
    uint GETXHit_;
    uint PUTSReqsReceived_;
    uint PUTEReqsReceived_;
    uint PUTMReqsReceived_;
    uint EvictionPUTSReqSent_;
    uint EvictionPUTMReqSent_;
    uint InvalidatePUTMReqSent_;
    uint FetchInvalidateReqSent_;
    uint FetchInvalidateXReqSent_;
    string ownerName_;
    string nextLevelCacheName_;

    
    void inc_GETXMissSM(Addr addr, bool pf);
    void inc_GETXMissIM(Addr addr, bool pf);
    void inc_GETSHit(Addr addr, bool pf);
    void inc_GETXHit(Addr addr, bool pf);
    void inc_GETSMissIS(Addr addr, bool pf);
    bool isExclusive(CacheLine* cacheLine);
    
    unsigned int getParentId(CacheLine* wbCacheLine);
    unsigned int getParentId(Addr baseAddr);

public:
    MESIBottomCC(const SST::MemHierarchy::Cache* _cache, string _ownerName, Output* _dbg,
                 vector<Link*>* _parentLinks, CacheListener* _listener,
                 unsigned int _lineSize, uint64 _accessLatency, bool _L1, MemNIC* _directoryLink) :
                 CoherencyController(_cache, _dbg, _lineSize), parentLinks_(_parentLinks),
                 listener_(_listener), ownerName_(_ownerName) {
        d_->debug(_INFO_,"--------------------------- Initializing [BottomCC] ... \n\n");
        GETSMissIS_            = 0;
        GETXMissSM_            = 0;
        GETXMissIM_            = 0;
        GETSHit_               = 0;
        GETXHit_               = 0;
        PUTSReqsReceived_      = 0;
        PUTEReqsReceived_      = 0;
        PUTMReqsReceived_      = 0;
        EvictionPUTSReqSent_   = 0;
        EvictionPUTMReqSent_   = 0;
        InvalidatePUTMReqSent_ = 0;
        
        L1_ = _L1;
        accessLatency_ = _accessLatency;
        directoryLink_ = _directoryLink;
    }
   
    void sendOutgoingCommands(){
        while(!outgoingEventQueue_.empty() && outgoingEventQueue_.front().deliveryTime <= timestamp_) {
            if(directoryLink_) directoryLink_->send(outgoingEventQueue_.front().event);
            else parentLinks_->at(0)->send(outgoingEventQueue_.front().event);
            outgoingEventQueue_.pop();
        }
    }

    
    void init(const char* name){}
    void handleEviction(MemEvent* event, CacheLine* wbCacheLine);
    void handleAccess(MemEvent* event, CacheLine* cacheLine, Command cmd);
    void handleAccessAck(MemEvent* ackEvent, CacheLine* cacheLine, const vector<mshrType*> mshrEntry);
    void handleWritebackOnAccess(Addr lineAddr, CacheLine* cacheLine, Command type);
    void handleInvalidate(MemEvent *event, CacheLine* cacheLine, Command cmd);
    void handleFetch(MemEvent *event, CacheLine* cacheLine, int _parentId);
    void handleFetchInvalidate(MemEvent* _event, CacheLine* _cacheLine, int _parentId);
    void handlePutAck(MemEvent* event, CacheLine* cacheLine);
    void printStats(int _stats, uint64 _GetSExReceived, uint64 _invalidateWaitingForUserLock, uint64 _totalInstReceived, uint64 _nonCoherenceReqsReceived);
    void forwardMessage(MemEvent* _event, CacheLine* cacheLine, vector<uint8_t>* _data);
    void forwardMessage(MemEvent* _event, Addr _baseAddr, unsigned int _lineSize, vector<uint8_t>* _data);
    bool modifiedStateNeeded(MemEvent* _event, CacheLine* _cacheLine);
    void processGetXRequest(MemEvent* event, CacheLine* cacheLine, Command cmd);
    void processGetSRequest(MemEvent* event, CacheLine* cacheLine);
    void processPutMRequest(MemEvent* event, CacheLine* cacheLine);
    void processPutERequest(MemEvent* event, CacheLine* cacheLine);
    void processInvRequest(MemEvent* event, CacheLine* cacheLine);
    void processInvXRequest(MemEvent* event, CacheLine* cacheLine);
    void updateEvictionStats(BCC_MESIState _state);
    bool canInvalidateRequestProceed(MemEvent* _event, CacheLine* _cacheLine, bool _sendAcks);
    void sendResponse(MemEvent* _event, CacheLine* _cacheLine, int _parentId);
    void sendWriteback(Command cmd, CacheLine* cacheLine, Link* deliveryLink);
    bool sendAckResponse(MemEvent *event);
    void setNextLevelCache(string _nlc){ nextLevelCacheName_ = _nlc; }
};


}}


#endif

