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


#ifndef __SST_MEMH_SIMPLEMEMBACKENDCONVERTOR__
#define __SST_MEMH_SIMPLEMEMBACKENDCONVERTOR__

#include "sst/elements/memHierarchy/membackend/memBackendConvertor.h"

namespace SST {
namespace MemHierarchy {

class SimpleMemBackendConvertor : public MemBackendConvertor {
 
  protected:

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
    SimpleMemBackendConvertor(Component *comp, Params &params);
    virtual bool clock( Cycle_t cycle );
    virtual void handleMemEvent(  MemEvent* );

    virtual void handleMemResponse( ReqId );
    virtual const std::string& getRequestor( ReqId );

  protected:
    ~SimpleMemBackendConvertor();

    void sendResponse( MemReq* );
    uint32_t genReqId( ) { return ++m_reqId; }

    uint32_t    m_reqId;
    typedef std::map<uint32_t,MemReq*>    PendingRequests;
    std::deque<MemReq*>     m_requestQueue;
    PendingRequests         m_pendingRequests;
    uint32_t                m_frontendRequestWidth;
    uint32_t                m_backendRequestWidth;
};

}
}
#endif
