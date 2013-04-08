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

#include <sstream>
#include <algorithm>
#include <iomanip>
#include <assert.h>

#include <sst_config.h>
#include <sst/core/serialization/element.h>
#include <sst/core/simulation.h>
#include <sst/core/element.h>
#include <sst/core/interfaces/memEvent.h>
#include <sst/core/interfaces/stringEvent.h>

#include "cache.h"


#define DPRINTF( fmt, args...) __DBG( DBG_CACHE, Cache, "%s: " fmt, getName().c_str(), ## args )

using namespace SST;
using namespace SST::MemHierarchy;
using namespace SST::Interfaces;

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
	for ( int r = 0 ; r < n_rows ; r++ ) {
		for ( int c = 0 ; c < n_ways ; c++ ) {
			database[r].blocks[c].row = r;
			database[r].blocks[c].col = c;
		}
	}


	num_read_hit = 0;
	num_read_miss = 0;
	num_supply_hit = 0;
	num_supply_miss = 0;
	num_write_hit = 0;
	num_write_miss = 0;
	num_upgrade_miss = 0;
}


void Cache::init(unsigned int phase)
{
	if ( !phase ) {
		for ( int i = 0 ; i < n_upstream ; i++ ) {
			upstream_links[i]->sendInitData(new StringEvent("SST::Interfaces::MemEvent"));
		}
		if ( directory_link ) directory_link->sendInitData(new StringEvent("SST::Interfaces::MemEvent"));
		if ( downstream_link ) downstream_link->sendInitData(new StringEvent("SST::Interfaces::MemEvent"));
		if ( snoop_link ) snoop_link->sendInitData(new StringEvent("SST::Interfaces::MemEvent"));
	}


	/* Cache should only initialize from upstream.  */
	for ( int i = 0 ; i < n_upstream ; i++ ) {
		SST::Event *ev = NULL;
		while ( (ev = upstream_links[i]->recvInitData()) != NULL ) {
			MemEvent *me = dynamic_cast<MemEvent*>(ev);
			if ( me ) {
				/* TODO:  Break down to cache-line size */
				// Pass down
				if ( downstream_link ) downstream_link->sendInitData(me);
				else if ( snoop_link ) snoop_link->sendInitData(me);
				else if ( directory_link ) directory_link->sendInitData(me);
				else delete me;
			} else {
				delete ev;
			}
		}
	}

	/* Eat anything coming over snoopy */
	SST::Event *ev = NULL;
	while ( (ev = snoop_link->recvInitData()) != NULL )
		delete ev;

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
    if ( DBG_CACHE & SST::_debug_flags )
        printCache();
	return 0;
}


void Cache::handleIncomingEvent(SST::Event *event, SourceType_t src)
{
	handleIncomingEvent(event, src, true);
}

void Cache::handleIncomingEvent(SST::Event *event, SourceType_t src, bool firstTimeProcessed)
{
	MemEvent *ev = static_cast<MemEvent*>(event);
    DPRINTF("Received Event %p (to %s) %s 0x%lx\n", ev, ev->getDst().c_str(), CommandString[ev->getCmd()], ev->getAddr());
    switch (ev->getCmd()) {
    case BusClearToSend:
        snoopBusQueue.clearToSend();
        delete ev;
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
        delete ev;
        break;

    default:
        /* Ignore */
        delete event;
        break;
    }
}



