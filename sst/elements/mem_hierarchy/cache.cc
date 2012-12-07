// Copyright 2012 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2012, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"
#include "sst/core/serialization/element.h"
#include "sst/core/simulation.h"
#include <assert.h>

#include "sst/core/element.h"

#include "cache.h"
#include "memEvent.h"


#define DPRINTF( fmt, args...) __DBG( DBG_CACHE, Cache, fmt, ## args )

using namespace SST;
using namespace SST::MemHierarchy;

static inline uint32_t numBits(uint32_t x) {
	return (uint32_t)(log(x)/log(2));
}

Cache::Cache(ComponentId_t id, Params_t& params) :
	Component(id)
{
	// get parameters
	n_ways = (uint32_t)params.find_integer("num_ways", 0);
	n_rows = (uint32_t)params.find_integer("num_rows", 0);
	blocksize = (uint32_t)params.find_integer("blocksize", 0);
	if ( n_ways == 0 || n_rows == 0 || blocksize == 0 ) {
		_abort(Cache, "# Ways, # Rows and Blocksize must all be >0\n");
	}
	DPRINTF("Cache Config:\n\t%u rows\n\t%u ways\n\t%u byte blocks\n",
			n_rows, n_ways, blocksize);

	rowshift = numBits(blocksize);
	rowmask = n_rows - 1;  // Assumption => n_rows is power of 2
	tagshift = numBits(blocksize) + numBits(n_rows);


	// TODO:  Use proper time conversion
	access_time = (uint32_t)params.find_integer("access_time", 0);
	if ( access_time < 1 ) _abort(Cache,"No Access Time set\n");

	// tell the simulator not to end without us
	registerExit();

	// configure out links
	cpu_link = configureLink( "cpu_link",
			new Event::Handler<Cache>(this, &Cache::handleCpuEvent) );
	bus_link = configureLink( "bus_link",
			new Event::Handler<Cache>(this, &Cache::handleBusEvent) );
	registerTimeBase("1 ns", true);
	assert(bus_link);

	busRequested = false;

	database = std::vector<CacheRow>(n_rows, CacheRow(this));

}

Cache::Cache() :
	Component(-1)
{
	// for serialization only
}


// incoming events are scanned and deleted
void Cache::handleCpuEvent(Event *ev)
{
	MemEvent *event = dynamic_cast<MemEvent*>(ev);
	if (event) {
		switch(event->getCmd()) {
		case ReadReq:
			handleReadReq(event, false);
			break;
		case WriteReq:
			handleWriteReq(event, false);
			break;
		default:
			_abort(Cache, "Cache doesn't recognize command %d from CPU!\n", event->getCmd());
		}
		delete event;
	} else {
		printf("Error! Bad Event Type!\n");
	}
}


void Cache::handleBusEvent(Event *ev)
{
	MemEvent *event = dynamic_cast<MemEvent*>(ev);
	if (event) {
		switch(event->getCmd()) {
		case ReadReq:
			handleReadReq(event, true);
			break;
		case ReadResp:
			handleReadResponse(event);
			break;
		case WriteReq:
			handleWriteReq(event, true);
			break;
		case WriteResp: /* TODO */
			break;
		case Invalidate:
			handleInvalidate(event);
			break;
		case Fetch:
			handleFetch(event, false);
			break;
		case Fetch_Invalidate:
			handleFetch(event, true);
			break;
		case FetchResp:
			/* Ignore */
			break;
		case BusClearToSend:
			sendBusPacket();
			break;
		default:
			_abort(Cache, "Cache doesn't recognize command %d from Bus\n", event->getCmd());
		}
		delete event;
	} else {
		printf("Error! Bad Event Type!\n");
	}
}


void Cache::requestBus(BusRequest *req, SimTime_t delay)
{
	if ( req != NULL ) {
		packetQueue.push_back(req);
	}
	if ( !busRequested ) { // Don't request if we've already got a request
		bus_link->Send(delay, new MemEvent(NULL, RequestBus));
		busRequested = true;
	}
}


void Cache::sendBusPacket(void)
{
	if ( packetQueue.size() == 0 ) {
		bus_link->Send(new MemEvent(NULL, CancelBusRequest));
		busRequested = false;
	} else {
		BusRequest *req = packetQueue.front();
		packetQueue.pop_front();
		bus_link->Send(req->msg);
		busRequested = false;
		if ( req->finishFunc ) {
			(this->*(req->finishFunc))(req);
		}
		delete req;
		if ( packetQueue.size() > 0 ) {
			requestBus(NULL);
		}
	}
}


