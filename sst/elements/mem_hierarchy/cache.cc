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


#define DPRINTF( fmt, args...) __DBG( DBG_CACHE, Cache, "%s: " fmt, getName().c_str(), ## args )

using namespace SST;
using namespace SST::MemHierarchy;

static const std::string NO_NEXT_LEVEL = "NONE";

Cache::Cache(ComponentId_t id, Params_t& params) :
	Component(id)
{
	// get parameters
	n_ways = params.find_integer("num_ways", 0);
	n_rows = params.find_integer("num_rows", 0);
	blocksize = params.find_integer("blocksize", 0);
	if ( n_ways == 0 || n_rows == 0 || blocksize == 0 ) {
		_abort(Cache, "# Ways, # Rows and Blocksize must all be >0\n");
	}

	n_upstream = params.find_integer("num_upstream", 0);
	if ( n_upstream > 0 ) {
		upstream_links = new SST::Link*[n_upstream];
		for ( int i = 0 ; i < n_upstream ; i++ ) {
			std::ostringstream linkName;
			linkName << "upstream" << i;
			std::string ln = linkName.str();
			upstream_links[i] = configureLink( ln, "50 ps",
					new Event::Handler<Cache, SourceType_t>(this,
						&Cache::handleIncomingEvent, UPSTREAM) );
			assert(upstream_links[i]);
			upstreamLinkMap[upstream_links[i]->getId()] = i;
		}
	}

	next_level_name = params.find_string("next_level", NO_NEXT_LEVEL);
	downstream_link = configureLink( "downstream",
			new Event::Handler<Cache, SourceType_t>(this,
				&Cache::handleIncomingEvent, DOWNSTREAM) );
	snoop_link = configureLink( "snoop_link", "50 ps",
			new Event::Handler<Cache, SourceType_t>(this,
				&Cache::handleIncomingEvent, SNOOP) );
	if ( snoop_link != NULL ) { // Snoop is a bus.
		snoopBusQueue.setup(this, snoop_link);
	}

	directory_link = configureLink( "directory_link",
			new Event::Handler<Cache, SourceType_t>(this,
				&Cache::handleIncomingEvent, DIRECTORY) );

	self_link = configureSelfLink("Self", params.find_string("access_time", ""),
				new Event::Handler<Cache>(this, &Cache::handleSelfEvent));

	rowshift = numBits(blocksize);
	rowmask = n_rows - 1;  // Assumption => n_rows is power of 2
	tagshift = numBits(blocksize) + numBits(n_rows);



	database = std::vector<CacheRow>(n_rows, CacheRow(this));

	// TODO:  Is this right?
	registerTimeBase("1 ns", true);
}


void Cache::handleIncomingEvent(SST::Event *event, SourceType_t src)
{
	MemEvent *ev = dynamic_cast<MemEvent*>(event);
	if (event) {
		DPRINTF("Received Event (to %s) %s 0x%lx\n", ev->getDst().c_str(), CommandString[ev->getCmd()], ev->getAddr());
		//bool to_me = (ev->getDst() == getName());
		switch (ev->getCmd()) {
		case BusClearToSend:
			snoopBusQueue.clearToSend();
			break;

		case ReadReq:
		case WriteReq:
			handleCPURequest(ev);
			break;

		case RequestData:
			handleCacheRequestEvent(ev, src);
			break;
		case SupplyData:
			handleCacheSupplyEvent(ev, src);
			break;
		case Invalidate:
			handleInvalidate(ev, src);
			break;

		default:
			/* Ignore */
			break;
		}
		delete event;
	} else {
		printf("Cache:: Error Bad Event Type!\n");
	}
}



void Cache::handleSelfEvent(SST::Event *event)
{
	SelfEvent *ev = dynamic_cast<SelfEvent*>(event);
	if ( ev ) {
		(this->*(ev->handler))(ev->event, ev->block, ev->event_source);
		delete ev;
	} else {
		_abort(Cache, "Cache::handleSelfEvent:  BAD TYPE!\n");
	}
}



void Cache::handleCPURequest(MemEvent *ev)
{
	assert(ev->getCmd() == ReadReq || ev->getCmd() == WriteReq);
	bool isRead = (ev->getCmd() == ReadReq);
	CacheBlock *block = findBlock(ev->getAddr(), false);
	DPRINTF("0x%lx %s %s\n", ev->getAddr(),
			isRead ? "READ" : "WRITE",
			(block) ? "HIT" : "MISS");
	if ( block ) {
		/* HIT */
		if ( isRead ) {
			block->locked++;
			self_link->Send(new SelfEvent(&Cache::sendCPUResponse, new MemEvent(ev), block, UPSTREAM));
		} else {
			if ( block->status == CacheBlock::EXCLUSIVE ) {
				block->locked++;
				self_link->Send(new SelfEvent(&Cache::sendCPUResponse, new MemEvent(ev), block, UPSTREAM));
			} else {
				issueInvalidate(ev, block);
			}
		}
		block->last_touched = getCurrentSimTime();
	} else {
		/* Miss */
		loadBlock(ev, UPSTREAM);
	}
}


