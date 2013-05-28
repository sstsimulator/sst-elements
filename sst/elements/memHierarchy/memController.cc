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

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>

#include <sst_config.h>
#include <sst/core/serialization/element.h>
#include <sst/core/simulation.h>
#include <sst/core/element.h>
#include <sst/core/interfaces/memEvent.h>
#include <sst/core/interfaces/stringEvent.h>

#include "memController.h"

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
	interleaveSize = (Addr)params.find_integer("interleaveSize", 0);
    interleaveSize *= 1024;
	interleaveStep = (Addr)params.find_integer("interleaveStep", 0);
    interleaveStep *= 1024;
    if ( interleaveStep > 0 && interleaveSize > 0 ) {
        numPages = memSize / interleaveSize;
    } else {
        numPages = 0;
    }

	std::string memoryFile = params.find_string("memory_file", NO_STRING_DEFINED);

	std::string clock_freq = params.find_string("clock", "");

    requestSize = (size_t)params.find_integer("request_width", 64);

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

	upstream_link = configureLink( "snoop_link", "50 ps",
			new Event::Handler<MemController>(this, &MemController::handleEvent));
    use_bus = (upstream_link != NULL);
    if ( !upstream_link ) {

        std::string link_lat = params.find_string("direct_link_latency", "100 ns");
        upstream_link = configureLink( "direct_link", link_lat,
                new Event::Handler<MemController>(this, &MemController::handleEvent));
    }

}


void MemController::init(unsigned int phase)
{
	if ( !phase ) {
        upstream_link->sendInitData(new StringEvent("SST::Interfaces::MemEvent"));
	}

	SST::Event *ev = NULL;
    while ( (ev = upstream_link->recvInitData()) != NULL ) {
        MemEvent *me = dynamic_cast<MemEvent*>(ev);
        if ( me ) {
            /* Push data to memory */
            if ( me->getCmd() == WriteReq ) {
                //printf("Memory received Init Command: of size 0x%x at addr 0x%"PRIx64"\n", me->getSize(), me->getAddr() );
                if ( isRequestAddressValid(me) ) {
                    Addr localAddr = convertAddressToLocalAddress(me->getAddr());
                    for ( size_t i = 0 ; i < me->getSize() ; i++ ) {
                        memBuffer[localAddr + i] = me->getPayload()[i];
                    }
                }
            } else {
                printf("Memory received unexpected Init Command: %d\n", me->getCmd() );
            }
        }
        delete ev;
    }

}

void MemController::setup(void)
{
}


void MemController::finish(void)
{
	munmap(memBuffer, memSize);
	if ( backing_fd != -1 ) {
		close(backing_fd);
	}
#if defined(HAVE_LIBDRAMSIM)
	if ( use_dramsim )
		memSystem->printStats(true);
#endif


#if 0
    /* TODO:  Toggle this based off of a parameter */
    printf("--------------------------------------------------------\n");
    printf("MemController: %s\n", getName().c_str());
    printf("Outstanding Requests:  %zu\n", outstandingReadReqs.size());
    for ( std::map<Addr, DRAMReq*>::iterator i = outstandingReadReqs.begin() ; i != outstandingReadReqs.end() ; ++i ) {
        DRAMReq *req = i->second;
        printf("\t0x%08lx\t%s (%lu, %lu)\t%zu bytes:  %zu/%zu\n",
                i->first, CommandString[req->reqEvent->getCmd()],
                req->reqEvent->getID().first, req->reqEvent->getID().second,
                req->size, req->amt_in_process, req->amt_processed);
    }
    printf("Requests Queue:  %zu\n", requestQueue.size());
    printf("--------------------------------------------------------\n");
#endif

}


void MemController::handleEvent(SST::Event *event)
{
	MemEvent *ev = static_cast<MemEvent*>(event);
    bool to_me = (!use_bus || ( ev->getDst() == getName() || ev->getDst() == BROADCAST_TARGET ));
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
        if ( !use_bus || ev->queryFlag(MemEvent::F_WRITEBACK) )
            addRequest(ev);
        else
            if ( ev->getSrc() != getName() ) // don't cancel from what we sent.
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
}



