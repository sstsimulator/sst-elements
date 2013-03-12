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

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <sst_config.h>
#include <sst/core/serialization/element.h>
#include <sst/core/simulation.h>
#include <sst/core/element.h>
#include <sst/core/interfaces/memEvent.h>
#include <sst/core/interfaces/stringEvent.h>

#include "memController.h"

#if defined(HAVE_LIBDRAMSIM)
// Our local copy of 'JEDEC_DATA_BUS_BITS', which has the comment // In bytes
static unsigned JEDEC_DATA_BUS_BITS_local= 64;
#endif

#define DPRINTF( fmt, args...) __DBG( DBG_MEMORY, Memory, "%s: " fmt, getName().c_str(), ## args )

#define NO_STRING_DEFINED "N/A"

using namespace SST;
using namespace SST::MemHierarchy;
using namespace SST::Interfaces;

MemController::MemController(ComponentId_t id, Params_t &params) : Component(id)
{
	unsigned int ramSize = (unsigned int)params.find_integer("mem_size", 0);
	if ( ramSize == 0 )
		_abort(MemController, "Must specify RAM size (mem_size) in MB\n");
	memSize = ramSize * (1024*1024);
	rangeStart = (Addr)params.find_integer("rangeStart", 0);
	rangeEnd = rangeStart + memSize;

	std::string memoryFile = params.find_string("memory_file", NO_STRING_DEFINED);

	std::string clock_freq = params.find_string("clock", "");

	registerClock(clock_freq, new Clock::Handler<MemController>(this,
				&MemController::clock));
	registerTimeBase("1 ns", true);

	use_dramsim = (bool)params.find_integer("use_dramsim", 0);
	if ( use_dramsim ) {
#if !defined(HAVE_LIBDRAMSIM)
		_abort(MemController, "This version of SST not compiled with DRAMSim.\n");
#else

		std::string deviceIniFilename = params.find_string("device_ini",
				NO_STRING_DEFINED);
		if ( deviceIniFilename == NO_STRING_DEFINED )
			_abort(MemController, "XML must define a 'device_ini' file parameter\n");
		std::string systemIniFilename = params.find_string("system_ini",
				NO_STRING_DEFINED);
		if ( systemIniFilename == NO_STRING_DEFINED )
			_abort(MemController, "XML must define a 'system_ini' file parameter\n");


		memSystem = DRAMSim::getMemorySystemInstance(
				deviceIniFilename, systemIniFilename, "", "", ramSize);

		DRAMSim::Callback<MemController, void, unsigned int, uint64_t, uint64_t>
			*readDataCB, *writeDataCB;

		readDataCB = new DRAMSim::Callback<MemController, void, unsigned int, uint64_t, uint64_t>(
				this, &MemController::dramSimDone);
		writeDataCB = new DRAMSim::Callback<MemController, void, unsigned int, uint64_t, uint64_t>(
				this, &MemController::dramSimDone);

		memSystem->RegisterCallbacks(readDataCB, writeDataCB, NULL);
#endif
	} else {
		std::string access_time = params.find_string("access_time", "1000 ns");
		self_link = configureSelfLink("Self", access_time,
				new Event::Handler<MemController>(this, &MemController::handleSelfEvent));
	}


	bus_requested = false;


	int mmap_flags = MAP_PRIVATE;
	if ( memoryFile != NO_STRING_DEFINED ) {
		backing_fd = open(memoryFile.c_str(), O_RDWR);
		if ( backing_fd < 0 ) {
			_abort(MemController, "Unable to open backing file!\n");
		}
	} else {
		backing_fd = -1;
		mmap_flags |= MAP_ANON;
	}

	memBuffer = (uint8_t*)mmap(NULL, memSize, PROT_READ|PROT_WRITE, mmap_flags, backing_fd, 0);
	if ( !memBuffer ) {
		_abort(MemController, "Unable to MMAP backing store for Memory\n");
	}

	snoop_link = configureLink( "snoop_link", "50 ps",
			new Event::Handler<MemController>(this, &MemController::handleEvent));
	assert(snoop_link);

}


