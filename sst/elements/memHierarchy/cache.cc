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

#include <sstream>
#include <algorithm>
#include <iomanip>
#include <assert.h>

#include <sst_config.h>
#include <sst/core/serialization/element.h>
#include <sst/core/simulation.h>
#include <sst/core/element.h>

#include "cache.h"


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

	//  Is this right?
	registerTimeBase("2 ns", true);

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
			//upstream_links[i]->sendInitData("SST::Interfaces::MemEvent");
			upstreamLinkMap[upstream_links[i]->getId()] = i;
		}
	}

	next_level_name = params.find_string("next_level", NO_NEXT_LEVEL);
	downstream_link = configureLink( "downstream",
			new Event::Handler<Cache, SourceType_t>(this,
				&Cache::handleIncomingEvent, DOWNSTREAM) );
	//if ( downstream_link ) downstream_link->sendInitData("SST::Interfaces::MemEvent");
	snoop_link = configureLink( "snoop_link", "50 ps",
			new Event::Handler<Cache, SourceType_t>(this,
				&Cache::handleIncomingEvent, SNOOP) );
	//if ( snoop_link ) snoop_link->sendInitData("SST::Interfaces::MemEvent");
	if ( snoop_link != NULL ) { // Snoop is a bus.
		snoopBusQueue.setup(this, snoop_link);
	}

	directory_link = configureLink( "directory_link",
			new Event::Handler<Cache, SourceType_t>(this,
				&Cache::handleIncomingEvent, DIRECTORY) );
	//if ( directory_link ) directory_link->sendInitData("SST::Interfaces::MemEvent");

	self_link = configureSelfLink("Self", params.find_string("access_time", ""),
				new Event::Handler<Cache>(this, &Cache::handleSelfEvent));

	rowshift = numBits(blocksize);
	rowmask = n_rows - 1;  // Assumption => n_rows is power of 2
	tagshift = numBits(blocksize) + numBits(n_rows);



	database = std::vector<CacheRow>(n_rows, CacheRow(this));
	for ( int r = 0 ; r < n_rows ; r++ ) {
		for ( int c = 0 ; c < n_ways ; c++ ) {
			database[r].blocks[c].row = r;
			database[r].blocks[c].col = c;
		}
	}


	// Let the Simulation know we use the interface
    Simulation::getSimulation()->requireEvent("interfaces.MemEvent");


	num_read_hit = 0;
	num_read_miss = 0;
	num_supply_hit = 0;
	num_supply_miss = 0;
	num_write_hit = 0;
	num_write_miss = 0;
	num_upgrade_miss = 0;

}

int Cache::Finish(void)
{
	printf("Cache %s stats:\n"
			"\t# Read    Hits:      %lu\n"
			"\t# Read    Misses:    %lu\n"
			"\t# Supply  Hits:      %lu\n"
			"\t# Supply  Misses:    %lu\n"
			"\t# Write   Hits:      %lu\n"
			"\t# Write   Misses:    %lu\n"
			"\t# Upgrade Misses:    %lu\n",
			getName().c_str(),
			num_read_hit, num_read_miss,
			num_supply_hit, num_supply_miss,
			num_write_hit, num_write_miss,
			num_upgrade_miss);
	printCache();
	return 0;
}


void Cache::handleIncomingEvent(SST::Event *event, SourceType_t src)
{
	handleIncomingEvent(event, src, true);
}

