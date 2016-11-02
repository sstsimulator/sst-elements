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
        bool isWrite()          { return (m_event->getCmd() == PutM) ? true : false; }

        void setResponse( MemEvent* event ) { m_respEvent = event; }
        MemEvent* getResponse() { return m_respEvent; }

        void increment( uint32_t bytes ) {
            m_offset += bytes;
            ++m_numReq;
        }
        void decrement( ) { --m_numReq; }
        bool isDone( ) {
            return ( m_offset == m_event->getSize() && 0 == m_numReq );
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
    virtual bool clock( Cycle_t cycle ) = 0;
    virtual void handleMemEvent(  MemEvent* ) = 0;
    virtual const std::string& getRequestor( ReqId ) { assert(0); };

  protected:
    ~MemBackendConvertor() {}

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
    MemBackend* m_backend;

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