void MemController::init(unsigned int phase)
{
	if ( !phase ) {
		snoop_link->sendInitData(new StringEvent("SST::Interfaces::MemEvent"));
	}

	SST::Event *ev = NULL;
	while ( (ev = snoop_link->recvInitData()) != NULL ) {
		MemEvent *me = dynamic_cast<MemEvent*>(ev);
		if ( me ) {
			/* Push data to memory */
			if ( me->getCmd() == WriteReq ) {
				for ( size_t i = 0 ; i < me->getSize() ; i++ ) {
					memBuffer[me->getAddr() + i - rangeStart] = me->getPayload()[i];
				}
			} else {
				printf("Memory received unexpected Init Command: %d\n", me->getCmd() );
			}
		}
		delete ev;
	}

}


int MemController::Finish(void)
{
	munmap(memBuffer, memSize);
	if ( backing_fd != -1 ) {
		close(backing_fd);
	}
#if defined(HAVE_LIBDRAMSIM)
	if ( use_dramsim )
		memSystem->printStats(true);
#endif
	return 0;
}


void MemController::handleEvent(SST::Event *event)
{
	MemEvent *ev = dynamic_cast<MemEvent*>(event);
	if ( ev ) {
		bool to_me = ( ev->getDst() == getName() || ev->getDst() == BROADCAST_TARGET );
		switch ( ev->getCmd() ) {
		case RequestData:
		case ReadReq:
			if ( to_me ) addRequest(ev);
			break;
		case ReadResp:
			if ( ev->getSrc() != getName() ) // don't cancel from what we sent.
				cancelEvent(ev);
			break;
		case WriteReq:
		case SupplyData:
			if ( ev->queryFlag(MemEvent::F_WRITEBACK) )
				addRequest(ev);
			else
				cancelEvent(ev);
			break;
		case BusClearToSend:
			if ( to_me ) sendBusPacket();
			break;
		default:
			/* Ignore */
			break;
		}
		delete event;
	} else {
		printf("Error!  Bad Event Type!\n");
	}
}


void MemController::handleSelfEvent(SST::Event *event)
{
	MemEvent *ev = dynamic_cast<MemEvent*>(event);
	assert(ev);
	if ( !isCanceled(ev) )
		sendResponse(ev);
}


void MemController::addRequest(MemEvent *ev)
{
	DRAMReq *req = new DRAMReq(ev);
	DPRINTF("New Memory Request for 0x%lx (%s)\n", req->addr, req->isWrite ? "WRITE" : "READ");
	requestQueue.push_back(req);
	outstandingReqs[ev->getAddr()] = req;
}



bool MemController::clock(Cycle_t cycle)
{
#if defined(HAVE_LIBDRAMSIM)
	if ( use_dramsim )
		memSystem->update();
#endif

	if ( use_dramsim ) {
#if defined(HAVE_LIBDRAMSIM)
		// Send any new requests
		while ( ! requestQueue.empty() ) {
			DRAMReq *req = requestQueue.front();
			if ( req->canceled ) {
				requestQueue.pop_front();
				if ( req->amt_in_process == 0 ) {
					// Haven't started.  get rid of it completely
					outstandingReqs.erase(req->addr);
					delete req;
				}
			} else {
				uint64_t addr = req->addr + req->amt_in_process;

				addr &= ~(JEDEC_DATA_BUS_BITS_local -1); // Round down to bus boundary

				bool ok = memSystem->willAcceptTransaction(addr);
				if ( !ok ) break;
				ok = memSystem->addTransaction(req->isWrite, addr);
				if ( !ok ) break;  // This *SHOULD* always be ok
				DPRINTF("Issued transaction for address 0x%lx\n", addr);
				req->amt_in_process += JEDEC_DATA_BUS_BITS_local;
				assert(dramReqs.find(addr) == dramReqs.end());
				dramReqs[addr] = req;

				if ( req->amt_in_process >= req->size ) {
					// Sent all requests off
					requestQueue.pop_front();
				}
			}
		}
#endif
	} else {
		while ( ! requestQueue.empty() ) {
			DRAMReq *req = requestQueue.front();
			/* Simple timing */
			if ( !req->canceled) {
				MemEvent *resp = performRequest(req);
				if ( resp->getCmd() != NULLCMD ) {
					self_link->Send(resp);
				} else {
					delete resp;
				}
			}
			requestQueue.pop_front();
		}
	}

	return false;
}