void Cache::handleSelfEvent(SST::Event *event)
{
	SelfEvent *ev = static_cast<SelfEvent*>(event);
    (this->*(ev->handler))(ev->event, ev->block, ev->event_source);
    delete ev;
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
			(block) ? ((isRead || block->status == CacheBlock::EXCLUSIVE) ? "HIT" : "UPGRADE") : "MISS",
			addrToBlockAddr(ev->getAddr()));
	if ( block ) {
		/* HIT */
		if ( isRead ) {
			if ( firstProcess ) num_read_hit++;
                if ( waitingForInvalidate(block) ) {
                    block->blockedEvents.push_back(ev);
                } else {
                    if ( ev->queryFlag(MemEvent::F_LOCKED) && (block->status != CacheBlock::EXCLUSIVE ) ) {
                        issueInvalidate(ev, block);
                    } else {
                        if ( ev->queryFlag(MemEvent::F_LOCKED) ) {
                            assert(!block->user_locked);
                            block->user_locked = true;
                            block->user_lock_needs_wb = false;
                        }
                        self_link->Send(1, new SelfEvent(&Cache::sendCPUResponse, makeCPUResponse(ev, block, UPSTREAM), block, UPSTREAM));
                        delete ev;
                    }
                }
        } else {
			if ( block->status == CacheBlock::EXCLUSIVE ) {
				if ( firstProcess ) num_write_hit++;
				updateBlock(ev, block);
				self_link->Send(1, new SelfEvent(&Cache::sendCPUResponse, makeCPUResponse(ev, block, UPSTREAM), block, UPSTREAM));
                if ( block->user_locked && ev->queryFlag(MemEvent::F_LOCKED) ) {
                    /* Unlock */
                    block->user_locked = false;
                    if ( block->user_lock_needs_wb ) {
                        writebackBlock(block, CacheBlock::SHARED);
                    }
                }
                delete ev;
			} else {
				if ( firstProcess ) num_upgrade_miss++;
                if ( waitingForInvalidate(block) ) {
                    block->blockedEvents.push_back(ev);
                } else {
                    issueInvalidate(ev, block);
                }
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


MemEvent* Cache::makeCPUResponse(MemEvent *ev, CacheBlock *block, SourceType_t src)
{
	Addr offset = ev->getAddr() - block->baseAddr;
	if ( offset+ev->getSize() > (Addr)blocksize ) {
		_abort(Cache, "Cache doesn't handle split rquests.\nReq for addr 0x%lx has offset of %lu, and size %u.  Blocksize is %u\n",
				ev->getAddr(), offset, ev->getSize(), blocksize);
	}

	MemEvent *resp = ev->makeResponse(this);
	if ( ev->getCmd() == ReadReq)
		resp->setPayload(ev->getSize(), &block->data[offset]);

	DPRINTF("Sending Response to CPU: (%lu, %d) in Response To (%lu, %d) [%s: 0x%lx] [...0x%02x%02x%02x%02x]\n",
			resp->getID().first, resp->getID().second,
			resp->getResponseToID().first, resp->getResponseToID().second,
			CommandString[resp->getCmd()], resp->getAddr(),
            resp->getPayload()[resp->getPayload().size()-4], resp->getPayload()[resp->getPayload().size()-3],
            resp->getPayload()[resp->getPayload().size()-2], resp->getPayload()[resp->getPayload().size()-1]);

	return resp;
}

void Cache::sendCPUResponse(MemEvent *ev, CacheBlock *block, SourceType_t src)
{
	/* CPU is always upstream link 0 */
	upstream_links[0]->Send(ev);

    /* release events pending on this row */
    CacheRow *row = findRow(ev->getAddr());
    handlePendingEvents(row, NULL);
}


void Cache::issueInvalidate(MemEvent *ev, CacheBlock *block)
{
	if ( snoop_link ) {
		BusHandlerArgs args;
		args.issueInvalidate.ev = ev;
		args.issueInvalidate.block = block;
		MemEvent *invEvent = new MemEvent(this, block->baseAddr, Invalidate);
		block->currentEvent = invEvent;
		DPRINTF("Enqueuing request to Invalidate block 0x%lx\n", block->baseAddr);
		snoopBusQueue.request( invEvent,
				new BusFinishHandler(&Cache::finishIssueInvalidateVA, args));
	} else {
		finishIssueInvalidate(ev, block);
	}
}

void Cache::finishIssueInvalidateVA(BusHandlerArgs &args)
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
    DPRINTF("Handling formerly blocked (initiating) event (%lu, %d) [%s: 0x%lx]\n",
            ev->getID().first, ev->getID().second,
            CommandString[ev->getCmd()], ev->getAddr());
	handleCPURequest(ev, false);
    while ( block->blockedEvents.size() > 0 ) {
        MemEvent *ev2 = block->blockedEvents.front();
        block->blockedEvents.pop_front();
        DPRINTF("Handling formerly blocked event (%lu, %d) [%s: 0x%lx]\n",
                ev2->getID().first, ev2->getID().second,
                CommandString[ev2->getCmd()], ev2->getAddr());
        handleCPURequest(ev2, false);
    }
}



/* Takes ownership of the event 'ev'. */
void Cache::loadBlock(MemEvent *ev, SourceType_t src)
{

	CacheBlock *block = NULL;
    Addr blockAddr = addrToBlockAddr(ev->getAddr());
	LoadList_t::iterator i = findWaitingLoad(blockAddr, blocksize);
	LoadInfo_t *li;
    bool reprocess = false;
	if ( i != waitingLoads.end() ) {
		li = &(i->second);
		assert (li->addr == blockAddr);

        /* Check to see if this a reprocess of the head event */
        if ( li->initiatingEvent != ev->getID() ) {
            DPRINTF("Adding to existing outstanding Load for this block.\n");
            li->list.push_back(LoadInfo_t::LoadElement_t(ev, src, getCurrentSimTime()));
            return;
        }
        DPRINTF("Reprocessing existing Load for this block.\n");
        reprocess = true;

    } else {
        DPRINTF("No existing Load for this block.  Creating.\n");
        std::pair<LoadList_t::iterator, bool> res = waitingLoads.insert(std::make_pair(blockAddr, LoadInfo_t(blockAddr)));
        li = &(res.first->second);
        li->initiatingEvent = ev->getID();
    }

    CacheRow *row = findRow(ev->getAddr());
    block = row->getLRU();

    if ( !block ) { // Row is full, wait for one to be available
        row->addWaitingEvent(ev, src);
        return;
    } else if ( block->status == CacheBlock::EXCLUSIVE ) {
        DPRINTF("Need to evict block 0x%lx to satisfy load for 0x%lx\n",
                block->baseAddr, ev->getAddr());

        writebackBlock(block, CacheBlock::INVALID); // We'll get it next time
        row->addWaitingEvent(ev, src);

        return;
    }

    /* Simple Load */
    block->activate(ev->getAddr());
    block->lock();

    li->targetBlock = block;
    if ( reprocess )
        li->list.push_front(LoadInfo_t::LoadElement_t(ev, src, getCurrentSimTime()));
    else
        li->list.push_back(LoadInfo_t::LoadElement_t(ev, src, getCurrentSimTime()));

    if ( snoop_link ) {
        MemEvent *req = new MemEvent(this, block->baseAddr, RequestData);
        req->setSize(blocksize);
        if ( next_level_name != NO_NEXT_LEVEL ) req->setDst(next_level_name);
        DPRINTF("Enqueuing request to load block 0x%lx\n", block->baseAddr);
        BusHandlerArgs args;
        args.loadBlock.loadInfo = li;
        snoopBusQueue.request( req, new BusFinishHandler(&Cache::finishLoadBlockBus, args));
        li->busEvent = req;
    }
    if ( downstream_link ) {
        downstream_link->Send(new MemEvent(this, block->baseAddr, RequestData));
    }
}


void Cache::finishLoadBlockBus(BusHandlerArgs &args)
{
	args.loadBlock.loadInfo->busEvent = NULL;
}



void Cache::handleCacheRequestEvent(MemEvent *ev, SourceType_t src, bool firstProcess)
{
	if ( src == SNOOP && ev->getSrc() == getName() ) {
        delete ev;
        return; // We sent it, ignore
    }

	CacheBlock *block = findBlock(ev->getAddr(), false);
	DPRINTF("0x%lx %s %s (block 0x%lx)%s\n", ev->getAddr(),
			(src == SNOOP && ev->getDst() != getName()) ? "SNOOP" : "",
			(block) ? "HIT" : "MISS",
			addrToBlockAddr(ev->getAddr()),
            (block && block->isLocked()) ? " LOCKED" : "");


    if ( src == SNOOP && ev->getSize() > blocksize ) {
        DPRINTF("SNOOP Request for block 0x%lx of size %d is larger than our blocksize (%d).  Ignoring.\n",
                addrToBlockAddr(ev->getAddr()), ev->getSize(), blocksize);
        delete ev;
        return;
    }

	if ( block ) {
        if ( block->isLocked() ) {
            self_link->Send(1, new SelfEvent(&Cache::retryEvent, ev, block, src));
            return;
        }
		if ( firstProcess ) num_supply_hit++;
		supplyMap_t::key_type supplyMapKey = std::make_pair(block->baseAddr, src);
		/* Hit */

		supplyMap_t::iterator supMapI = supplyInProgress.find(supplyMapKey);
		if ( supMapI != supplyInProgress.end() && !supMapI->second.canceled ) {
			// we're already working on this
			DPRINTF("Detected that we're already working on this\n");
            delete ev;
			return;
		}

		supplyInProgress[supplyMapKey] = SupplyInfo(NULL);
		block->lock();
		block->last_touched = getCurrentSimTime();
		self_link->Send(1, new SelfEvent(&Cache::supplyData, ev, block, src));
	} else {
		/* Miss */
		if ( src != SNOOP || ev->getDst() == getName() ) {
			if ( firstProcess) num_supply_miss++;
			loadBlock(ev, src);
		} else {
            /* Ignore  - Snoop request, or not to us */
            delete ev;
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
		block->unlock();
        delete ev;
		return;
	}


	MemEvent *resp = new MemEvent(this, block->baseAddr, SupplyData);
    if ( block && block->user_locked ) {
        block->user_lock_needs_wb = true;
        resp->setFlag(MemEvent::F_DELAYED);
    } else {
        resp->setPayload(block->data);
    }

	if ( src != SNOOP ) {
		SST::Link *link = getLink(src, ev->getLinkId());
		link->Send(resp);
		block->unlock();
		supplyInProgress.erase(supMapI);
	} else {
		BusHandlerArgs args;
		assert(block->isLocked());
		args.supplyData.block = block;
		args.supplyData.src = src;
		supMapI->second.busEvent = resp;
		DPRINTF("Enqueuing request to supply block 0x%lx\n", block->baseAddr);
		snoopBusQueue.request( resp,
				new BusFinishHandler(&Cache::finishBusSupplyData, args));
	}
	delete ev;
}


void Cache::finishBusSupplyData(BusHandlerArgs &args)
{
	DPRINTF("Supply Message sent for block 0x%lx\n", args.supplyData.block->baseAddr);
	args.supplyData.block->unlock();
	supplyMap_t::key_type supplyMapKey = std::make_pair(args.supplyData.block->baseAddr, args.supplyData.src);

	supplyMap_t::iterator supMapI = supplyInProgress.find(supplyMapKey);
	assert(supMapI != supplyInProgress.end());
	supplyInProgress.erase(supMapI);
}



void Cache::handleCacheSupplyEvent(MemEvent *ev, SourceType_t src)
{
	if ( src == SNOOP && ev->getSrc() == getName() ) {
        delete ev;
        return; // We sent it
    }

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
						DPRINTF("Canceling Bus Request for Supply on 0x%lx (%p)\n", supMapI->second.busEvent->getAddr(), supMapI->second.busEvent);
						BusHandlers handlers = snoopBusQueue.cancelRequest(supMapI->second.busEvent);
						if ( handlers.finish ) {
							handlers.finish->args.supplyData.block->unlock();
                            delete supMapI->second.busEvent;
							supMapI->second.busEvent = NULL;
							delete handlers.finish;
						}
                        if ( handlers.init ) delete handlers.init;
					}
				}
				blkAddr += blocksize;
			}
		}
	}

    /* If we're trying to load this, but this is locked elsewhere, we need to wait longer for the data. */
    if ( ev->queryFlag(MemEvent::F_DELAYED) ) {
        delete ev;
        return;
    }

	/* Check to see if we're trying to load this data */
	LoadList_t::iterator i = findWaitingLoad(ev->getAddr(), ev->getSize());
	if ( i != waitingLoads.end() ) {
		while ( i != waitingLoads.end() ) {
			LoadInfo_t &li = i->second;
			if ( li.busEvent ) {
				DPRINTF("Canceling Bus Request for Load on 0x%lx\n", li.busEvent->getAddr());
				BusHandlers handlers = snoopBusQueue.cancelRequest(li.busEvent);
                if ( handlers.init ) delete handlers.init;
                if ( handlers.finish ) delete handlers.finish;
                delete li.busEvent;
			}

			if ( !li.targetBlock ) {
				/* We still don't have a block assigned, so we didn't ask for
				 * this.  Must be a snoop that we can ignore.
				 * (no room in the inn) */
				assert( src == SNOOP );
				break;
			}

			if ( ev->getSize() < blocksize ) {
				// This isn't enough for us, but may satisfy others
				DPRINTF("Not enough info in block (%u vs %u blocksize)\n",
						ev->getSize(), blocksize);
				li.targetBlock->status = CacheBlock::INVALID;
				li.targetBlock->unlock();

				// Delete events that we would be reprocessing
				for ( uint32_t n = 0 ; n < li.list.size() ; n++ ) {
					LoadInfo_t::LoadElement_t &oldEV = li.list[n];
					if ( src == SNOOP && oldEV.src == SNOOP ) {
						delete oldEV.ev;
					}
				}

			} else {
				updateBlock(ev, li.targetBlock);
				li.targetBlock->status = CacheBlock::SHARED;
				li.targetBlock->unlock();

				for ( uint32_t n = 0 ; n < li.list.size() ; n++ ) {
					LoadInfo_t::LoadElement_t &oldEV = li.list[n];
					/* If this was from the Snoop Bus, and we've got other cache's asking
					 * for this data over the snoop bus, we can assume they saw it, and
					 * we don't need to reprocess them.
					 */
					if ( src == SNOOP && oldEV.src == SNOOP ) {
						delete oldEV.ev;
                    } else {
                        /* Make them be processed in order, so pass 'n' as a delay */
                        handleIncomingEvent(oldEV.ev, oldEV.src, false);
                        //self_link->Send(n, new SelfEvent(&Cache::finishSupplyEvent, oldEV.ev, li.targetBlock, oldEV.src));
                    }
				}
			}
            handlePendingEvents(findRow(li.targetBlock->baseAddr), li.targetBlock);
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
    delete ev;
}


void Cache::finishSupplyEvent(MemEvent *origEV, CacheBlock *block, SourceType_t origSrc)
{
    DPRINTF("\n");
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

	if ( block->status == CacheBlock::SHARED ) {
		DPRINTF("Invalidating block 0x%lx\n", block->baseAddr);
		block->status = CacheBlock::INVALID;
        handlePendingEvents(findRow(block->baseAddr), NULL);
	}
	if ( block->status == CacheBlock::EXCLUSIVE ) {
		DPRINTF("Invalidating EXCLUSIVE block 0x%lx\n", block->baseAddr);
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
	BusHandlers handler = snoopBusQueue.cancelRequest(block->currentEvent);
    delete block->currentEvent;
	block->currentEvent = NULL;
	handleCPURequest(handler.finish->args.issueInvalidate.ev, false);
	if ( handler.init ) delete handler.init;
	if ( handler.finish ) delete handler.finish;
}


void Cache::writebackBlock(CacheBlock *block, CacheBlock::BlockStatus newStatus)
{
    if ( block->wb_in_progress ) {
		DPRINTF("Writeback already in progress for block 0x%lx\n", block->baseAddr);
        return;
    }
    block->wb_in_progress = true;
	if ( snoop_link ) {
		block->lock();
		BusHandlerArgs args;
		args.writebackBlock.block = block;
		args.writebackBlock.newStatus = newStatus;
		args.writebackBlock.decrementLock = true;
		DPRINTF("Enqueuing request to writeback block 0x%lx\n", block->baseAddr);

        MemEvent *ev = new MemEvent(this, block->baseAddr, SupplyData);
        ev->setFlag(MemEvent::F_WRITEBACK);
        ev->setPayload(block->data);
		snoopBusQueue.request(ev,
				new BusFinishHandler(&Cache::finishWritebackBlockVA, args),
                new BusInitHandler(&Cache::prepWritebackBlock, args));
	} else {
		finishWritebackBlock(block, newStatus, false);
	}
}


void Cache::prepWritebackBlock(BusHandlerArgs &args, MemEvent *ev)
{
    ev->setPayload(args.writebackBlock.block->data);
}


void Cache::finishWritebackBlockVA(BusHandlerArgs& args)
{
	CacheBlock *block = args.writebackBlock.block;
	CacheBlock::BlockStatus newStatus = args.writebackBlock.newStatus;
	bool decrementLock = args.writebackBlock.decrementLock;
	finishWritebackBlock(block, newStatus, decrementLock);
}

void Cache::finishWritebackBlock(CacheBlock *block, CacheBlock::BlockStatus newStatus, bool decrementLock)
{

    block->wb_in_progress = false;
	if ( decrementLock ) /* AKA, sent on the Snoop Bus */
		block->unlock();

	if ( downstream_link ) {
        MemEvent *ev = new MemEvent(this, block->baseAddr, SupplyData);
        ev->setFlag(MemEvent::F_WRITEBACK);
        ev->setPayload(block->data);
		downstream_link->Send(ev);
    }
	if ( directory_link ) {
        MemEvent *ev = new MemEvent(this, block->baseAddr, SupplyData);
        ev->setFlag(MemEvent::F_WRITEBACK);
        ev->setPayload(block->data);
		directory_link->Send(ev);
    }


    DPRINTF("Wrote Back Block 0x%lx\n", block->baseAddr);

	assert(!block->isLocked());
	block->status = newStatus;
    CacheRow *row = findRow(block->baseAddr);
    if ( newStatus == CacheBlock::INVALID ) block = NULL;
    handlePendingEvents(row, block);
}



/**** Utility Functions ****/

void Cache::handlePendingEvents(CacheRow *row, CacheBlock *block)
{
    if ( row->waitingEvents.size() > 0 ) {

        std::map<Addr, CacheRow::eventQueue_t>::iterator q;
        if ( block ) {
            q = row->waitingEvents.find(block->baseAddr);
        } else {
            /* Pick one. */
            q = row->waitingEvents.begin();
        }
        if ( q != row->waitingEvents.end() ) {
            CacheRow::eventQueue_t &queue = q->second;
            while ( queue.size() ) {
                std::pair<MemEvent*, SourceType_t> ev = queue.front();
                queue.pop_front();
                DPRINTF("Issuing Retry for event (%lu, %d) %s [0x%lx]\n",
                        ev.first->getID().first, ev.first->getID().second, CommandString[ev.first->getCmd()], ev.first->getAddr());
                self_link->Send(1, new SelfEvent(&Cache::retryEvent, ev.first, NULL, ev.second));
            }
            row->waitingEvents.erase(q);
        }
    }

}


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
#if 0
    char buffer[2*64 + 8] = {0};
    char *b = buffer;
    b += sprintf(b, " [0x");
    for ( uint32_t i = 0 ; i < blocksize ; i++ ) {
        b += sprintf(b, "%02x", block->data[i]);
    }
    b += sprintf(b, "]");
    DPRINTF("Updating block 0x%lx %s\n", block->baseAddr, buffer);
#else
    DPRINTF("Updating block 0x%lx\n", block->baseAddr);
#endif
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
#if 0
		LoadList_t::iterator i = waitingLoads.begin();
		while ( i != waitingLoads.end() ) {
			if ( i->addr == baddr ) {
				return i;
			}
			++i;
		}
#else
        LoadList_t::iterator i = waitingLoads.find(baddr);
        if ( i != waitingLoads.end() )
            return i;
#endif
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
			ss << "0x" << std::setw(8) << std::hex << b.baseAddr << std::dec << " ";
			ss << std::setw(4) << b.tag << " ";
			ss << "| ";
		}
		ss << std::endl;
	}

	ss << std::endl;
    if ( waitingLoads.size() > 0 ) {
        ss << "Waiting Loads\n";
        for ( LoadList_t::iterator i = waitingLoads.begin() ; i != waitingLoads.end() ; ++i ) {
            Addr addr = i->first;
            LoadInfo_t &li = i->second;

            ss << "0x" << std::setw(4) << std::hex << addr << std::dec;
            if ( li.targetBlock != NULL )
                ss << " slated for [" << li.targetBlock->row << ", " << li.targetBlock->col << "]";
            ss << "\n";

            for ( std::deque<LoadInfo_t::LoadElement_t>::iterator j = li.list.begin() ; j != li.list.end() ; ++j ) {
                MemEvent *ev = j->ev;
                SimTime_t elapsed = getCurrentSimTime() - j->issueTime;
                ss << "\t(" << ev->getID().first << ", " << ev->getID().second << ")  " << CommandString[ev->getCmd()] << "\t" << elapsed << "\n";
            }
        }
    }

    size_t num_pend = 0;
    for ( int r = 0 ; r < n_rows ; r++ ) {
        CacheRow *row = &database[r];
        num_pend += row->waitingEvents.size();
    }
    if ( num_pend > 0 ) {
        ss << "Pending Events\t" << num_pend << std::endl;
        for ( int r = 0 ; r < n_rows ; r++ ) {
            CacheRow *row = &database[r];
            if ( row->waitingEvents.size() > 0 ) {
                ss << "Row " << r << "\n";
#if 0
                for ( std::deque<std::pair<MemEvent*, SourceType_t> >::iterator i = row->waitingEvents.begin() ; i != row->waitingEvents.end() ; ++i ) {
                    MemEvent *ev = i->first;
                    ss << "\tEvent id (" <<  ev->getID().first << ", " << ev->getID().second << ") Command:  " << CommandString[ev->getCmd()] << "  0x" << std::hex << ev->getAddr() << std::dec << "\n";
                }
#else
                for ( std::map<Addr, CacheRow::eventQueue_t>::iterator i = row->waitingEvents.begin () ; i != row->waitingEvents.end() ; ++i ) {
                    ss << "\tBlock Address    0x" << std::hex << i->first << std::dec << "\n";
                    for ( CacheRow::eventQueue_t::iterator j = i->second.begin() ; j != (i->second.end()) ; ++j ) {
                        MemEvent *ev = j->first;
                        ss << "\t\tEvent id (" <<  ev->getID().first << ", " << ev->getID().second << ") Command:  " << CommandString[ev->getCmd()] << "  0x" << std::hex << ev->getAddr() << std::dec << "\n";
                    }
                }
#endif
            }
        }
    }

    if ( snoopBusQueue.size() ) {
        ss << "Bus Queue Size:  " << snoopBusQueue.size() << "\n";
    }

	std::cout << ss.str();

}
