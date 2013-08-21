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

#include <sst_config.h>
#include <sst/core/serialization.h>
#include "memController.h"

#include <sstream>
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <sst/core/simulation.h>
#include <sst/core/element.h>
#include <sst/core/interfaces/memEvent.h>
#include <sst/core/interfaces/stringEvent.h>

#include "bus.h"

#define NO_STRING_DEFINED "N/A"

using namespace SST;
using namespace SST::MemHierarchy;
using namespace SST::Interfaces;

MemController::MemController(ComponentId_t id, Params_t &params) : Component(id)
{
    dbg.init("@t:Memory::@p():@l " + getName() + ": ", 0, 0, (Output::output_location_t)params.find_integer("debug", 0));
    statsOutputTarget = (Output::output_location_t)params.find_integer("printStats", 0);

    unsigned int ramSize = (unsigned int)params.find_integer("mem_size", 0);
	if ( 0 == ramSize )
		_abort(MemController, "Must specify RAM size (mem_size) in MB\n");
	memSize = ramSize * (1024*1024ul);

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

	// default access time
	std::string access_time = params.find_string("access_time", "10 ns");

	// check for and initialize dramsim
	use_dramsim = (bool)params.find_integer("use_dramsim", 0);
	if ( use_dramsim ) {
#if !defined(HAVE_LIBDRAMSIM)
		_abort(MemController, "This version of SST not compiled with DRAMSim.\n");
#else

		std::string deviceIniFilename = params.find_string("device_ini",
				NO_STRING_DEFINED);
		if ( NO_STRING_DEFINED == deviceIniFilename )
			_abort(MemController, "XML must define a 'device_ini' file parameter\n");
		std::string systemIniFilename = params.find_string("system_ini",
				NO_STRING_DEFINED);
		if ( NO_STRING_DEFINED == systemIniFilename )
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

	  // check for vaultsim
	  use_vaultSim = (bool)params.find_integer("use_vaultSim", 0);
	  if ( use_vaultSim && use_dramsim ) {
	    _abort(MemController, "Controller configured to use dramsim & vaultSim. Choose one\n");
	  }

	  if ( use_vaultSim ) {
	    cube_link = configureLink( "cube_link", access_time,
				       new Event::Handler<MemController>(this,
									 &MemController::handleCubeEvent));
	  }
	}

	self_link = configureSelfLink("Self", access_time,
				      new Event::Handler<MemController>(this,
 &MemController::handleSelfEvent));


	int mmap_flags = MAP_PRIVATE;
	if ( NO_STRING_DEFINED != memoryFile ) {
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
			new Event::Handler<MemController>(this, &MemController::handleBusEvent));
    use_bus = (NULL != upstream_link );

    if ( !upstream_link ) {
        std::string link_lat = params.find_string("direct_link_latency", "100 ns");
        upstream_link = configureLink( "direct_link", link_lat,
                new Event::Handler<MemController>(this, &MemController::handleEvent));
    }

    respondToInvalidates = false;

    numReadsSupplied = 0;
    numReadsCanceled = 0;
    numWrites = 0;
    numReqOutstanding = 0;
    numCycles = 0;
}


MemController::~MemController()
{
    while ( requests.size() ) {
        DRAMReq *req = requests.front();
        requests.pop_front();
        delete req;
    }
}