void Cache::sendCPUResponse(MemEvent *ev, CacheBlock *block, SourceType_t src)
{
	Addr offset = ev->getAddr() - block->baseAddr;
	if ( offset+ev->getSize() > (Addr)blocksize ) {
		_abort(Cache, "Cache doesn't handle split rquests.\nReq for addr 0x%lx has offset of %lu, and size %u.  Blocksize is %u\n",
				ev->getAddr(), offset, ev->getSize(), blocksize);
	}

	MemEvent *resp = ev->makeResponse(getName());
	DPRINTF("Sending Response to CPU: (%lu, %d) in Response To (%lu, %d) [%s: 0x%lx]\n",
			resp->getID().first, resp->getID().second,
			resp->getResponseToID().first, resp->getResponseToID().second,
			CommandString[resp->getCmd()], resp->getAddr());
	if ( ev->getCmd() == ReadReq)
		resp->setPayload(ev->getSize(), &block->data[offset]);

	/* CPU is always upstream link 0 */
	upstream_links[0]->Send(resp);
	block->locked--;
	delete ev; // This should only come through handleCPURequest, which creates new events.
}


void Cache::issueInvalidate(MemEvent *ev, CacheBlock *block)
{
	if ( snoop_link ) {
		BusFinishHandlerArgs args;
		args.issueInvalidate.ev = new MemEvent(ev);
		args.issueInvalidate.block = block;
		MemEvent *invEvent = new MemEvent(getName(), block->baseAddr, Invalidate);
		block->currentEvent = invEvent;
		snoopBusQueue.request( invEvent,
				new BusFinishHandler(&Cache::finishIssueInvalidateVA, args));
	} else {
		finishIssueInvalidate(new MemEvent(ev), block);
	}
}

void Cache::finishIssueInvalidateVA(BusFinishHandlerArgs &args)
{
	MemEvent *ev = args.issueInvalidate.ev;
	CacheBlock *block = args.issueInvalidate.block;
	finishIssueInvalidate(ev, block);
}

void Cache::finishIssueInvalidate(MemEvent *ev, CacheBlock *block)
{
	/* TODO:  Deal with the lack of atomicity.  Count ACKs */
	if ( downstream_link ) {
		downstream_link->Send(new MemEvent(getName(), block->baseAddr, Invalidate));
	}
	if ( directory_link ) {
		directory_link->Send(new MemEvent(getName(), block->baseAddr, Invalidate));
	}
	for ( int i = 0 ; i < n_upstream ; i++ ) {
		if ( upstream_links[i]->getId() != ev->getLinkId() ) {
			upstream_links[i]->Send(new MemEvent(getName(), block->baseAddr, Invalidate));
		}
	}
	block->status = CacheBlock::EXCLUSIVE;
	block->currentEvent = NULL;
	// Only thing that can cause us to issue Invalidate is a WriteReq
	handleCPURequest(ev);
	delete ev;
}



void Cache::loadBlock(MemEvent *ev, SourceType_t src)
{

	CacheBlock *block = NULL;
	bool already_asked = false;
	LoadInfo_t &li = waitingLoads[addrToBlockAddr(ev->getAddr())];
	if ( li.targetBlock != NULL ) {
		block = li.targetBlock;
		already_asked = true;
	} else {
		block = findRow(ev->getAddr())->getLRU();
		block->activate(ev->getAddr());
		block->locked++;

		li.targetBlock = block;
	}

	li.list.push_back(std::make_pair(new MemEvent(ev), src));

	if ( !already_asked ) {
		if ( snoop_link ) {
			snoopBusQueue.request(
					new MemEvent(getName(), block->baseAddr, RequestData),
					NULL /* No post-send Handler */);
		}
		if ( downstream_link ) {
			downstream_link->Send(new MemEvent(getName(), block->baseAddr, RequestData));
		}
	}
}





void Cache::handleCacheRequestEvent(MemEvent *ev, SourceType_t src)
{
	CacheBlock *block = findBlock(ev->getAddr(), false);
	if ( block ) {
		supplyMap_t::key_type supplyMapKey = std::make_pair(block->baseAddr, src);
		/* Hit */

		supplyMap_t::iterator supMapI = supplyInProgress.find(supplyMapKey);
		if ( supMapI != supplyInProgress.end() && !supMapI->second.canceled ) {
			// we're already working on this
			DPRINTF("Detected that we're already working on this\n");
			return;
		}

		supplyInProgress[supplyMapKey] = SupplyInfo(NULL);
		self_link->Send(new SelfEvent(&Cache::supplyData, new MemEvent(ev), block, src));
		block->locked++;
		block->last_touched = getCurrentSimTime();
	} else {
		/* Miss */
		switch ( src ) {
		case SNOOP:
			if ( ev->getDst() == getName() )
				/* Ignore if not to us */
				loadBlock(ev, src);
			break;
		default:
			loadBlock(ev, src);
		}
	}
}