MemEvent* MemController::performRequest(DRAMReq *req)
{
	MemEvent *resp = req->reqEvent->makeResponse(this);
	if ( req->isWrite ) {
		for ( size_t i = 0 ; i < req->size ; i++ ) {
			memBuffer[req->addr + i - rangeStart] = req->reqEvent->getPayload()[i];
		}
	} else {
		for ( size_t i = 0 ; i < req->size ; i++ ) {
			resp->getPayload()[i] = memBuffer[req->addr + i - rangeStart];
		}
	}
	return resp;
}


void MemController::sendBusPacket(void)
{
	for (;;) {
		if ( busReqs.size() == 0 ) {
			snoop_link->Send(new MemEvent(this, NULL, CancelBusRequest));
			bus_requested = false;
			break;
		} else {
			MemEvent *ev = busReqs.front();
			busReqs.pop_front();
			if ( !isCanceled(ev) ) {
				DPRINTF("Sending (%lu, %d) in response to (%lu, %d) 0x%lx\n",
						ev->getID().first, ev->getID().second,
						ev->getResponseToID().first, ev->getResponseToID().second,
						ev->getAddr());
				snoop_link->Send(0, ev);
				bus_requested = false;
				if ( busReqs.size() > 0 ) {
					// Re-request bus
					sendResponse(NULL);
				}
				break;
			}
			delete outstandingReqs[ev->getAddr()];
			outstandingReqs.erase(ev->getAddr());
		}
	}
}


void MemController::sendResponse(MemEvent *ev)
{
	if ( ev != NULL ) {
		busReqs.push_back(ev);
	}
	if (!bus_requested) {
		snoop_link->Send(new MemEvent(this, NULL, RequestBus));
		bus_requested = true;
	}
}


bool MemController::isCanceled(Addr addr)
{
	std::map<Addr, DRAMReq*>::iterator i = outstandingReqs.find(addr);
	if ( i == outstandingReqs.end() ) return false;
	return i->second->canceled;
}

bool MemController::isCanceled(MemEvent *ev)
{
	return isCanceled(ev->getAddr());
}


void MemController::cancelEvent(MemEvent* ev)
{
	std::map<Addr, DRAMReq*>::iterator i = outstandingReqs.find(ev->getAddr());
	DPRINTF("Looking to cancel for (0x%lx)\n", ev->getAddr());
	if ( i != outstandingReqs.end() && i->second->size <= ev->getSize() ) {
		DPRINTF("Canceling request.\n");
		outstandingReqs[ev->getAddr()]->canceled = true;
	}
}




#if defined(HAVE_LIBDRAMSIM)

void MemController::dramSimDone(unsigned int id, uint64_t addr, uint64_t clockcycle)
{
	DRAMReq *req = dramReqs[addr];
	dramReqs.erase(addr);
	req->amt_processed += JEDEC_DATA_BUS_BITS_local;
	if ( req->amt_processed >= req->size ) {
		// This req is done
		if ( !req->canceled ) {
	DPRINTF("Memory Request for 0x%lx (%s) Finished\n", req->addr, req->isWrite ? "WRITE" : "READ");
			MemEvent *resp = performRequest(req);
			sendResponse(resp);
		} else {
			outstandingReqs.erase(req->addr);
			delete req;
		}
	}
}



#endif

