// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2017, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef __SST_MEMH_MEMBACKENDCONVERTOR__
#define __SST_MEMH_MEMBACKENDCONVERTOR__

#include <sst/core/subcomponent.h>
#include "sst/elements/memHierarchy/memEvent.h"

namespace SST {
namespace MemHierarchy {

class MemBackend;

class MemBackendConvertor : public SubComponent {
  public:
    typedef uint64_t ReqId; 

    class MemReq {
      public:
        MemReq( MemEvent* event, uint32_t reqId  ) : m_event(event), m_respEvent( NULL),
            m_reqId(reqId), m_offset(0), m_numReq(0)
        { }
        ~MemReq() {
            delete m_event;
        }

        static uint32_t getBaseId( ReqId id) { return id >> 32; }
        Addr baseAddr() { return m_event->getBaseAddr(); }
        Addr addr()     { return m_event->getBaseAddr() + m_offset; }

        uint32_t processed()    { return m_offset; }
        uint64_t id()           { return ((uint64_t)m_reqId << 32) | m_offset; }
        MemEvent* getMemEvent() { return m_event; }
        bool isWrite()          { return (m_event->getCmd() == PutM || (m_event->queryFlag(MemEvent::F_NONCACHEABLE) && m_event->getCmd() == GetX)) ? true : false; }
        uint32_t size()         { return m_event->getSize(); }

        void setResponse( MemEvent* event ) { m_respEvent = event; }
        MemEvent* getResponse() { return m_respEvent; }

        void increment( uint32_t bytes ) {
            m_offset += bytes;
            ++m_numReq;
        }
        void decrement( ) { --m_numReq; }
        bool isDone( ) {
            return ( m_offset >= m_event->getSize() && 0 == m_numReq );
        }

      private:
        MemEvent*   m_event;
        MemEvent*   m_respEvent;
        uint32_t    m_reqId;
        uint32_t    m_offset;
        uint32_t    m_numReq;
    };

  public:

    MemBackendConvertor();
    MemBackendConvertor(Component* comp, Params& params);
    void finish(void);
    virtual const std::string& getClockFreq();
    virtual size_t getMemSize();
    virtual bool clock( Cycle_t cycle );
    virtual void handleMemEvent(  MemEvent* );

    virtual const std::string& getRequestor( ReqId reqId ) { 
        uint32_t id = MemReq::getBaseId(reqId);
        if ( m_pendingRequests.find( id ) == m_pendingRequests.end() ) {
            m_dbg.fatal(CALL_INFO, -1, "memory request not found\n");
        }

        return m_pendingRequests[id]->getMemEvent()->getRqstr();
    }

  protected:
    ~MemBackendConvertor() {
        while ( m_requestQueue.size()) {
            delete m_requestQueue.front();
            m_requestQueue.pop_front();
        }

        PendingRequests::iterator iter = m_pendingRequests.begin();
        for ( ; iter != m_pendingRequests.end(); ++ iter ) {
            delete iter->second;
        }
    }

    MemEvent* doResponse( ReqId reqId );
    void sendResponse( MemEvent* event );
    void sendFlushResponse( MemEvent* event );

    MemBackend* m_backend;
    uint32_t    m_backendRequestWidth;

  private:
    virtual bool issue(MemReq*) = 0;




    bool setupMemReq( MemEvent* ev ) {
        if ( FlushLine == ev->getCmd() || FlushLineInv == ev->getCmd() ) {
            // TODO optimize if this becomes a problem, it is slow
            std::unordered_set<MemEvent*> dependsOn;
            for (std::deque<MemReq*>::iterator it = m_requestQueue.begin(); it != m_requestQueue.end(); it++) {
                if ((*it)->baseAddr() == ev->getBaseAddr()) {
                    MemEvent * req = (*it)->getMemEvent();
                    dependsOn.insert(req);
                    if (m_dependentRequests.find(req) == m_dependentRequests.end()) {
                        std::unordered_set<MemEvent*> flushSet;
                        flushSet.insert(ev);
                        m_dependentRequests.insert(std::make_pair(req, flushSet));
                    } else {
                        (m_dependentRequests.find(req)->second).insert(ev);
                    }
                }
            }

            if (dependsOn.empty()) return false;
            m_waitingFlushes.insert(std::make_pair(ev, dependsOn));
            return true; 
        }

        uint32_t id = genReqId();
        MemReq* req = new MemReq( ev, id );
        m_requestQueue.push_back( req );
        m_pendingRequests[id] = req;
        return true;
    }

    void doClockStat( ) {
        totalCycles->addData(1);        
    }

    void doReceiveStat( Command cmd) {
        switch (cmd ) { 
            case GetS: 
                stat_GetSReqReceived->addData(1);
                break;
            case GetX:
                stat_GetXReqReceived->addData(1);
                break;
            case GetSEx:
                stat_GetSExReqReceived->addData(1);
                break;
            case PutM:
                stat_PutMReqReceived->addData(1);
                break;
            default:
                break;
        }
    }

    void doResponseStat( Command cmd, Cycle_t latency ) {
        switch (cmd) {
            case GetS:
                stat_GetSLatency->addData(latency);
                break;
            case GetSEx:
                stat_GetSExLatency->addData(latency);
                break;
            case GetX:
                stat_GetXLatency->addData(latency);
                break;
            case PutM:
                stat_PutMLatency->addData(latency);
                break;
            default:
                break;
        }
    }

    Output      m_dbg;

    uint64_t m_cycleCount;

    uint32_t genReqId( ) { return ++m_reqId; }

    uint32_t    m_reqId;

    typedef std::map<uint32_t,MemReq*>    PendingRequests;

    std::deque<MemReq*>     m_requestQueue;
    PendingRequests         m_pendingRequests;
    uint32_t                m_frontendRequestWidth;

    std::map<MemEvent*, std::unordered_set<MemEvent*> > m_waitingFlushes; // Set of request events for each flush
    std::map<MemEvent*, std::unordered_set<MemEvent*> > m_dependentRequests; // Reverse map, set of flushes for each request ID, for faster lookup

    Statistic<uint64_t>* stat_GetSLatency;
    Statistic<uint64_t>* stat_GetSExLatency;
    Statistic<uint64_t>* stat_GetXLatency;
    Statistic<uint64_t>* stat_PutMLatency;

    Statistic<uint64_t>* stat_GetSReqReceived;
    Statistic<uint64_t>* stat_GetXReqReceived;
    Statistic<uint64_t>* stat_PutMReqReceived;
    Statistic<uint64_t>* stat_GetSExReqReceived;

    Statistic<uint64_t>* cyclesWithIssue;
    Statistic<uint64_t>* cyclesAttemptIssueButRejected;
    Statistic<uint64_t>* totalCycles;
    Statistic<uint64_t>* stat_outstandingReqs;

};

}
}
#endif
