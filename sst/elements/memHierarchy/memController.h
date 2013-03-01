// Copyright 2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013, Sandia Corporation
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
	int Finish();


private:

	struct DRAMReq {
		MemEvent *reqEvent;
		bool canceled;
		Addr addr;
		bool isWrite;
		size_t size;
		size_t amt_in_process;
		size_t amt_processed;

		DRAMReq(MemEvent *ev) :
			reqEvent(new MemEvent(ev)), canceled(false), addr(ev->getAddr()),
			isWrite(ev->getCmd() == SupplyData || ev->getCmd() == WriteReq),
			size(ev->getSize()), amt_processed(0)
		{ }
	};


	MemController();  // for serialization only

	void handleEvent(SST::Event *event);
	void handleSelfEvent(SST::Event *event);

	void addRequest(MemEvent *ev);
	bool clock(SST::Cycle_t cycle);

	MemEvent* performRequest(DRAMReq *req);

	void sendBusPacket(void);

	void sendResponse(MemEvent *ev);
	bool isCanceled(Addr addr);
	bool isCanceled(MemEvent *ev);
	void cancelEvent(MemEvent *ev);


	bool use_dramsim;

	SST::Link *self_link;
	SST::Link *snoop_link;
	bool bus_requested;
	std::deque<MemEvent*> busReqs;

	std::deque<DRAMReq*> requestQueue;
	std::map<Addr, DRAMReq*> outstandingReqs;


	int backing_fd;
	uint8_t *memBuffer;
	size_t memSize;
	Addr rangeStart;
	Addr rangeEnd;

#if defined(HAVE_LIBDRAMSIM)
	void dramSimDone(unsigned int id, uint64_t addr, uint64_t clockcycle);

	DRAMSim::MultiChannelMemorySystem *memSystem;

	std::map<uint64_t, DRAMReq*> dramReqs;
#endif


};


}}

#endif /* _MEMORYCONTROLLER_H */