void MemController::addRequest(MemEvent *ev)
{
	DPRINTF("New Memory Request for 0x%"PRIx64"\n", ev->getAddr());

    if ( isRequestAddressValid(ev) ) {

        DRAMReq *req = new DRAMReq(ev, requestSize);
        DPRINTF("Creating DRAM Request for 0x%"PRIx64" (%s)\n", req->addr, req->isWrite ? "WRITE" : "READ");

        requests.push_back(req);
        requestQueue.push_back(req);
    } else {
        /* TODO:  Ideally, if this came over as a direct message, not over a bus, we should NAK this */
        DPRINTF("Ignoring request for 0x%"PRIx64" as it isn't in our range. [0x%"PRIx64" - 0x%"PRIx64"]\n",
                ev->getAddr(), rangeStart, rangeStart + memSize);
    }

}


void MemController::cancelEvent(MemEvent* ev)
{
	DPRINTF("Looking to cancel for (0x%"PRIx64")\n", ev->getAddr());
    for ( size_t i = 0 ; i < requests.size() ; ++i ) {
        if ( requests[i]->isSatisfiedBy(ev) ) {
            if ( !requests[i]->isWrite && !requests[i]->canceled ) {
                requests[i]->canceled = true;
                DPRINTF("Canceling request.\n");
                if ( requests[i]->status == DRAMReq::RETURNED ) {
                    sendBusCancel(requests[i]->reqEvent->getAddr());
                }
            }
        }
    }
}




bool MemController::clock(Cycle_t cycle)
{
#if defined(HAVE_LIBDRAMSIM)
	if ( use_dramsim )
		memSystem->update();
#endif

    while ( !requestQueue.empty() ) {
        DRAMReq *req = requestQueue.front();
        if ( req->canceled ) {
            requestQueue.pop_front();
            if ( req->status == DRAMReq::NEW ) // Haven't started processing
                req->status = DRAMReq::DONE;
            continue;
        }

        req->status = DRAMReq::PROCESSING;
        uint64_t addr = req->addr + req->amt_in_process;
        if ( use_dramsim ) {
#if defined(HAVE_LIBDRAMSIM)

            bool ok = memSystem->willAcceptTransaction(addr);
            if ( !ok ) break;
            ok = memSystem->addTransaction(req->isWrite, addr);
            if ( !ok ) break;  // This *SHOULD* always be ok
            DPRINTF("Issued transaction for address 0x%"PRIx64"\n", addr);
            dramReqs[addr].push_back(req);
#endif
        } else {
            DPRINTF("Issued transaction for address 0x%"PRIx64"\n", addr);
            self_link->send(1, new MemCtrlEvent(req));
        }

        req->amt_in_process += requestSize;

        if ( req->amt_in_process >= req->size ) {
            DPRINTF("Completed issue of request\n");
            performRequest(req);
            requestQueue.pop_front();
        }
    }

    /* Clean out old requests */
    while ( requests.size() ) {
        DRAMReq *req = requests.front();
        if ( req->status == DRAMReq::DONE ) {
            requests.pop_front();
        } else {
            break;
        }
    }

	return false;
}


bool MemController::isRequestAddressValid(MemEvent *ev)
{
    Addr addr = ev->getAddr();

    if ( numPages == 0 ) {
        return ( addr >= rangeStart && addr < (rangeStart + memSize) );
    } else {
        if ( addr < rangeStart ) return false;

        addr = addr - rangeStart;
        Addr step = addr / interleaveStep;
        if ( step >= numPages ) return false;

        Addr offset = addr % interleaveStep;
        if ( offset >= interleaveSize ) return false;

        return true;
    }

}


Addr MemController::convertAddressToLocalAddress(Addr addr)
{
    if ( numPages == 0 ) {
        return addr - rangeStart;
    } else {
        addr = addr - rangeStart;
        Addr step = addr / interleaveStep;
        Addr offset = addr % interleaveStep;
        return (step * interleaveSize) + offset;
    }
}