void Cache::supplyData(MemEvent *ev, CacheBlock *block, SourceType_t src)
{
	supplyMap_t::key_type supplyMapKey = std::make_pair(block->baseAddr, src);
	supplyMap_t::iterator supMapI = supplyInProgress.find(supplyMapKey);
	assert(supMapI != supplyInProgress.end());

	if ( supMapI->second.canceled ) {
		DPRINTF("Request has been canceled!\n");
		supplyInProgress.erase(supMapI);
		block->locked--;
		return;
	}


	MemEvent *resp = new MemEvent(getName(), block->baseAddr, SupplyData);
	resp->setPayload(block->data);

	if ( src != SNOOP ) {
		SST::Link *link = getLink(src, ev->getLinkId());
		link->Send(resp);
		block->locked--;
		supplyInProgress.erase(supMapI);
	} else {
		BusFinishHandlerArgs args;
		args.supplyData.block = block;
		args.supplyData.src = src;
		supMapI->second.busEvent = resp;
		snoopBusQueue.request( resp,
				new BusFinishHandler(&Cache::finishBusSupplyData, args));
	}
	delete ev;
}


void Cache::finishBusSupplyData(BusFinishHandlerArgs &args)
{
	args.supplyData.block->locked--;
	supplyMap_t::key_type supplyMapKey = std::make_pair(args.supplyData.block->baseAddr, args.supplyData.src);

	supplyMap_t::iterator supMapI = supplyInProgress.find(supplyMapKey);
	assert(supMapI != supplyInProgress.end());
	supplyInProgress.erase(supMapI);
}



void Cache::handleCacheSupplyEvent(MemEvent *ev, SourceType_t src)
{
	/*
	 * If snoop, cancel any supplies we're trying to do
	 * Check to see if we're trying to load this data, if so, handle
	 */
	if ( src == SNOOP ) {
		supplyMap_t::key_type supplyMapKey = std::make_pair(ev->getAddr(), src);
		supplyMap_t::iterator supMapI = supplyInProgress.find(supplyMapKey);
		if ( supMapI != supplyInProgress.end() ) {
			// Mark it canceled
			DPRINTF("Marking request for 0x%lx as canceled\n", ev->getAddr());
			supMapI->second.canceled = true;
			if ( supMapI->second.busEvent != NULL ) {
				// Bus requested.  Cancel it, too
				BusFinishHandler *handler = snoopBusQueue.cancelRequest(supMapI->second.busEvent);
				if ( handler ) {
					handler->args.supplyData.block->locked--;
					delete handler;
				}
			}
		}
	}

	/* Check to see if we're trying to load this data */
	LoadList_t::iterator i = waitingLoads.find(ev->getAddr());
	if ( i != waitingLoads.end() ) {
		LoadInfo_t &li = i->second;

		updateBlock(ev, li.targetBlock);
		li.targetBlock->locked--;
		li.targetBlock->status = CacheBlock::SHARED;

		for ( uint32_t n = 0 ; n < li.list.size() ; n++ ) {
			std::pair<MemEvent *, SourceType_t> oldEV = li.list[n];
			/* If this was from the Snoop Bus, and we've got other cache's asking
			 * for this data over the snoop bus, we can assume they saw it, and
			 * we don't need to reprocess them.
			 */
			if ( src == SNOOP && oldEV.second == SNOOP ) {
				delete oldEV.first;
				continue;
			}

			/* Make them be processed in order, so pass 'n' as a delay */
			self_link->Send(n, new SelfEvent(&Cache::finishSupplyEvent,
						oldEV.first, li.targetBlock, oldEV.second));
		}
		waitingLoads.erase(i);
	} else {
		assert ( src == SNOOP );
		assert ( ev->getDst() != getName() ); // Unmatched reply
	}

}


void Cache::finishSupplyEvent(MemEvent *origEV, CacheBlock *block, SourceType_t origSrc)
{
	handleIncomingEvent(origEV, origSrc);
}


void Cache::handleInvalidate(MemEvent *ev, SourceType_t src)
{
	if ( ev->getSrc() == getName() ) return; // Don't cancel our own.
	CacheBlock *block = findBlock(ev->getAddr());
	if ( !block ) return;

	if ( waitingForInvalidate(block) ) {
		cancelInvalidate(block); /* Should cause a re-issue of the write */
	}

	if ( block->status == CacheBlock::SHARED ) block->status = CacheBlock::INVALID;
	if ( block->status == CacheBlock::EXCLUSIVE ) {
		writebackBlock(block, CacheBlock::INVALID);
	}
}

