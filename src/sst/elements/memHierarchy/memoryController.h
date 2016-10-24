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

#ifndef _MEMORYCONTROLLER_H
#define _MEMORYCONTROLLER_H


#include <sst/core/event.h>
#include <sst/core/module.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/output.h>
#include <map>

#ifdef HAVE_LIBZ
#include <zlib.h>
#endif

#include "sst/elements/memHierarchy/memEvent.h"
#include "sst/elements/memHierarchy/bus.h"
#include "sst/elements/memHierarchy/cacheListener.h"
#include "sst/elements/memHierarchy/memNIC.h"

namespace SST {
namespace MemHierarchy {

class MemBackend;
using namespace std;

class MemController : public SST::Component {
public:
	typedef uint64_t ReqId;

    MemController(ComponentId_t id, Params &params);
    void init(unsigned int);
    void setup();
    void finish();

    Output dbg;
    virtual void handleMemResponse( ReqId );
    virtual const std::string& getRequestor( ReqId );

private:

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
		Addr addr()		{ return m_event->getBaseAddr() + m_offset; }

		uint32_t processed() 	{ return m_offset; }
		uint64_t id() 			{ return ((uint64_t)m_reqId << 32) | m_offset; }
		MemEvent* getMemEvent() { return m_event; } 
		bool isWrite() 			{ return (m_event->getCmd() == PutM) ? true : false; }

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
		MemEvent* 	m_event;
		MemEvent* 	m_respEvent;
		uint32_t    m_reqId;
		uint32_t	m_offset;
		uint32_t	m_numReq;
	};

	std::deque<MemReq*> 		requestQueue_;
	typedef std::map<uint32_t,MemReq*> 	PendingRequests;
	PendingRequests				pendingRequests_;

    MemController();  // for serialization only
    ~MemController();

    void handleEvent( SST::Event* );
    void addRequest( MemEvent* );
    bool clock( SST::Cycle_t );
    MemEvent* performRequest( MemEvent* );
    void sendResponse( MemReq* );
    void printMemory( Addr _localAddr );
    int setBackingFile(string memoryFile);

    bool isRequestAddressValid(MemEvent *ev){
        Addr addr = ev->getAddr();
        Addr step;

        if(0 == numPages_)     return (addr >= rangeStart_ && addr < (rangeStart_ + memSize_));
        if(addr < rangeStart_) return false;
        
        addr = addr - rangeStart_;
        step = addr / interleaveStep_;
        if(step >= numPages_)  return false;

        Addr offset = addr % interleaveStep_;
        if (offset >= interleaveSize_) return false;

        return true;
    }


    Addr convertAddressToLocalAddress(Addr addr){
        if (0 == numPages_) return addr - rangeStart_;
        
        addr = addr - rangeStart_;
        Addr step = addr / interleaveStep_;
        Addr offset = addr % interleaveStep_;
        return (step * interleaveSize_) + offset;
    }
    
    void cancelEvent(MemEvent *ev) {}
    void sendBusPacket(Bus::key_t key){}
    void sendBusCancel(Bus::key_t key){}
    void handleBusEvent(SST::Event *event){}
    
	uint32_t genReqId( ) { return ++reqId_; }
	uint32_t reqId_;
#if 0
    typedef deque<DRAMReq*> dramReq_t;
#endif
    
    bool        divertDCLookups_;
    SST::Link*  cacheLink_;         // Link to the rest of memHierarchy 
    MemNIC*     networkLink_;       // Link to the rest of memHierarchy if we're communicating over a network
    MemBackend* backend_;
    int         protocol_;
#if 0
    dramReq_t   requestQueue_;      // Requests waiting to be issued
    set<DRAMReq*>   requestPool_;   // All requests that are in flight at the memory controller (including those waiting to be issued) 
#endif
    int         backingFd_;
    uint8_t*    memBuffer_;
    uint64_t    memSize_;
    uint64_t    requestSize_;
    Addr        rangeStart_;
    uint64_t    numPages_;
    Addr        interleaveSize_;
    Addr        interleaveStep_;
    bool        doNotBack_;
    uint32_t    cacheLineSize_;
    uint32_t    requestWidth_;
    std::vector<CacheListener*> listeners_;
    int         maxReqsPerCycle_;

    Statistic<uint64_t>* cyclesWithIssue;
    Statistic<uint64_t>* cyclesAttemptIssueButRejected;
    Statistic<uint64_t>* totalCycles;
    Statistic<uint64_t>* stat_GetSReqReceived;
    Statistic<uint64_t>* stat_GetXReqReceived;
    Statistic<uint64_t>* stat_PutMReqReceived;
    Statistic<uint64_t>* stat_GetSExReqReceived;
    Statistic<uint64_t>* stat_outstandingReqs;
    Statistic<uint64_t>* stat_GetSLatency;
    Statistic<uint64_t>* stat_GetSExLatency;
    Statistic<uint64_t>* stat_GetXLatency;
    Statistic<uint64_t>* stat_PutMLatency;

    Output::output_location_t statsOutputTarget_;
//#ifdef HAVE_LIBZ
//    gzFile traceFP;
//#else
    FILE *traceFP;
//#endif


};



}}

#endif /* _MEMORYCONTROLLER_H */