void Cache::handleIncomingEvent(SST::Event *event, SourceType_t src, bool firstTimeProcessed)
{
	MemEvent *ev = dynamic_cast<MemEvent*>(event);
	if (event) {
		DPRINTF("Received Event (to %s) %s 0x%lx\n", ev->getDst().c_str(), CommandString[ev->getCmd()], ev->getAddr());
		switch (ev->getCmd()) {
		case BusClearToSend:
			snoopBusQueue.clearToSend();
			break;

		case ReadReq:
		case WriteReq:
			handleCPURequest(ev, firstTimeProcessed);
			break;

		case RequestData:
			handleCacheRequestEvent(ev, src, firstTimeProcessed);
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

void Cache::retryEvent(MemEvent *ev, CacheBlock *block, SourceType_t src)
{
	handleIncomingEvent(ev, src, false);
}


void Cache::handleCPURequest(MemEvent *ev, bool firstProcess)
{
	assert(ev->getCmd() == ReadReq || ev->getCmd() == WriteReq);
	bool isRead = (ev->getCmd() == ReadReq);
	CacheBlock *block = findBlock(ev->getAddr(), false);
	DPRINTF("(%lu, %d) 0x%lx %s %s (block 0x%lx)\n",
			ev->getID().first, ev->getID().second,
			ev->getAddr(),
			isRead ? "READ" : "WRITE",
			(block) ? "HIT" : "MISS",
			addrToBlockAddr(ev->getAddr()));
	if ( block ) {
		/* HIT */
		if ( isRead ) {
			block->locked++;
			if ( firstProcess ) num_read_hit++;
			self_link->Send(1, new SelfEvent(&Cache::sendCPUResponse, new MemEvent(ev), block, UPSTREAM));
		} else {
			if ( block->status == CacheBlock::EXCLUSIVE ) {
				if ( firstProcess ) num_write_hit++;
				block->locked++;
				updateBlock(ev, block);
				self_link->Send(1, new SelfEvent(&Cache::sendCPUResponse, new MemEvent(ev), block, UPSTREAM));
			} else {
				if ( firstProcess ) num_upgrade_miss++;
				issueInvalidate(ev, block);
			}
		}
		block->last_touched = getCurrentSimTime();
	} else {
		if ( firstProcess ) {
			if ( isRead )
				num_read_miss++;
			else
				num_write_miss++;
		}
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

	MemEvent *resp = ev->makeResponse(this);
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
		MemEvent *invEvent = new MemEvent(this, block->baseAddr, Invalidate);
		block->currentEvent = invEvent;
		DPRINTF("Enqueuing request to Invalidate block 0x%lx\n", block->baseAddr);
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
		downstream_link->Send(new MemEvent(this, block->baseAddr, Invalidate));
	}
	if ( directory_link ) {
		directory_link->Send(new MemEvent(this, block->baseAddr, Invalidate));
	}
	for ( int i = 0 ; i < n_upstream ; i++ ) {
		if ( upstream_links[i]->getId() != ev->getLinkId() ) {
			upstream_links[i]->Send(new MemEvent(this, block->baseAddr, Invalidate));
		}
	}
	block->status = CacheBlock::EXCLUSIVE;
	block->currentEvent = NULL;
	// Only thing that can cause us to issue Invalidate is a WriteReq
	handleCPURequest(ev, false);
	delete ev;
}



void Cache::loadBlock(MemEvent *ev, SourceType_t src)
{

	CacheBlock *block = NULL;
	bool already_asked = false;
	LoadList_t::iterator i = findWaitingLoad(addrToBlockAddr(ev->getAddr()), blocksize);
	LoadInfo_t *li;
	if ( i != waitingLoads.end() ) {
		DPRINTF("Adding to existing outstanding Load for this block.\n");
		li = &(*i);
		assert (li->addr == addrToBlockAddr(ev->getAddr()));
	} else {
		DPRINTF("No existing Load for this block.  Creating.\n");
		waitingLoads.push_back(LoadInfo_t(addrToBlockAddr(ev->getAddr())));
		i = findWaitingLoad(addrToBlockAddr(ev->getAddr()), blocksize);
		assert ( i != waitingLoads.end() );
		li = &(*i);
		li->addr = addrToBlockAddr(ev->getAddr());
	}


	if ( li->targetBlock != NULL ) {
		block = li->targetBlock;
		already_asked = true;
	} else {
		block = findRow(ev->getAddr())->getLRU();
		if ( !block ) { // Row is full, wait for one to be available
			self_link->Send(1, new SelfEvent(&Cache::retryEvent, new MemEvent(ev), NULL, src));
			return;
		}
		block->activate(ev->getAddr());
		block->locked++;

		li->targetBlock = block;
	}

	li->list.push_back(LoadInfo_t::LoadElement_t(new MemEvent(ev), src, getCurrentSimTime()));

	if ( !already_asked ) {
		if ( snoop_link ) {
			MemEvent *req = new MemEvent(this, block->baseAddr, RequestData);
			req->setSize(blocksize);
			if ( next_level_name != NO_NEXT_LEVEL ) req->setDst(next_level_name);
			DPRINTF("Enqueuing request to load block 0x%lx\n", block->baseAddr);
			BusFinishHandlerArgs args;
			args.loadBlock.loadInfo = li;
			snoopBusQueue.request( req, new BusFinishHandler(&Cache::finishLoadBlockBus, args));
			li->busEvent = req;
		}
		if ( downstream_link ) {
			downstream_link->Send(new MemEvent(this, block->baseAddr, RequestData));
		}
	}
}


void Cache::finishLoadBlockBus(BusFinishHandlerArgs &args)
{
	args.loadBlock.loadInfo->busEvent = NULL;
}



void Cache::handleCacheRequestEvent(MemEvent *ev, SourceType_t src, bool firstProcess)
{
	if ( src == SNOOP && ev->getSrc() == getName() ) return; // We sent it, ignore

	CacheBlock *block = findBlock(ev->getAddr(), false);
	DPRINTF("0x%lx %s %s (block 0x%lx)\n", ev->getAddr(),
			(src == SNOOP && ev->getDst() != getName()) ? "SNOOP" : "",
			(block) ? "HIT" : "MISS",
			addrToBlockAddr(ev->getAddr()));
	if ( block ) {
		if ( firstProcess ) num_supply_hit++;
		supplyMap_t::key_type supplyMapKey = std::make_pair(block->baseAddr, src);
		/* Hit */

		supplyMap_t::iterator supMapI = supplyInProgress.find(supplyMapKey);
		if ( supMapI != supplyInProgress.end() && !supMapI->second.canceled ) {
			// we're already working on this
			DPRINTF("Detected that we're already working on this\n");
			return;
		}

		supplyInProgress[supplyMapKey] = SupplyInfo(NULL);
		self_link->Send(1, new SelfEvent(&Cache::supplyData, new MemEvent(ev), block, src));
		block->locked++;
		block->last_touched = getCurrentSimTime();
	} else {
		/* Miss */
		if ( src != SNOOP || ev->getDst() == getName() ) {
			if ( firstProcess) num_supply_miss++;
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


	MemEvent *resp = new MemEvent(this, block->baseAddr, SupplyData);
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
		DPRINTF("Enqueuing request to supply block 0x%lx\n", block->baseAddr);
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
	if ( src == SNOOP && ev->getSrc() == getName() ) return; // We sent it
	/*
	 * If snoop, cancel any supplies we're trying to do
	 * Check to see if we're trying to load this data, if so, handle
	 */
	if ( src == SNOOP ) {
		if ( ev->getSize() >= blocksize ) {
			Addr blkAddr = addrToBlockAddr(ev->getAddr());
			while ( blkAddr < (ev->getAddr() + ev->getSize()) ) {
				supplyMap_t::key_type supplyMapKey = std::make_pair(blkAddr, src);
				supplyMap_t::iterator supMapI = supplyInProgress.find(supplyMapKey);
				if ( supMapI != supplyInProgress.end() ) {
					// Mark it canceled
					DPRINTF("Marking request for 0x%lx as canceled\n", ev->getAddr());
					supMapI->second.canceled = true;
					if ( supMapI->second.busEvent != NULL ) {
						// Bus requested.  Cancel it, too
						DPRINTF("Canceling Bus Request for Supply on 0x%lx\n", supMapI->second.busEvent->getAddr());
						BusFinishHandler *handler = snoopBusQueue.cancelRequest(supMapI->second.busEvent);
						if ( handler ) {
							handler->args.supplyData.block->locked--;
							delete handler;
						}
					}
				}
				blkAddr += blocksize;
			}
		}
	}

	/* Check to see if we're trying to load this data */
	LoadList_t::iterator i = findWaitingLoad(ev->getAddr(), ev->getSize());
	if ( i != waitingLoads.end() ) {
		while ( i != waitingLoads.end() ) {
			LoadInfo_t &li = *i;
			if ( li.busEvent ) {
				DPRINTF("Canceling Bus Request for Load on 0x%lx\n", li.busEvent->getAddr());
				snoopBusQueue.cancelRequest(li.busEvent);
			}

			if ( ev->getSize() < blocksize ) {
				// This isn't enough for us, but may satisfy others
				DPRINTF("Not enough info in block (%u vs %u blocksize)\n",
						ev->getSize(), blocksize);
				li.targetBlock->status = CacheBlock::INVALID;
				li.targetBlock->locked--;

				// Delete events that we would be reprocessing
				for ( uint32_t n = 0 ; n < li.list.size() ; n++ ) {
					LoadInfo_t::LoadElement_t &oldEV = li.list[n];
					if ( src == SNOOP && oldEV.src == SNOOP ) {
						delete oldEV.ev;
					}
				}

			} else {
				DPRINTF("Updating block 0x%lx\n", li.targetBlock->baseAddr);
				updateBlock(ev, li.targetBlock);
				li.targetBlock->status = CacheBlock::SHARED;
				li.targetBlock->locked--;

				for ( uint32_t n = 0 ; n < li.list.size() ; n++ ) {
					LoadInfo_t::LoadElement_t &oldEV = li.list[n];
					/* If this was from the Snoop Bus, and we've got other cache's asking
					 * for this data over the snoop bus, we can assume they saw it, and
					 * we don't need to reprocess them.
					 */
					if ( src == SNOOP && oldEV.src == SNOOP ) {
						delete oldEV.ev;
						continue;
					}

					/* Make them be processed in order, so pass 'n' as a delay */
					self_link->Send(n, new SelfEvent(&Cache::finishSupplyEvent,
								oldEV.ev, li.targetBlock, oldEV.src));
				}
			}
			waitingLoads.erase(i);
			i = findWaitingLoad(ev->getAddr(), ev->getSize());
		}
	} else {
		assert ( src == SNOOP );
		if ( ev->getDst() == getName() ) {
			DPRINTF("WARNING:  Unmatched message.  Hopefully we recently just canceled this request, and our sender didn't get the memo.\n");
#if 0
			printCache();
			_abort(Cache, "%s Received an unmatched message!\n", getName().c_str());
#endif
		}
	}

}


void Cache::finishSupplyEvent(MemEvent *origEV, CacheBlock *block, SourceType_t origSrc)
{
	handleIncomingEvent(origEV, origSrc, false);
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
	DPRINTF("Canceling Bus Request for Invalidate 0x%lx\n", block->baseAddr);
	BusFinishHandler *handler = snoopBusQueue.cancelRequest(block->currentEvent);
	block->currentEvent = NULL;
	handleCPURequest(handler->args.issueInvalidate.ev, false);
	delete handler;
}


void Cache::writebackBlock(CacheBlock *block, CacheBlock::BlockStatus newStatus)
{
	MemEvent *ev = new MemEvent(this, block->baseAddr, SupplyData);
	ev->setFlag(MemEvent::F_WRITEBACK);
	ev->setPayload(block->data);

	if ( snoop_link ) {
		block->locked++;
		BusFinishHandlerArgs args;
		args.writebackBlock.block = block;
		args.writebackBlock.newStatus = newStatus;
		args.writebackBlock.decrementLock = true;
		DPRINTF("Enqueuing request to writeback block 0x%lx\n", block->baseAddr);
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

	MemEvent *ev = new MemEvent(this, block->baseAddr, SupplyData);
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
	if (ev->getSize() == blocksize) {
		// Assumption:  if sizes are equal, base addresses are, too
		std::copy(ev->getPayload().begin(), ev->getPayload().end(), block->data.begin());
	} else {
		// Update a portion of the block
		Addr blockoffset = (ev->getAddr() <= block->baseAddr) ?
			0 :
			ev->getAddr() - block->baseAddr;

		Addr payloadoffset = (ev->getAddr() >= block->baseAddr) ?
			0 :
			block->baseAddr - ev->getAddr();

		for ( uint32_t i = 0 ; i < std::min(blocksize,ev->getSize()) ; i++ ) {
			assert(blockoffset+i < blocksize);
			block->data[blockoffset+i] = ev->getPayload()[payloadoffset+i];
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
		if ( b->isValid() && b->tag == tag ) {
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


Cache::LoadList_t::iterator Cache::findWaitingLoad(Addr addr, uint32_t size)
{
	uint32_t offset = 0;
	while ( offset < size ) {
		// Check potentialls multiple blocks

		Addr baddr = addrToBlockAddr(addr + offset);
		LoadList_t::iterator i = waitingLoads.begin();
		while ( i != waitingLoads.end() ) {
#if 0
			DPRINTF("Comparing [0x%lx]  to  [0x%lx] : %s\n",
					i->addr, addr + offset, (addrToBlockAddr(i->addr) == baddr) ?
					"TRUE" : "FALSE");
#endif
			if ( addrToBlockAddr(i->addr) == baddr ) {
#if 0
				DPRINTF("FOUND!\n");
#endif
				return i;
			}
			++i;
		}
		offset += blocksize;
	}

	return waitingLoads.end();
}


void Cache::printCache(void)
{
	static const char *status[] = {"I", "A", "S", "E"};
	std::ostringstream ss;

	ss << getName() << std::endl;
	ss << std::setfill('0');

	for ( int r = 0 ; r < n_rows; r++ ) {
		ss << std::setw(2) << r << " | ";
		CacheRow &row = database[r];
		for ( int c = 0 ; c < n_ways ; c++ ) {
			CacheBlock &b = row.blocks[c];
			ss << status[b.status] << " ";
			ss << "0x" << std::setw(4) << std::hex << b.baseAddr << std::dec << " ";
			ss << b.tag << " ";
			ss << "| ";
		}
		ss << std::endl;
	}

	ss << std::endl;
	ss << "Waiting Loads\n";
	for ( LoadList_t::iterator i = waitingLoads.begin() ; i != waitingLoads.end() ; ++i ) {
		Addr addr = i->addr;
		LoadInfo_t &li = *i;

		ss << "0x" << std::setw(4) << std::hex << addr << std:: dec;
		if ( li.targetBlock != NULL )
			ss << " slated for [" << li.targetBlock->row << ", " << li.targetBlock->col << "]";
		ss << "\n";

		for ( std::vector<LoadInfo_t::LoadElement_t>::iterator j = li.list.begin() ; j != li.list.end() ; ++j ) {
			MemEvent *ev = j->ev;
			SourceType_t src = j->src;
			SimTime_t elapsed = getCurrentSimTime() - j->issueTime;
			ss << "\t(" << ev->getID().first << ", " << ev->getID().second << ")  " << CommandString[ev->getCmd()] << "\t" << elapsed << "\n";
		}
	}

	std::cout << ss.str();

}