bool Cache::waitingForInvalidate(CacheBlock *block)
{
	return ( block->currentEvent && block->currentEvent->getCmd() == Invalidate);
}

void Cache::cancelInvalidate(CacheBlock *block)
{
	assert( waitingForInvalidate(block) );
	BusFinishHandler *handler = snoopBusQueue.cancelRequest(block->currentEvent);
	block->currentEvent = NULL;
	handleCPURequest(handler->args.issueInvalidate.ev);
	delete handler;
}


void Cache::writebackBlock(CacheBlock *block, CacheBlock::BlockStatus newStatus)
{
	MemEvent *ev = new MemEvent(getName(), block->baseAddr, SupplyData);
	ev->setFlag(MemEvent::F_WRITEBACK);
	ev->setPayload(block->data);

	if ( snoop_link ) {
		block->locked++;
		BusFinishHandlerArgs args;
		args.writebackBlock.block = block;
		args.writebackBlock.newStatus = newStatus;
		args.writebackBlock.decrementLock = true;
		snoopBusQueue.request(new MemEvent(ev),
				new BusFinishHandler(&Cache::finishWritebackBlockVA, args));
	} else {
		finishWritebackBlock(block, newStatus, false);
	}
	delete ev;
}


void Cache::finishWritebackBlockVA(BusFinishHandlerArgs& args)
{
	CacheBlock *block = args.writebackBlock.block;
	CacheBlock::BlockStatus newStatus = args.writebackBlock.newStatus;
	bool decrementLock = args.writebackBlock.decrementLock;
	finishWritebackBlock(block, newStatus, decrementLock);
}

void Cache::finishWritebackBlock(CacheBlock *block, CacheBlock::BlockStatus newStatus, bool decrementLock)
{

	MemEvent *ev = new MemEvent(getName(), block->baseAddr, SupplyData);
	ev->setFlag(MemEvent::F_WRITEBACK);
	ev->setPayload(block->data);

	if ( decrementLock ) /* AKA, sent on the Snoop Bus */
		block->locked--;

	if ( downstream_link )
		downstream_link->Send(new MemEvent(ev));
	if ( directory_link )
		directory_link->Send(new MemEvent(ev));

	delete ev;

	assert(block->locked == 0);
	block->status = newStatus;
}



/**** Utility Functions ****/

void Cache::updateBlock(MemEvent *ev, CacheBlock *block)
{
	if (ev->getSize() == (uint32_t)blocksize) {
		// Assumption:  if sizes are equal, base addresses are, too
		std::copy(ev->getPayload().begin(), ev->getPayload().end(), block->data.begin());
	} else {
		// Update a portion of the block
		Addr offset = ev->getAddr() - block->baseAddr;
		for ( uint32_t i = 0 ; i < ev->getSize() ; i++ ) {
			block->data[offset+i] = ev->getPayload()[i];
		}
	}
	block->last_touched = getCurrentSimTime();
}



SST::Link* Cache::getLink(SourceType_t type, int link_id)
{
	switch (type) {
	case DOWNSTREAM:
		return downstream_link;
	case SNOOP:
		return snoop_link;
	case DIRECTORY:
		return directory_link;
	case UPSTREAM:
		return upstream_links[upstreamLinkMap[link_id]];
	case SELF:
		return self_link;
	}
	return NULL;
}


int Cache::numBits(int x)
{
	return (int)(log(x)/log(2));
}

Addr Cache::addrToTag(Addr addr)
{
	return (addr >> tagshift);
}

Addr Cache::addrToBlockAddr(Addr addr)
{
	return addr & ~(blocksize-1);
}

Cache::CacheBlock* Cache::findBlock(Addr addr, bool emptyOK)
{
	CacheBlock *block = NULL;
	CacheRow *row = findRow(addr);
	uint32_t tag = addrToTag(addr);
	for ( int i = 0 ; i < n_ways ; i++ ) {
		CacheBlock *b = &(row->blocks[i]);
		if ( !b->isInvalid() && b->tag == tag ) {
			block = b;
			break;
		}
	}
	if ( ! block && emptyOK ) { // See if we can find an empty
		for ( int i = 0 ; i < n_ways ; i++ ) {
			CacheBlock *b = &(row->blocks[i]);
			if ( b->isInvalid() ) {
				block = b;
				break;
			}
		}
	}
	return block;
}


Cache::CacheRow* Cache::findRow(Addr addr)
{
	/* Calculate Row number */
	Addr row = (addr >> rowshift) & rowmask;
	assert(row < (Addr)n_rows);
	return &database[row];
}

