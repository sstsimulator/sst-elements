// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2013, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _MEMORYCONTROLLER_H
#define _MEMORYCONTROLLER_H

#include "sst_config.h"

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>

#include <sst/core/interfaces/memEvent.h>
using namespace SST::Interfaces;

#if defined(HAVE_LIBDRAMSIM)
// DRAMSim uses DEBUG
#ifdef DEBUG
# define OLD_DEBUG DEBUG
# undef DEBUG
#endif
#include <DRAMSim.h>
#ifdef OLD_DEBUG
# define DEBUG OLD_DEBUG
# undef OLD_DEBUG
#endif
#endif


namespace SST {
namespace MemHierarchy {

class MemController : public SST::Component {
public:
	MemController(ComponentId_t id, Params_t &params);
	void init(unsigned int);
	int Setup();
	int Finish();


private:

	struct DRAMReq {
        enum Status_t {NEW, PROCESSING, RETURNED, DONE};

		MemEvent *reqEvent;
		MemEvent *respEvent;
		bool isWrite;
        bool canceled;

		size_t size;
		size_t amt_in_process;
		size_t amt_processed;
        Status_t status;

		Addr addr;
        uint32_t num_req; // size / bus width;

		DRAMReq(MemEvent *ev, const size_t busWidth) :
			reqEvent(new MemEvent(ev)), respEvent(NULL),
			isWrite(ev->getCmd() == SupplyData || ev->getCmd() == WriteReq),
            canceled(false),
			size(ev->getSize()), amt_in_process(0), amt_processed(0), status(NEW)
		{
            Addr reqEndAddr = ev->getAddr() + ev->getSize();
            addr = ev->getAddr() & ~(busWidth -1); // round down to bus alignment;

            num_req = (reqEndAddr - addr) / busWidth;
            if ( (reqEndAddr - addr) % busWidth ) num_req++;

            size = num_req * busWidth;
#if 0
            printf(
                    "***************************************************\n"
                    "Buswidth = %zu\n"
                    "Ev:   0x%08lx  + 0x%02x -> 0x%08lx\n"
                    "Req:  0x%08lx  + 0x%02x   [%u count]\n"
                    "***************************************************\n",
                    busWidth, ev->getAddr(), ev->getSize(), reqEndAddr,
                    addr, size, num_req);
#endif
        }

        ~DRAMReq() {
            delete reqEvent;
        }

        bool canSatisfy(const MemEvent *ev)
        {
            return ((reqEvent->getAddr() <= ev->getAddr()) &&
                    (reqEvent->getAddr()+reqEvent->getSize() >= (ev->getAddr() + ev->getSize())));
        }

        bool isSatisfiedBy(const MemEvent *ev)
        {
            return ((reqEvent->getAddr() >= ev->getAddr()) &&
                    (reqEvent->getAddr()+reqEvent->getSize() <= (ev->getAddr() + ev->getSize())));
        }

	};

    class MemCtrlEvent : public SST::Event {
    public:
        MemCtrlEvent(DRAMReq* req) : SST::Event(), req(req)
        { }

        DRAMReq *req;
    private:
        friend class boost::serialization::access;
        template<class Archive>
            void
            serialize(Archive & ar, const unsigned int version )
            {
                ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Event);
                ar & BOOST_SERIALIZATION_NVP(req);
            }
    };



	MemController();  // for serialization only

	void handleEvent(SST::Event *event);

	void addRequest(MemEvent *ev);
	void cancelEvent(MemEvent *ev);
	bool clock(SST::Cycle_t cycle);

	void performRequest(DRAMReq *req);

	void sendBusPacket(void);
    void sendBusCancel(void);

	void sendResponse(DRAMReq *req);

    void handleMemResponse(DRAMReq *req);
	void handleSelfEvent(SST::Event *event);

	bool use_dramsim;

	SST::Link *self_link;
	SST::Link *snoop_link;
	bool bus_requested;
	std::deque<DRAMReq*> busReqs;

	std::deque<DRAMReq*> requestQueue;
	std::deque<DRAMReq*> requests;


	int backing_fd;
	uint8_t *memBuffer;
	size_t memSize;
    size_t requestSize;
	Addr rangeStart;
	Addr rangeEnd;

#if defined(HAVE_LIBDRAMSIM)
	void dramSimDone(unsigned int id, uint64_t addr, uint64_t clockcycle);

	DRAMSim::MultiChannelMemorySystem *memSystem;

    std::map<uint64_t, std::deque<DRAMReq*> > dramReqs;
#endif


};


}}

#endif /* _MEMORYCONTROLLER_H */