void MemController::performRequest(DRAMReq *req)
{
	MemEvent *resp = req->reqEvent->makeResponse(this);
    Addr localAddr = convertAddressToLocalAddress(req->addr);

    req->respEvent = resp;
	if ( req->isWrite ) {
		for ( size_t i = 0 ; i < req->reqEvent->getSize() ; i++ ) {
			memBuffer[localAddr + i] = req->reqEvent->getPayload()[i];
		}
        DPRINTF("Writing Memory: %zu bytes beginning at 0x%"PRIx64" [0x%02x%02x%02x%02x%02x%02x%02x%02x...\n",
                req->size, req->addr,
                memBuffer[localAddr + 0], memBuffer[localAddr + 1],
                memBuffer[localAddr + 2], memBuffer[localAddr + 3],
                memBuffer[localAddr + 4], memBuffer[localAddr + 5],
                memBuffer[localAddr + 6], memBuffer[localAddr + 7]);
	} else {
		for ( size_t i = 0 ; i < resp->getSize() ; i++ ) {
			resp->getPayload()[i] = memBuffer[localAddr + i];
		}
        DPRINTF("Reading Memory: %zu bytes beginning at 0x%"PRIx64" [0x%02x%02x%02x%02x%02x%02x%02x%02x...\n",
                req->size, req->addr,
                memBuffer[localAddr + 0], memBuffer[localAddr + 1],
                memBuffer[localAddr + 2], memBuffer[localAddr + 3],
                memBuffer[localAddr + 4], memBuffer[localAddr + 5],
                memBuffer[localAddr + 6], memBuffer[localAddr + 7]);
	}
}


void MemController::sendBusPacket(void)
{
    assert(use_bus);
	for (;;) {
		if ( busReqs.size() == 0 ) {
            DPRINTF("Sending cancelation, as we have nothing in the queue.\n");
			upstream_link->send(new MemEvent(this, NULL, CancelBusRequest));
			break;
		} else {
            DRAMReq *req = busReqs.front();
			busReqs.pop_front();
            req->status = DRAMReq::DONE;
			if ( !req->canceled ) {
                MemEvent *ev = req->respEvent;
				DPRINTF("Sending (%"PRIu64", %d) in response to (%"PRIu64", %d) 0x%"PRIx64"\n",
						ev->getID().first, ev->getID().second,
						ev->getResponseToID().first, ev->getResponseToID().second,
						ev->getAddr());
				upstream_link->send(0, ev);
				break;
            }
		}
	}
}

void MemController::sendBusCancel(Addr addr) {
    upstream_link->send(new MemEvent(this, addr, CancelBusRequest));
}


void MemController::sendResponse(DRAMReq *req)
{
    if ( use_bus ) {
        busReqs.push_back(req);
        upstream_link->send(new MemEvent(this, req->reqEvent->getAddr(), RequestBus));
    } else {
        upstream_link->send(req->respEvent);
        req->status = DRAMReq::DONE;
    }
}


void MemController::handleMemResponse(DRAMReq *req)
{
    DPRINTF("Finishing processing for req 0x%"PRIx64"\n", req->addr);
    req->amt_processed += requestSize;
    if ( req->amt_processed >= req->size ) {
        req->status = DRAMReq::RETURNED;
    }

    if ( req->status == DRAMReq::RETURNED ) {
        if ( !req->canceled )
            sendResponse(req);
        else
            req->status = DRAMReq::DONE;
    } else {
       if ( req->canceled ) {
           if ( req->amt_processed >= req->amt_in_process )
               req->status = DRAMReq::DONE;
       }
    }

}


void MemController::handleSelfEvent(SST::Event *event)
{
	MemCtrlEvent *ev = static_cast<MemCtrlEvent*>(event);
    DRAMReq *req = ev->req;
    handleMemResponse(req);
    delete event;
}

#if defined(HAVE_LIBDRAMSIM)

void MemController::dramSimDone(unsigned int id, uint64_t addr, uint64_t clockcycle)
{
    std::deque<DRAMReq *> &reqs = dramReqs[addr];
    DPRINTF("Memory Request for 0x%"PRIx64" Finished [%zu reqs]\n", addr, reqs.size());
    assert(reqs.size());
    DRAMReq *req = reqs.front();
    reqs.pop_front();
    if ( reqs.size() == 0 )
        dramReqs.erase(addr);

    handleMemResponse(req);
}


#endif

