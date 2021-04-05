// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
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
#include "sst/elements/memHierarchy/memEvent.h"

namespace SST {
namespace MemHierarchy {

class MemBackend;

class ScratchBackendConvertor : public SubComponent {
public:
    SST_ELI_REGISTER_SUBCOMPONENT_API(SST::MemHierarchy::ScratchBackendConvertor)

#define SCRATCHBACKENDCONVERTOR_ELI_SLOTS {"backend", "Backend memory model", "SST::MemHierarchy::MemBackend"}

    typedef uint64_t ReqId;

    class MemReq {
      public:
        MemReq( MemEvent* event, uint32_t reqId ) : m_event(event),
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
        bool isWrite()          { return (m_event->getCmd() == Command::PutM || (m_event->queryFlag(MemEvent::F_NONCACHEABLE) && m_event->getCmd() == Command::GetX)) ? true : false; }
        uint32_t size()         { return m_event->getSize(); }

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
        uint32_t    m_reqId;
        uint32_t    m_offset;
        uint32_t    m_numReq;
    };

  public:

    ScratchBackendConvertor();
    ScratchBackendConvertor(ComponentId_t id, Params& params);
    void finish(void);
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

    virtual void setCallbackHandler(std::function<void(Event::id_type)> func);

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

    std::function<void(Event::id_type)> m_notifyResponse;

  private:
    virtual bool issue(MemReq*) = 0;

    void setupMemReq( MemEvent* ev ) {
        uint32_t id = genReqId();
        MemReq* req = new MemReq( ev, id );
        m_requestQueue.push_back( req );
        m_pendingRequests[id] = req;
    }

    inline void doClockStat( ) {
        stat_totalCycles->addData(1);
    }

    void doReceiveStat( Command cmd) {
        switch (cmd ) {
            case Command::GetS:
                stat_GetSReqReceived->addData(1);
                break;
            case Command::GetX:
                stat_GetXReqReceived->addData(1);
                break;
            case Command::GetSX:
                stat_GetSXReqReceived->addData(1);
                break;
            case Command::PutM:
                stat_PutMReqReceived->addData(1);
                break;
            default:
                break;
        }
    }

    void doResponseStat( Command cmd, Cycle_t latency ) {
        switch (cmd) {
            case Command::GetS:
                stat_GetSLatency->addData(latency);
                break;
            case Command::GetSX:
                stat_GetSXLatency->addData(latency);
                break;
            case Command::GetX:
                stat_GetXLatency->addData(latency);
                break;
            case Command::PutM:
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

    Statistic<uint64_t>* stat_GetSLatency;
    Statistic<uint64_t>* stat_GetXLatency;
    Statistic<uint64_t>* stat_GetSXLatency;
    Statistic<uint64_t>* stat_PutMLatency;

    Statistic<uint64_t>* stat_GetSReqReceived;
    Statistic<uint64_t>* stat_GetXReqReceived;
    Statistic<uint64_t>* stat_GetSXReqReceived;
    Statistic<uint64_t>* stat_PutMReqReceived;

    Statistic<uint64_t>* stat_cyclesWithIssue;
    Statistic<uint64_t>* stat_cyclesAttemptIssueButRejected;
    Statistic<uint64_t>* stat_totalCycles;
    Statistic<uint64_t>* stat_outstandingReqs;
};

}
}
#endif