void Cache::handleReadReq(MemEvent *ev, bool isSnoop)
{
	/* TODO:  Deal with requests that split block boundaries */
	CacheBlock *block = findBlock(ev->getAddr());
	DPRINTF("Read Request %sfor addr 0x%lx (%s)\n",
			isSnoop ? "(Snoop)" : "", ev->getAddr(), block ? "Hit" : "Miss");
	if ( block ) { /* Hit! */
		if ( isSnoop && block->status == CacheBlock::EXCLUSIVE ) {
			writeBack(block, &Cache::finishWriteBackAsShared);
		}
		// Update LRU
		block->last_touched = getCurrentSimTime();
		sendData(ev, block, isSnoop);
	} else { /* Miss! */
		if ( !isSnoop ) {
			handleMiss(ev);
		}
	}
}


void Cache::handleReadResponse(MemEvent *ev)
{
	RequestHold::RequestList *list = awaitingResponse.findReqs(ev->getAddr());
	if ( list == NULL ) {
		// Can't find matching request
		_abort(Cache, "What to do with unmatched responses! 0x%lx\n",
				ev->getAddr());
	}


	CacheBlock *block = list->block;
	/* Update cache with this block */
	assert(block->tag == addrToTag(ev->getAddr()));
	updateBlock(ev, block);
	block->status = CacheBlock::SHARED;


	while ( list->requests.size() > 0 ) {
		MemEvent *origEvent = list->requests.front();
		list->requests.pop_front();
		DPRINTF("Clearing response for 0x%lx req (%lu, %d)\n", ev->getAddr(),
				origEvent->getID().first, origEvent->getID().second);

		/* handleCpuEvent() will delete origEvent */
		/* Can assume Cpu-style event as snoops don't cause us to miss */
		/* BJM:  TODO: Validate this assumption, please */
		handleCpuEvent(origEvent);
	}


	awaitingResponse.clearReqs(ev->getAddr());
}


void Cache::handleWriteReq(MemEvent *ev, bool isSnoop)
{
	CacheBlock *block = findBlock(ev->getAddr());
	DPRINTF("Write Request %sfor addr 0x%lx (%s)\n",
			isSnoop ? "" : "(snoop) ", ev->getAddr(), block ? "Hit" : "Miss");
	assert(!isSnoop);  // Should get an invalidate, not a write req
	if ( block ) { /* Hit! */
		if ( block->status == CacheBlock::SHARED ) {
			sendInvalidate(ev, block);
		} else {
			// We're already exclusive
			updateBlock(ev, block);
		}
	} else { /* Miss! */
		handleMiss(ev);
	}
}


void Cache::handleInvalidate(MemEvent *ev)
{
	CacheBlock *block = findBlock(ev->getAddr());
	if ( block ) {
		assert( block->status == CacheBlock::SHARED );  // Shouldn't be Exclusive or Invalid
		block->status = CacheBlock::INVALID;
		/* TODO:  Send Acknowledge */
	}
}


void Cache::handleFetch(MemEvent *ev, bool invalidate)
{
	CacheBlock *block = findBlock(ev->getAddr());
	assert( block );  // Shouldn't be asked for a fetch if we don't have it.
	MemEvent *me = new MemEvent(block->baseAddr, FetchResp);
	me->setSize(blocksize);
	me->setPayload(block->data);
	me->setDst(ev->getSrc());
	requestBus(new BusRequest(me, block, false, &Cache::finishFetch, NULL, (void*)invalidate), access_time);
}

void Cache::finishFetch(BusRequest *req)
{
	bool invalidate = (bool)req->ud;
	req->block->status = invalidate ? CacheBlock::INVALID : CacheBlock::SHARED;
}