void MemController::init(unsigned int phase)
{
	if ( !phase ) {
        upstream_link->sendInitData(new StringEvent("SST::Interfaces::MemEvent"));
	}

	SST::Event *ev = NULL;
    while ( NULL != (ev = upstream_link->recvInitData()) ) {
        MemEvent *me = dynamic_cast<MemEvent*>(ev);
        if ( me ) {
            /* Push data to memory */
            if ( WriteReq == me->getCmd() ) {
                if ( isRequestAddressValid(me) ) {
                    Addr localAddr = convertAddressToLocalAddress(me->getAddr());
                    for ( size_t i = 0 ; i < me->getSize() ; i++ ) {
                        memBuffer[localAddr + i] = me->getPayload()[i];
                    }
                }
            } else {
                Output out("", 0, 0, Output::STDERR);
                out.output("Memory received unexpected Init Command: %d\n", me->getCmd() );
            }
        } else {
            StringEvent *se = dynamic_cast<StringEvent*>(ev);
            if ( se ) {
                if ( std::string::npos != se->getString().find(Bus::BUS_INFO_STR) ) {
                    /* Determine if we are to participate in ACK'ing Invalidates */
                    std::istringstream is(se->getString());
                    std::string header;
                    is >> header;
                    assert(!header.compare(Bus::BUS_INFO_STR));
                    std::string tag;
                    is >> tag;
                    int numPeers = 1;
                    if ( !tag.compare("NumACKPeers:") ) {
                        is >> numPeers;
                    }

                    respondToInvalidates = (numPeers > 1);
                }
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
	if ( -1 != backing_fd ) {
		close(backing_fd);
	}
#if defined(HAVE_LIBDRAMSIM)
	if ( use_dramsim )
		memSystem->printStats(true);
#endif

    Output out("", 0, 0, statsOutputTarget);
    out.output("Memory %s stats:\n"
	       "\t # Reads:             %"PRIu64"\n"
	       "\t # Writes:            %"PRIu64"\n"
	       "\t # Canceled Reads:    %"PRIu64"\n"
	       "\t # Avg. Requests out: %.3f\n",	       
            getName().c_str(),
            numReadsSupplied,
            numWrites,
	       numReadsCanceled,
	       float(numReqOutstanding)/float(numCycles)
);

}

void MemController::handleEvent(SST::Event *event)
{
	MemEvent *ev = static_cast<MemEvent*>(event);
    bool to_me = (!use_bus || ( ev->getDst() == getName() || BROADCAST_TARGET == ev->getDst() ));
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
        if ( use_bus ) // don't cancel from what we sent.
            cancelEvent(ev);
        if ( !use_bus || ev->queryFlag(MemEvent::F_WRITEBACK) )
            addRequest(ev);
        break;
    case Invalidate:
        if ( respondToInvalidates ) {
            /* We participate, but only by acknoweldging the request */
            DRAMReq *ackReq = new DRAMReq(ev, requestSize);
            requests.push_back(ackReq);
            ackReq->respEvent = ev->makeResponse(this);
            ackReq->isACK = true;
            ackReq->status = DRAMReq::RETURNED;
            sendResponse(ackReq);
        }
        break;
    case NACK:
        /* If somebody sent a NACK, that implies that any ReadRequest for the
         * same thing will need to be re-tried.  Let's cancel any action we
         * might be taking, and wait for the re-try.
         */
        if ( use_bus ) cancelEvent(ev);
        break;
    default:
        /* Ignore */
        break;
    }
    delete event;
}


void MemController::handleBusEvent(SST::Event *event)
{
    BusEvent *be = static_cast<BusEvent*>(event);
    switch ( be->getCmd() ) {
    case BusEvent::ClearToSend:
        sendBusPacket(be->getKey());
        break;
    case BusEvent::SendData:
        handleEvent(be->getPayload());
        break;
    default:
        _abort(MemController, "Bus Client should not be receiving this command.\n");
    }
    delete be;
}


void MemController::addRequest(MemEvent *ev)
{
	dbg.output(CALL_INFO, "New Memory Request for 0x%"PRIx64"\n", ev->getAddr());

    if ( isRequestAddressValid(ev) ) {

        DRAMReq *req = new DRAMReq(ev, requestSize);
        dbg.output(CALL_INFO, "Creating DRAM Request for 0x%"PRIx64" (%s)\n", req->addr, req->isWrite ? "WRITE" : "READ");

        requests.push_back(req);
        requestQueue.push_back(req);
    } else {
        /* TODO:  Ideally, if this came over as a direct message, not over a bus, we should NAK this */
        dbg.output(CALL_INFO, "Ignoring request for 0x%"PRIx64" as it isn't in our range. [0x%"PRIx64" - 0x%"PRIx64"]\n",
                ev->getAddr(), rangeStart, rangeStart + memSize);
    }

}


void MemController::cancelEvent(MemEvent* ev)
{
	dbg.output(CALL_INFO, "Looking to cancel for (0x%"PRIx64")\n", ev->getAddr());
    for ( size_t i = 0 ; i < requests.size() ; ++i ) {
        if ( requests[i]->isSatisfiedBy(ev) ) {
            if ( !requests[i]->isWrite && !requests[i]->canceled ) {
                requests[i]->canceled = true;
                numReadsCanceled++;
                dbg.output(CALL_INFO, "Canceling request 0x%"PRIx64" (%"PRIu64", %d).\n", requests[i]->addr, requests[i]->respEvent->getID().first, requests[i]->respEvent->getID().second);
                if ( DRAMReq::RETURNED == requests[i]->status ) {
                    sendBusCancel(requests[i]->respEvent->getID());
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
            if ( DRAMReq::NEW == req->status ) // Haven't started processing
                req->status = DRAMReq::DONE;
            continue;
        }

        req->status = DRAMReq::PROCESSING;
        uint64_t addr = req->addr + req->amt_in_process;
	bool isDCLookup = (0 == req->addr);
        if ( use_dramsim && !isDCLookup) {
#if defined(HAVE_LIBDRAMSIM)

	  // ugly hardcoded limit on outstanding requests
	  if (dramReqs.size() > 64) {
	    req->status = DRAMReq::NEW;
	    break;
	  }

	  bool ok = memSystem->willAcceptTransaction(addr);
	  if ( !ok ) break;
	  ok = memSystem->addTransaction(req->isWrite, addr);
	  if ( !ok ) break;  // This *SHOULD* always be ok
	  dbg.output(CALL_INFO, "Issued transaction for address 0x%"PRIx64"\n", addr);
	  dramReqs[addr].push_back(req);
#endif
	} else if ( use_vaultSim && !isDCLookup) {
	  dbg.output(CALL_INFO, "Issued transaction to Cube Chain for address 0x%"PRIx64"\n", addr);
	  // ugly hardcoded limit on outstanding requests
	  if (outToCubes.size() > 255) {
	    req->status = DRAMReq::NEW;
	    break;
	  }
	  SST::Interfaces::MemEvent::id_type reqID = req->reqEvent->getID();
	  assert(outToCubes.find(reqID) == outToCubes.end());
	  outToCubes[reqID] = req; // associate the memEvent w/ the DRAMReq
	  MemEvent *outgoingEvent = new MemEvent(req->reqEvent); // we make a copy, because the dramreq keeps to 'original'
	  cube_link->send(outgoingEvent); // send the event off
        } else {
            dbg.output(CALL_INFO, "Issued transaction for address 0x%"PRIx64"\n", addr);
            self_link->send(new MemCtrlEvent(req));
        }

        req->amt_in_process += requestSize;

        if ( req->amt_in_process >= req->size ) {
            dbg.output(CALL_INFO, "Completed issue of request\n");
            performRequest(req);
            requestQueue.pop_front();
        }
    }

    /* Clean out old requests */
#if 1
    while ( requests.size() ) {
      DRAMReq *req = requests.front();
      if ( DRAMReq::DONE == req->status ) {
	requests.pop_front();
	delete req;
      } else {
	break;
      }
    }
#else
    // may be faster if you have a lot of things retiring each cycle
    dramReq_t::iterator new_end = 
      remove_if(requests.begin(), requests.end(), isDone());
    requests.erase(new_end, requests.end());
#endif

    numReqOutstanding += requests.size();
    numCycles++;

	return false;
}


bool MemController::isRequestAddressValid(MemEvent *ev)
{
    Addr addr = ev->getAddr();

    if ( 0 == numPages ) {
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
    if ( 0 == numPages ) {
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
    if ((localAddr + req->reqEvent->getSize()) >= memSize) {
        _abort(MemController, "Attempting to access past the end of Memory.\n");
    }

    req->respEvent = resp;
	if ( req->isWrite ) {
		for ( size_t i = 0 ; i < req->reqEvent->getSize() ; i++ ) {
			memBuffer[localAddr + i] = req->reqEvent->getPayload()[i];
		}
        dbg.output(CALL_INFO, "Writing Memory: %zu bytes beginning at 0x%"PRIx64" [0x%02x%02x%02x%02x%02x%02x%02x%02x...\n",
                req->size, req->addr,
                memBuffer[localAddr + 0], memBuffer[localAddr + 1],
                memBuffer[localAddr + 2], memBuffer[localAddr + 3],
                memBuffer[localAddr + 4], memBuffer[localAddr + 5],
                memBuffer[localAddr + 6], memBuffer[localAddr + 7]);
	} else {
		for ( size_t i = 0 ; i < resp->getSize() ; i++ ) {
			resp->getPayload()[i] = memBuffer[localAddr + i];
		}
        dbg.output(CALL_INFO, "Reading Memory: %zu bytes beginning at 0x%"PRIx64" [0x%02x%02x%02x%02x%02x%02x%02x%02x...\n",
                req->size, req->addr,
                memBuffer[localAddr + 0], memBuffer[localAddr + 1],
                memBuffer[localAddr + 2], memBuffer[localAddr + 3],
                memBuffer[localAddr + 4], memBuffer[localAddr + 5],
                memBuffer[localAddr + 6], memBuffer[localAddr + 7]);
	}
}


void MemController::sendBusPacket(Bus::key_t key)
{
    assert(use_bus);
	for (;;) {
		if ( 0 == busReqs.size() ) {
            dbg.output(CALL_INFO, "Sending cancelation, as we have nothing in the queue.\n");
			upstream_link->send(new BusEvent(BusEvent::CancelRequest, key));
			break;
		} else {
            DRAMReq *req = busReqs.front();
			busReqs.pop_front();
            req->status = DRAMReq::DONE;
			if ( !req->canceled ) {
                MemEvent *ev = req->respEvent;
				dbg.output(CALL_INFO, "Sending (%"PRIu64", %d) in response to (%"PRIu64", %d) 0x%"PRIx64"\n",
						ev->getID().first, ev->getID().second,
						ev->getResponseToID().first, ev->getResponseToID().second,
						ev->getAddr());
                assert(ev->getID() == key);
                if ( req->respEvent->getCmd() == WriteResp ) numWrites++;
                else if ( req->respEvent->getCmd() == SupplyData ) numReadsSupplied++;
				upstream_link->send(0, new BusEvent(ev));
				break;
            } else {
                if ( req->respEvent->getID() == key ) {
                    /* Bus asked for this item, but it is canceled.
                     * This most likely means that we raced a cancelation
                     * and the bus giving us permission.  A bus cancelation
                     * will be received by the bus, and we don't need to do
                     * anything.  Pretend this thing never happened.
                     */
                    dbg.output(CALL_INFO, "Choosing to not send event for (%"PRIu64", %d), because we think it was recently canceled.\n", key.first, key.second);
                    break;
                } else {
                    dbg.output(CALL_INFO, "Skipping over canceled event (%"PRIu64", %d)\n", req->respEvent->getID().first, req->respEvent->getID().second);
                }
            }
		}
	}
}

void MemController::sendBusCancel(Bus::key_t key) {
    assert(use_bus);
    dbg.output(CALL_INFO, "Sending cancelation of event (%"PRIu64", %d)\n", key.first, key.second);
    upstream_link->send(new BusEvent(BusEvent::CancelRequest, key));
}


void MemController::sendResponse(DRAMReq *req)
{
    if ( use_bus ) {
        busReqs.push_back(req);
        Bus::key_t key = req->respEvent->getID();
        dbg.output(CALL_INFO, "Requesting bus for event (%"PRIu64", %d)\n", key.first, key.second);
        upstream_link->send(new BusEvent(BusEvent::RequestBus, key));
    } else {
        upstream_link->send(req->respEvent);
        req->status = DRAMReq::DONE;
        if ( req->respEvent->getCmd() == WriteResp ) numWrites++;
        else if ( req->respEvent->getCmd() == SupplyData ) numReadsSupplied++;
    }
}


void MemController::handleMemResponse(DRAMReq *req)
{
    dbg.output(CALL_INFO, "Finishing processing for req 0x%"PRIx64"\n", req->addr);
    req->amt_processed += requestSize;
    if ( req->amt_processed >= req->size ) {
        req->status = DRAMReq::RETURNED;
    }

    if ( DRAMReq::RETURNED == req->status ) {
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

void MemController::handleCubeEvent(SST::Event *event)
{
  MemEvent *ev = dynamic_cast<MemEvent*>(event);
  if (ev) {
    memEventToDRAMMap_t::iterator ri = outToCubes.find(ev->getResponseToID());
    if (ri != outToCubes.end()) {
      handleMemResponse(ri->second);
      outToCubes.erase(ri);
      delete event;
    } else {
      _abort(MemController, "Could not match incoming request from cubes\n");
    }
  } else {
    _abort(MemController, "Recived wrong event type from cubes\n");
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
    dbg.output(CALL_INFO, "Memory Request for 0x%"PRIx64" Finished [%zu reqs]\n", addr, reqs.size());
    assert(reqs.size());
    DRAMReq *req = reqs.front();
    reqs.pop_front();
    if ( 0 == reqs.size() )
        dramReqs.erase(addr);

    handleMemResponse(req);
}


#endif

