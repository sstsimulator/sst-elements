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


#ifndef __SST_MEMH_SCRATCHBACKENDCONVERTOR__
#define __SST_MEMH_SCRATCHBACKENDCONVERTOR__

#include <sst/core/subcomponent.h>
#include "sst/elements/memHierarchy/scratchEvent.h"

/*
 *  Converts ScratchEvent to membackend interface
 *
 */

namespace SST {
namespace MemHierarchy {

class MemBackend;

class ScratchBackendConvertor : public SubComponent {
  public:
    typedef uint64_t ReqId; 

    class ScratchReq {
      public:
        ScratchReq( ScratchEvent* event, uint32_t reqId, uint64_t time  ) : m_event(event),
            m_reqId(reqId), m_offset(0), m_numReq(0), m_deliveryTime(time)
        { }
        ~ScratchReq() {
            delete m_event;
        }

        static uint32_t getBaseId( ReqId id) { return id >> 32; }
        Addr baseAddr() { return m_event->getBaseAddr(); }
        Addr addr()     { return m_event->getBaseAddr() + m_offset; }

        uint32_t processed()    { return m_offset; }
        uint64_t id()           { return ((uint64_t)m_reqId << 32) | m_offset; }
        ScratchEvent* getScratchEvent() { return m_event; }
        bool isWrite()          { return (m_event->getCmd() == Write) ? true : false; }

        uint64_t getDeliveryTime() { return m_deliveryTime; }

        void increment( uint32_t bytes ) {
            m_offset += bytes;
            ++m_numReq;
        }
        void decrement( ) { --m_numReq; }
        bool isDone( ) {
            return ( m_offset >= m_event->getSize() && 0 == m_numReq );
        }

      private:
        ScratchEvent*   m_event;
        uint32_t        m_reqId;
        uint32_t        m_offset;
        uint32_t        m_numReq;
        uint64_t        m_deliveryTime;
    };

  public:

    ScratchBackendConvertor();
    ScratchBackendConvertor(Component* comp, Params& params);
    void finish(void);
    virtual const std::string& getClockFreq();
    virtual size_t getMemSize();
    virtual bool clock( Cycle_t cycle );
    virtual void handleScratchEvent(  ScratchEvent* );

    virtual const std::string& getRequestor( ReqId reqId ) { 
        uint32_t id = ScratchReq::getBaseId(reqId);
        if ( m_pendingRequests.find( id ) == m_pendingRequests.end() ) {
            m_dbg.fatal(CALL_INFO, -1, "memory request not found\n");
        }

        return m_pendingRequests[id]->getScratchEvent()->getRqstr();
    }

  protected:
    ~ScratchBackendConvertor() {
        while ( m_requestQueue.size()) {
            delete m_requestQueue.front();
            m_requestQueue.pop_front();
        }

        PendingRequests::iterator iter = m_pendingRequests.begin();
        for ( ; iter != m_pendingRequests.end(); ++ iter ) {
            delete iter->second;
        }
    }

    bool doResponse( ReqId reqId, SST::Event::id_type &respId );
    void notifyResponse( SST::Event::id_type );

    MemBackend* m_backend;
    uint32_t    m_backendRequestWidth;

  private:
    virtual bool issue(ScratchReq*) = 0;

    void setupScratchReq( ScratchEvent* ev ) {
        uint32_t id = genReqId();
        ScratchReq* req = new ScratchReq( ev, id, m_cycleCount );
        m_requestQueue.push_back( req );
        m_pendingRequests[id] = req;
    }

    inline void doClockStat( ) {
        stat_totalCycles->addData(1);        
    }

    void doReceiveStat( ScratchCommand cmd) {
        switch (cmd ) { 
            case Read: 
                stat_ReadReceived->addData(1);
                break;
            case Write:
                stat_WriteReceived->addData(1);
                break;
            default:
                break;
        }
    }

    void doResponseStat( ScratchCommand cmd, Cycle_t latency ) {
        switch (cmd) {
            case Read:
                stat_ReadLatency->addData(latency);
                break;
            case Write:
                stat_WriteLatency->addData(latency);
                break;
            default:
                break;
        }
    }

    Output      m_dbg;

    uint64_t m_cycleCount;

    uint32_t genReqId( ) { return ++m_reqId; }

    uint32_t    m_reqId;

    typedef std::map<uint32_t,ScratchReq*>    PendingRequests;

    std::deque<ScratchReq*>     m_requestQueue;
    PendingRequests         m_pendingRequests;
    uint32_t                m_frontendRequestWidth;

    Statistic<uint64_t>* stat_ReadLatency;
    Statistic<uint64_t>* stat_WriteLatency;

    Statistic<uint64_t>* stat_ReadReceived;
    Statistic<uint64_t>* stat_WriteReceived;

    Statistic<uint64_t>* stat_cyclesWithIssue;
    Statistic<uint64_t>* stat_cyclesAttemptIssueButRejected;
    Statistic<uint64_t>* stat_totalCycles;
    Statistic<uint64_t>* stat_outstandingReqs;
};

}
}
#endif