void Cache::handleMiss(MemEvent *ev)
{
	/* Check to see if there are outstanding requests */
	if ( !awaitingResponse.exists(addrToBlockAddr(ev->getAddr())) ) {
		/* Select target row */
		CacheRow *row = findRow(ev->getAddr());
		CacheBlock *block = row->getLRU();

		/* Take care of writeback, if needed */
		if ( block->status == CacheBlock::EXCLUSIVE ) {
			writeBack(block, &Cache::advanceMiss, new MemEvent(ev));
			/* Will return when placed on bus */
			return;
		}

		/* Request block */
		block->activate(ev->getAddr());

		MemEvent *me = new MemEvent(block->baseAddr, ReadReq);
		me->setSize(blocksize);

		requestBus(new BusRequest(me, block, false));
		awaitingResponse.storeReq(block, new MemEvent(ev));
		DPRINTF("Storing request (%lu, %d) for addr 0x%lx\n", ev->getID().first, ev->getID().second, block->baseAddr);
	} else {
		awaitingResponse.addReq(addrToBlockAddr(ev->getAddr()), new MemEvent(ev));
		DPRINTF("Adding request (%lu, %d) for addr 0x%lx\n", ev->getID().first, ev->getID().second, addrToBlockAddr(ev->getAddr()));
	}
}


void Cache::advanceMiss(BusRequest *req)
{
	/* writeBack finished  Go on with original request */
	MemEvent *ev = static_cast<MemEvent*>(req->ud);
	handleMiss(ev);
	delete ev;
}



void Cache::writeBack(CacheBlock *block, postSendHandler finishFunc, void *ud)
{
	assert( block );
	MemEvent *me = new MemEvent(block->baseAddr, WriteBack);
	me->setSize(blocksize);
	me->setPayload(block->data);
	requestBus(new BusRequest(me, block, false, finishFunc, NULL, ud), access_time);
}


void Cache::finishWriteBackAsShared(BusRequest *req)
{
	req->block->status = CacheBlock::SHARED;
}


void Cache::sendData(MemEvent *ev, CacheBlock *block, bool isSnoop)
{
	Addr offset = ev->getAddr() - block->baseAddr;
	if ( offset+ev->getSize() > blocksize ) {
		_abort(Cache, "Cache doesn't handle split rquests.\nReq for addr 0x%lx has offset of %lu, and size %u.  Blocksize is %u\n",
				ev->getAddr(), offset, ev->getSize(), blocksize);
	}

	MemEvent *resp = ev->makeResponse();
	resp->setPayload(ev->getSize(), &block->data[offset]);

	if ( isSnoop ) {
		resp->setDst(ev->getSrc());
		requestBus(new BusRequest(resp, block, false), access_time);
	} else {
		cpu_link->Send(access_time, resp);
	}
}


void Cache::sendInvalidate(MemEvent *ev, CacheBlock *block)
{
	MemEvent *me = new MemEvent(block->baseAddr, Invalidate);
	requestBus(new BusRequest(me, block, true, &Cache::finishInvalidate, &Cache::cancelInvalidate, (void*)new MemEvent(ev)));
}


void Cache::finishInvalidate(BusRequest *req)
{
	/* Assumption:  Only called through write requests.  Need to process the write now */
	/* Assumption:  Bus-based.  Can fire off as soon as we get the bus to send invalidates
	 *     TODO     This assumption is wrong.  Need to wait until they're all actually processed.
	 */
	updateBlock(req->msg, req->block);
}


void Cache::cancelInvalidate(BusRequest *req)
{
	/* Somebody else got ahead of us.  Need to retry */
	MemEvent *origEvent = static_cast<MemEvent*>(req->ud);
	handleWriteReq(origEvent, false);
}


void Cache::updateBlock(MemEvent *ev, CacheBlock *block)
{
	assert(ev->getSize() == blocksize);
	std::copy(ev->getPayload().begin(), ev->getPayload().end(), block->data.begin());
	block->last_touched = getCurrentSimTime();
}


Addr Cache::addrToTag(Addr addr)
{
	return (addr >> tagshift);
}

Addr Cache::addrToBlockAddr(Addr addr)
{
	return addr & ~(blocksize-1);
}

Cache::CacheBlock* Cache::findBlock(Addr addr)
{
	CacheBlock *block = NULL;
	CacheRow *row = findRow(addr);
	uint32_t tag = addrToTag(addr);
	for ( uint32_t i = 0 ; i < n_ways ; i++ ) {
		CacheBlock *b = &(row->blocks[i]);
		if ( b->isValid() && b->tag == tag ) {
			block = b;
			break;
		}
	}
	return block;
}


Cache::CacheRow* Cache::findRow(Addr addr)
{
	/* Calculate Row number */
	Addr row = (addr >> rowshift) & rowmask;
	assert(row < n_rows);
	return &database[row];
}


// Element Libarary / Serialization stuff

