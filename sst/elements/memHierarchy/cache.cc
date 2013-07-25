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
#include <string>
#include <algorithm>
#include <iomanip>
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

#include "cache.h"
#include "cacheListener.h"
#include "bus.h"



using namespace SST;
using namespace SST::MemHierarchy;
using namespace SST::Interfaces;

static const std::string NO_NEXT_LEVEL = "NONE";

Cache::Cache(ComponentId_t id, Params_t& params) :
	Component(id)
{
    dbg.init("@t:Cache::@p():@l " + getName() + ": ", 0, 0, (Output::output_location_t)params.find_integer("debug", 0));
    statsOutputTarget = (Output::output_location_t)params.find_integer("printStats", 0);

	// get parameters
	n_ways = params.find_integer("num_ways", 0);
	n_rows = params.find_integer("num_rows", 0);
	blocksize = params.find_integer("blocksize", 0);
	if ( 0 == n_ways || 0 == n_rows || 0 == blocksize ) {
		_abort(Cache, "# Ways, # Rows and Blocksize must all be >0\n");
	}

    std::string cacheType = params.find_string("mode", "STANDARD");
    std::transform(cacheType.begin(), cacheType.end(), cacheType.begin(), ::toupper);
    if ( cacheType == "INCLUSIVE" ) {
        cacheMode = INCLUSIVE;
    } else if ( cacheType == "EXCLUSIVE" ) {
        _abort(Cache, "Cache mode EXCLUSIVE not yet implemented.\n");
        cacheMode = EXCLUSIVE;
    } else if ( cacheType == "STANDARD" ) {
        cacheMode = STANDARD;
    } else
        _abort(Cache, "Cache 'mode' must be one of 'INCLUSIVE', 'EXCLUSIVE' or 'STANDARD' (default)\n");


	//  TODO:  Is this right?
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
            dbg.output(CALL_INFO, "upstream_links[%d]->getId() = %ld\n", i, upstream_links[i]->getId());
		}
	}

	next_level_name = params.find_string("next_level", NO_NEXT_LEVEL);
	downstream_link = configureLink( "downstream",
			new Event::Handler<Cache, SourceType_t>(this,
				&Cache::handleIncomingEvent, DOWNSTREAM) );
    if ( downstream_link )
        dbg.output(CALL_INFO, "Downstream Link id = %ld\n", downstream_link->getId());
	snoop_link = configureLink( "snoop_link", "50 ps",
			new Event::Handler<Cache>(this, &Cache::handleBusEvent) );
	if ( NULL != snoop_link ) { // Snoop is a bus.
		snoopBusQueue.setup(this, snoop_link);
        dbg.output(CALL_INFO, "SNOOP Link id = %ld\n", snoop_link->getId());
	}

    /* TODO:  Check to see if 'directory_link' exists */
    if ( isPortConnected("directory_link") ) {
        MemNIC::ComponentInfo myInfo;
        myInfo.link_port = "directory_link";
        myInfo.link_bandwidth = "2 ns"; // Time base as registered earlier
        myInfo.name = getName();
        myInfo.network_addr = params.find_integer("net_addr");
        myInfo.type = MemNIC::TypeCache;
        myInfo.typeInfo.cache.blocksize = blocksize;
        myInfo.typeInfo.cache.num_blocks = n_ways * n_rows;

        directory_link = new MemNIC(this, myInfo,
                new Event::Handler<Cache, SourceType_t>(this,
                    &Cache::handleIncomingEvent, DIRECTORY) );
    } else {
        directory_link = NULL;
    }

	self_link = configureSelfLink("Self", params.find_string("access_time", ""),
				new Event::Handler<Cache>(this, &Cache::handleSelfEvent));

	rowshift = numBits(blocksize);
	rowmask = n_rows - 1;  // Assumption => n_rows is power of 2
    if ( 0 != (n_rows & (n_rows - 1)) ) {
        _abort(Cache, "Cache:%s:  Parameter num_rows must be a power of two.\n", getName().c_str());
    }
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
    num_invalidates = 0;

    maxResponseTimeAllowed = params.find_integer("maxL1ResponseTime", 0);
    registerClock("1 GHz", new Clock::Handler<Cache>(this, &Cache::clockTick));

    std::string prefetcher = params.find_string("prefetcher");
    if ( prefetcher == "" ) {
        listener = new CacheListener();
    } else {
        listener = dynamic_cast<CacheListener*>(loadModule(prefetcher, params));

        if(NULL == listener) {
            _abort(Cache, "Prefetcher could not be loaded.\n");
        }
    }

    listener->setOwningComponent(this);
    listener->registerResponseCallback(new Event::Handler<Cache>(this, &Cache::handlePrefetchEvent));

    /* L1 status will be detected by seeing CPU requests come in. */
    isL1 = false;
}


bool Cache::clockTick(Cycle_t) {
    if ( directory_link ) {
        directory_link->clock();
    }

    if ( 0 < maxResponseTimeAllowed && !responseTimes.empty() ) {
        SimTime_t now = getCurrentSimTimeNano();
        for ( std::map<MemEvent::id_type, SimTime_t>::iterator i = responseTimes.begin() ; i != responseTimes.end() ; ++i ) {
            if ( (now - i->second) > maxResponseTimeAllowed ) {
                _abort(Cache, "CPU Request (%"PRIu64", %d) has taken %"PRIu64" ns, which is greater than allowed (%"PRIu64")\n",
                        i->first.first, i->first.second, (now - i->second), maxResponseTimeAllowed);
            }
        }
    }

    return false;
}


void Cache::init(unsigned int phase)
{
    if ( directory_link ) directory_link->init(phase);
	if ( !phase ) {
		for ( int i = 0 ; i < n_upstream ; i++ ) {
			upstream_links[i]->sendInitData(new StringEvent("SST::Interfaces::MemEvent"));
		}
		if ( downstream_link ) downstream_link->sendInitData(new StringEvent("SST::Interfaces::MemEvent"));
		if ( snoop_link ) snoop_link->sendInitData(new StringEvent("SST::Interfaces::MemEvent"));
	}


	/* Cache should only initialize from upstream.  */
	for ( int i = 0 ; i < n_upstream ; i++ ) {
		SST::Event *ev = NULL;
		while ( NULL != (ev = upstream_links[i]->recvInitData()) ) {
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

	/* pass downstream (if possible ) anything coming over snoopy */
    if ( snoop_link ) {
        SST::Event *ev = NULL;
        while ( NULL != (ev = snoop_link->recvInitData()) ) {
			MemEvent *me = dynamic_cast<MemEvent*>(ev);
			StringEvent *se = dynamic_cast<StringEvent*>(ev);
			if ( me ) {
				if ( directory_link ) directory_link->sendInitData(me);
                else if ( downstream_link ) downstream_link->sendInitData(me);
                else delete me;
            } else if ( se ) {
                if ( std::string::npos != se->getString().find(Bus::BUS_INFO_STR) ) {
                    snoopBusQueue.init(se->getString());
                }
                delete se;
            } else {
                delete ev;
            }
        }
    }

}


void Cache::setup(void)
{
    if ( directory_link ) {
        directory_link->setup();
        const std::vector<MemNIC::ComponentInfo> &peers = directory_link->getPeerInfo();
        for ( std::vector<MemNIC::ComponentInfo>::const_iterator i = peers.begin() ; i != peers.end() ; ++i ) {
            if ( i->type == MemNIC::TypeDirectoryCtrl ) {
                /* Record directory controller info */
                directories.push_back(*i);
            }
        }
        // Save some memory
        directory_link->clearPeerInfo();
    }
}


void Cache::finish(void)
{
    if ( directory_link ) {
        directory_link->finish();
    }

    Output out("", 0, 0, statsOutputTarget);
	out.output("Cache %s stats:\n"
			"\t# Read    Hits:      %"PRIu64"\n"
			"\t# Read    Misses:    %"PRIu64"\n"
			"\t# Supply  Hits:      %"PRIu64"\n"
			"\t# Supply  Misses:    %"PRIu64"\n"
			"\t# Write   Hits:      %"PRIu64"\n"
			"\t# Write   Misses:    %"PRIu64"\n"
			"\t# Upgrade Misses:    %"PRIu64"\n"
			"\t# Invalidations:     %"PRIu64"\n",
			getName().c_str(),
			num_read_hit, num_read_miss,
			num_supply_hit, num_supply_miss,
			num_write_hit, num_write_miss,
			num_upgrade_miss,
            num_invalidates);
    if ( latStats.getNumPkts() > 0 ) {
        out.output( "\tAvg Response Time:  %lg ns\n"
                "\tMin Response Time:      %"PRIu64" ns\n"
                "\tMax Response Time:      %"PRIu64" ns\n"
                "\tResponse Time StdDev:   %lg\n",
                latStats.getMeanLatency(),
                latStats.getMinLatency(),
                latStats.getMaxLatency(),
                latStats.getStdDevLatency());
    }

    listener->printStats(out);

    if ( DBG_CACHE & SST::_debug_flags )
        printCache(out);
}


void Cache::printStatus(Output &out)
{
    out.output("MemHierarchy::Cache %s\n", getName().c_str());
    out.output("\t# Accesses:  %"PRIu64"\n",
            num_read_hit + num_read_miss + num_write_hit + num_write_miss + num_supply_hit + num_supply_miss);
    out.output("\t# Invalidations:  %"PRIu64"\n", num_invalidates);
    if ( isL1 )
        out.output("\tMax Response Latency:  %"PRIu64"\n", latStats.getMaxLatency());

    out.output("\tInvalidations in progress:  %zu\n", invalidations.size());
    for ( std::map<Addr, Invalidation>::iterator i = invalidations.begin() ; i != invalidations.end() ; ++i ) {
        out.output("\t\t0x%"PRIx64":\n", i->first);
        Invalidation &inv = i->second;
        out.output("\t\t\tIssued via (%"PRIu64", %d)\n", inv.issuingEvent.first, inv.issuingEvent.second);
        out.output("\t\t\tBus Event: %p\n", inv.busEvent);
        out.output("\t\t\tTarget Block: %p\n", inv.block);
        out.output("\t\t\tIs Cancelable: %s\n", inv.canCancel ? "Yes" : "No");
        out.output("\t\t\t# Waiting Events: %zu\n", inv.waitingEvents.size());
        for ( std::deque<std::pair<MemEvent*, SourceType_t> >::iterator j = inv.waitingEvents.begin() ; j != inv.waitingEvents.end() ; ++j ) {
            out.output("\t\t\t\t(%"PRIu64", %d)\n", j->first->getID().first, j->first->getID().second);
        }
    }

    out.output("\tWaiting Loads:  %zu\n", waitingLoads.size());
    for ( LoadList_t::iterator i = waitingLoads.begin() ; i != waitingLoads.end() ; ++i ) {
        out.output("\t\tAddr:\t0x%"PRIx64"\n", i->first);
        out.output("\t\tWaiting: %zu\n", i->second->list.size());
        for ( std::deque<LoadInfo_t::LoadElement_t>::iterator j = i->second->list.begin() ; j != i->second->list.end() ; ++j ) {
            out.output("\t\t\t(%"PRIu64", %d)\t\t[%"PRIu64" ns]\n", j->ev->getID().first, j->ev->getID().second, getCurrentSimTimeNano() - j->issueTime);
        }
    }

    out.output("\tSupplies in Progress:  %zu\n", suppliesInProgress.size());
    for ( supplyMap_t::iterator i = suppliesInProgress.begin() ; i != suppliesInProgress.end() ; ++i ) {
        out.output("\t\t0x%"PRIx64"  (Caused by: (%"PRIu64", %d), busEvent: (%"PRIu64", %d), status: %s\n",
                i->first.first,
                i->second.initiatingEvent->getID().first, i->second.initiatingEvent->getID().second,
                (NULL == i->second.busEvent) ? 0 : i->second.busEvent->getID().first,
                (NULL == i->second.busEvent) ? -1 : i->second.busEvent->getID().second,
                i->second.canceled ? "CANCELED" : "In Progress");
    }

    out.output("\tOutstanding Writes:  %zu\n", outstandingWrites.size());
    for ( std::map<MemEvent::id_type, std::pair<MemEvent*, SourceType_t> >::iterator i = outstandingWrites.begin() ; i != outstandingWrites.end() ; ++i ) {
        out.output("\t\t0x%"PRIx64"\t caused by (%"PRIu64", %d).  Write Event: (%"PRIu64", %d)\n",
                i->second.first->getAddr(),
                i->second.first->getID().first, i->second.first->getID().second,
                i->first.first, i->first.second);
    }

    if ( NULL != snoop_link ) {
        snoopBusQueue.printStatus(out);
    }

}


void Cache::handleBusEvent(SST::Event *event)
{
    BusEvent *be = static_cast<BusEvent*>(event);
    switch ( be->getCmd() ) {
    case BusEvent::SendData:
        handleIncomingEvent(be->getPayload(), SNOOP);
        break;
    case BusEvent::ClearToSend:
        snoopBusQueue.clearToSend(be);
        break;
    default:
        _abort(Cache, "Bus Client shouldn't be receiving this event (%d).\n", be->getCmd());
    }

    delete be;
}


void Cache::handleIncomingEvent(SST::Event *event, SourceType_t src)
{
	handleIncomingEvent(event, src, true);
}

void Cache::handleIncomingEvent(SST::Event *event, SourceType_t src, bool firstTimeProcessed, bool firstPhaseComplete)
{
	MemEvent *ev = static_cast<MemEvent*>(event);
    dbg.output(CALL_INFO, "Received Event on %d %p (%"PRIu64", %d) (%s to %s) %s 0x%"PRIx64" [Size: %u]\n",
            src, ev,
            ev->getID().first, ev->getID().second,
            ev->getSrc().c_str(), ev->getDst().c_str(),
            CommandString[ev->getCmd()], ev->getAddr(), ev->getSize());
    switch (ev->getCmd()) {
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
        handleInvalidate(ev, src, firstPhaseComplete);
        break;

    case ACK:
        if ( SNOOP != src || ev->getDst() == getName() ) {
            ackInvalidate(ev);
        } else {
            delete event;
        }
        break;

    case NACK:
        handleNACK(ev, src);
        break;

    case Fetch:
        handleFetch(ev, false, firstPhaseComplete);
        break;
    case FetchInvalidate:
        handleFetch(ev, true, firstPhaseComplete);
        break;

    case WriteResp:
        handleWriteResp(ev, src);
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
    ev->fire();
    delete ev;
}

void Cache::retryEvent(MemEvent *ev, CacheBlock *block, SourceType_t src)
{
	handleIncomingEvent(ev, src, false);
}


void Cache::handlePrefetchEvent(SST::Event *event)
{
    dbg.output(CALL_INFO, "Incoming PREFETCHER Event!\n");
    self_link->send(1, new SelfEvent(this, &Cache::retryEvent, static_cast<MemEvent*>(event), NULL, PREFETCHER));
}



void Cache::handleCPURequest(MemEvent *ev, bool firstProcess)
{
    isL1 = true;
	assert(ev->getCmd() == ReadReq || ev->getCmd() == WriteReq);
	bool isRead = (ev->getCmd() == ReadReq);
	CacheBlock *block = findBlock(ev->getAddr(), false);
	dbg.output(CALL_INFO, "(%"PRIu64", %d) 0x%"PRIx64"%s %s%s %s (block 0x%"PRIx64" [%d])\n",
			ev->getID().first, ev->getID().second,
			ev->getAddr(),
            ev->queryFlag(MemEvent::F_LOCKED) ? " [LOCKED]" : "",
            ev->queryFlag(MemEvent::F_UNCACHED) ? "Uncached " : "",
			isRead ? "READ" : "WRITE",
			(block) ? ((isRead || CacheBlock::EXCLUSIVE == block->status) ? "HIT" : "UPGRADE") : "MISS",
			addrToBlockAddr(ev->getAddr()),
            block ? block->status : -1
            );

    if ( firstProcess ) registerNewCPURequest(ev->getID());

    if ( ev->queryFlag(MemEvent::F_UNCACHED) ) {
        if ( isRead ) loadBlock(ev, UPSTREAM);
        else handleUncachedWrite(ev, UPSTREAM);
        return;
    }

    if ( firstProcess )
        listener->notifyAccess(isRead ? CacheListener::READ : CacheListener::WRITE,
                (NULL != block) ? CacheListener::HIT : CacheListener::MISS,
                ev->getAddr());

    if ( ev->queryFlag(MemEvent::F_LOCKED) && !isRead ) assert(CacheBlock::EXCLUSIVE == block->status);
    if ( block ) {
        /* HIT */
        if ( isRead ) {
            if ( firstProcess ) num_read_hit++;
            if ( waitingForInvalidate(block->baseAddr) ) {
                dbg.output(CALL_INFO, "Invalidation for this in progress.  Putting into queue.\n");
                invalidations[block->baseAddr].waitingEvents.push_back(std::make_pair(ev, UPSTREAM));
            } else {
                if ( ev->queryFlag(MemEvent::F_LOCKED) && (CacheBlock::EXCLUSIVE != block->status) ) {
                    issueInvalidate(ev, UPSTREAM, block, CacheBlock::EXCLUSIVE, SEND_BOTH);
                } else {
                    if ( ev->queryFlag(MemEvent::F_LOCKED) ) {
                        if ( block->wb_in_progress || ( supplyInProgress(block->baseAddr, SNOOP)) ) {
                            /* We still have this in exclusive, but a writeback is in progress.
                             * this will take us out of exclusive.  Let's punt and retry later.
                             */
                            dbg.output(CALL_INFO, "There's a WB (%d) or a Supply in progress.  Retry this locked event later.\n", block->wb_in_progress);
                            self_link->send(1, new SelfEvent(this, &Cache::retryEvent, ev, block, UPSTREAM));
                            return;
                        }
                        /* TODO!   We lock on (bytes/words), but the line is locked! */
                        block->user_locked++;
                        block->user_lock_needs_wb = false;
                    }
                    self_link->send(1, new SelfEvent(this, &Cache::sendCPUResponse, makeCPUResponse(ev, block, UPSTREAM), block, UPSTREAM));
                    delete ev;
                }
            }
        } else {
            if ( CacheBlock::EXCLUSIVE == block->status ) {
                if ( firstProcess ) num_write_hit++;
                updateBlock(ev, block);
                self_link->send(1, new SelfEvent(this, &Cache::sendCPUResponse, makeCPUResponse(ev, block, UPSTREAM), block, UPSTREAM));
                if ( block->user_locked && ev->queryFlag(MemEvent::F_LOCKED) ) {
                    /* Unlock */
                    assert(block->user_locked);
                    block->user_locked--;
                    /* If we're no longer locked, and we've tagged that we need
                     * to send a writeback, do so, unless we haven't yet sent
                     * the 'SENT_DELAYED' flag.  If the F_DELAYED message is
                     * still pending, it will get promoted to a full SupplyData
                     * when it sends.
                     */
                    if ( 0 == block->user_locked && block->user_lock_needs_wb && block->user_lock_sent_delayed ) {
                        writebackBlock(block, CacheBlock::SHARED);
                    }
                }
                delete ev;
            } else {
                if ( firstProcess ) num_upgrade_miss++;
                if ( waitingForInvalidate(block->baseAddr) ) {
                    dbg.output(CALL_INFO, "Invalidation for this in progress.  Putting into queue.\n");
                    invalidations[block->baseAddr].waitingEvents.push_back(std::make_pair(ev, UPSTREAM));
                } else {
                    issueInvalidate(ev, UPSTREAM, block, CacheBlock::EXCLUSIVE, SEND_BOTH);
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

// #define CACHE_PRINT_DATA
#ifdef CACHE_PRINT_DATA
static const char* printData(MemEvent *ev) {
    static char buffer[1024] = {0};
    memset(buffer, '\0', sizeof(buffer));
    char *p = buffer;

    p += sprintf(p, "0x");

    size_t nb = ev->getSize();
    std::vector<uint8_t> &data = ev->getPayload();
    for ( size_t i = 0 ; i < nb ; i++ ) {
        p += sprintf(p, "%02x", data[i]);
    }

    return buffer;
}
#endif

MemEvent* Cache::makeCPUResponse(MemEvent *ev, CacheBlock *block, SourceType_t src)
{
	Addr offset = ev->getAddr() - block->baseAddr;
	if ( offset+ev->getSize() > (Addr)blocksize ) {
		_abort(Cache, "Cache doesn't handle split rquests.\nReq for addr 0x%"PRIx64" has offset of %"PRIu64", and size %u.  Blocksize is %u\n",
				ev->getAddr(), offset, ev->getSize(), blocksize);
	}

	MemEvent *resp = ev->makeResponse(this);
	if ( ev->getCmd() == ReadReq)
		resp->setPayload(ev->getSize(), &block->data[offset]);

	dbg.output(CALL_INFO, "Creating Response to CPU: (%"PRIu64", %d) in Response To (%"PRIu64", %d) [%s: 0x%"PRIx64"] [%s]\n",
			resp->getID().first, resp->getID().second,
			resp->getResponseToID().first, resp->getResponseToID().second,
			CommandString[resp->getCmd()], resp->getAddr(),
#ifdef CACHE_PRINT_DATA
            printData((ev->getCmd() == ReadReq) ? resp : ev)
#else
            "DATA HIDDEN"
#endif
           );

	return resp;
}

void Cache::sendCPUResponse(MemEvent *ev, CacheBlock *block, SourceType_t src)
{
    dbg.output(CALL_INFO, "Sending CPU Response %s 0x%"PRIx64"  (%"PRIu64", %d)\n",
            CommandString[ev->getCmd()], ev->getAddr(),
            ev->getID().first, ev->getID().second);

    clearCPURequest(ev->getResponseToID());

	/* CPU is always upstream link 0 */
	upstream_links[0]->send(ev);

    /* release events pending on this row */
    CacheRow *row = findRow(ev->getAddr());
    handlePendingEvents(row, NULL);
}


void Cache::issueInvalidate(MemEvent *ev, SourceType_t src, CacheBlock *block, CacheBlock::BlockStatus newStatus, ForwardDir_t direction, bool cancelable)
{
    block->lock();
    invalidations[block->baseAddr].block = block;
    invalidations[block->baseAddr].newStatus = newStatus;
    issueInvalidate(ev, src, block->baseAddr, direction, cancelable);
}


void Cache::issueInvalidate(MemEvent *ev, SourceType_t src, Addr addr, ForwardDir_t direction, bool cancelable)
{

    Invalidation &inv = invalidations[addr];
    assert(0 == inv.waitingEvents.size());
    inv.waitingEvents.push_back(std::make_pair(ev, src));
    inv.waitingACKs = 0;
    inv.canCancel = cancelable;

    MemEvent *invalidateEvent = new MemEvent(this, addr, Invalidate);
    inv.issuingEvent = invalidateEvent->getID();


    dbg.output(CALL_INFO, "Enqueuing request to Invalidate block 0x%"PRIx64"  [Inv Event: (%"PRIu64", %d)]\n", addr, invalidateEvent->getID().first, invalidateEvent->getID().second);

	if ( ((ev->getAddr() != addr) || (SNOOP != src)) && snoop_link ) {
		MemEvent *invEvent = new MemEvent(invalidateEvent);
        BusHandlerArgs args;
        args.invalidation.inv = &invalidations[addr];
		snoopBusQueue.request(invEvent, BusHandlers(NULL, &Cache::finishBusSendInvalidation, args));
        inv.waitingACKs += snoopBusQueue.getNumPeers();
        inv.busEvent = invEvent;
	}

    if ( direction == SEND_DOWN || direction == SEND_BOTH ) {
        if ( downstream_link && NO_NEXT_LEVEL != next_level_name ) {
            downstream_link->send(new MemEvent(invalidateEvent));
            inv.waitingACKs++;
        }
        if ( directory_link ) {
            MemEvent *dirInvEvent = new MemEvent(invalidateEvent);
            dirInvEvent->setDst(findTargetDirectory(addr));
            directory_link->send(dirInvEvent);
            inv.waitingACKs++;
        }
    }

    if ( direction == SEND_UP || direction == SEND_BOTH ) {
        for ( int i = 0 ; i < n_upstream ; i++ ) {
            /* Don't issue invalidate to the person that sent us the command that's causing
             * us to invalidate.
             *
             * Exemption:  Invalidating a block (due to INCLUSIVEness replacement)
             *             This can be checked by if the block's address doesn't
             *             match the event's address
             */
            if ( upstream_links[i]->getId() != ev->getLinkId() || (addrToBlockAddr(ev->getAddr()) != addr) ) {
                upstream_links[i]->send(new MemEvent(invalidateEvent));
                inv.waitingACKs++;
            }
        }
    }
    dbg.output(CALL_INFO, "Expecting %d acknowledgments\n", inv.waitingACKs);
    if ( 0 == inv.waitingACKs ) { //a.k.a., nobody to worry about waiting for
        finishIssueInvalidate(addr);
    }
    delete invalidateEvent;

}


void Cache::finishBusSendInvalidation(BusHandlerArgs &args)
{
    args.invalidation.inv->busEvent = NULL;
}


void Cache::finishIssueInvalidate(Addr addr)
{
    assert(0 == invalidations[addr].waitingACKs);

    CacheBlock *block = invalidations[addr].block;
    if ( block ) {
        block->unlock();
        block->status = invalidations[addr].newStatus;
        block->last_touched = getCurrentSimTime();
        dbg.output(CALL_INFO, "Marking block 0x%"PRIx64" with status %d\n", block->baseAddr, block->status);
    }

    dbg.output(CALL_INFO, "Received all invalidate ACKs for 0x%"PRIx64" (%"PRIu64", %d)\n", addr, invalidations[addr].issuingEvent.first, invalidations[addr].issuingEvent.second);


    std::deque<std::pair<MemEvent*, SourceType_t> > waitingEvents = invalidations[addr].waitingEvents;
    /* Erase before processing, otherwise, we'll think we're still waiting for invalidations */
    invalidations.erase(addr);
    bool first = true;
    while ( waitingEvents.size() > 0 ) {
        std::pair<MemEvent*, SourceType_t> ev2 = waitingEvents.front();
        waitingEvents.pop_front();
        dbg.output(CALL_INFO, "Handling formerly blocked event (%"PRIu64", %d) [%s: 0x%"PRIx64"]\n",
                ev2.first->getID().first, ev2.first->getID().second,
                CommandString[ev2.first->getCmd()], ev2.first->getAddr());
        handleIncomingEvent(ev2.first, ev2.second, false, first);
        first = false;
    }

    handlePendingEvents(findRow(addr), NULL);

}



/* Takes ownership of the event 'ev'. */
void Cache::loadBlock(MemEvent *ev, SourceType_t src)
{

    std::pair<LoadInfo_t*, bool> initRes = initLoad(ev->getAddr(), ev, src);
    LoadInfo_t *li = initRes.first;
    bool reprocess = !initRes.second;

    /* Check to see if this a reprocess of the head event */
    if ( reprocess && li->initiatingEvent != ev->getID() ) {
        dbg.output(CALL_INFO, "Adding to existing outstanding Load for this block.\n");
        li->list.push_back(LoadInfo_t::LoadElement_t(ev, src, getCurrentSimTime()));
        return;
    }

    /* Don't bother messing with the cache if this is an uncached operation */
    if ( !li->uncached ) {
        CacheRow *row = findRow(ev->getAddr());

        if ( NULL == li->targetBlock ) {
            li->targetBlock = row->getLRU();
        }

        if ( !li->targetBlock ) { // Row is full, wait for one to be available
            row->addWaitingEvent(ev, src);
            return;
        } else {
            CacheBlock *block = li->targetBlock;
            block->loadInfo = li;
            if ( INCLUSIVE == cacheMode && CacheBlock::EXCLUSIVE != block->status ) {
                if ( CacheBlock::DIRTY_PRESENT == block->status ) {
                    dbg.output(CALL_INFO, "Replacing a block to handle load.  Need to invalidate any upstream DIRTY copies of old cache block 0x%"PRIx64" [%d.%d].\n", block->baseAddr, block->status, block->user_locked);
                    issueInvalidate(ev, src, block, CacheBlock::EXCLUSIVE, SEND_UP, false);
                    return;
                } else if ( CacheBlock::DIRTY_UPSTREAM == block->status ) {
                    dbg.output(CALL_INFO, "Replacing a block to handle load.  Need to fetch upstream DIRTY copies of old cache block 0x%"PRIx64" [%d.%d].\n", block->baseAddr, block->status, block->user_locked);
                    //issueInvalidate(ev, src, block, CacheBlock::EXCLUSIVE, SEND_UP, false);
                    fetchBlock(ev, block, src);
                    return;
                } else if ( CacheBlock::INVALID != block->status ) {
                    dbg.output(CALL_INFO, "Replacing a block to handle load.  Need to invalidate any upstream copies of old cache block 0x%"PRIx64" [%d.%d].\n", block->baseAddr, block->status, block->user_locked);
                    /* We can go straight to INVALID...
                     *  INCLUSIVE cache's aren't L1 (why bother?)
                     *  Upstream caches either don't have it, have it in SHARED (clean)
                     *  Or EXCLUSIVE, in which case they'll write back before ack'ing.
                     */
                    if ( waitingForInvalidate(block->baseAddr) ) {
                        // Already working to invalidate this
                    }
                    issueInvalidate(ev, src, block, CacheBlock::INVALID, SEND_UP, false);
                    return;
                }
            }
            if ( CacheBlock::EXCLUSIVE == block->status ) {
                dbg.output(CALL_INFO, "Need to evict block 0x%"PRIx64" to satisfy load for 0x%"PRIx64"\n",
                        block->baseAddr, ev->getAddr());

                row->addWaitingEvent(ev, src);
                writebackBlock(block, CacheBlock::INVALID); // We'll get it next time

                return;
            } else {
                dbg.output(CALL_INFO, "Replacing block (old status is [%d], 0x%"PRIx64" [%s]\n",
                        block->status, block->baseAddr,
                        block->locked ? "LOCKED" : "unlocked");
                if ( block->isValid() && directory_link ) {
                    // Send Eviction notification
                    dbg.output(CALL_INFO, "Sending eviction notification of 0x%"PRIx64" to directory.\n", block->baseAddr);
                    MemEvent *evict = new MemEvent(this, block->baseAddr, Evicted);
                    evict->setDst(findTargetDirectory(block->baseAddr));
                    directory_link->send(evict);
                }
            }
        }

        /* Simple Load */
        li->targetBlock->activate(ev->getAddr());
        li->targetBlock->lock();
    }

    li->loadDirection = SEND_DOWN;

    if ( reprocess )
        li->list.push_front(LoadInfo_t::LoadElement_t(ev, src, getCurrentSimTime()));
    else
        li->list.push_back(LoadInfo_t::LoadElement_t(ev, src, getCurrentSimTime()));

    li->eventScheduled = true;
    self_link->send(1, new SelfEvent(this, &Cache::finishLoadBlock, li, li->addr, li->targetBlock));
}


std::pair<Cache::LoadInfo_t*,bool> Cache::initLoad(Addr addr, MemEvent *ev, SourceType_t src)
{
    bool initialEvent = false;

    Addr blockAddr = addrToBlockAddr(addr);
	LoadList_t::iterator i = waitingLoads.find(blockAddr);
    LoadInfo_t *li = NULL;
	if ( i != waitingLoads.end() ) {
		li = i->second;
		assert(li->addr == blockAddr);
        initialEvent = false;

    } else {
        li = new LoadInfo_t(blockAddr);
        dbg.output(CALL_INFO, "No existing Load for this block.  Creating.  [li: %p]\n", li);
        waitingLoads.insert(std::make_pair(blockAddr, li));
        li->initiatingEvent = ev->getID();
        li->uncached = ev->queryFlag(MemEvent::F_UNCACHED);
        initialEvent = true;
    }

    return std::make_pair(li, initialEvent);

}


void Cache::finishLoadBlock(LoadInfo_t *li, Addr addr, CacheBlock *block)
{
    li->nackRescheduled = false;
    li->eventScheduled = false;
    if ( li->satisfied ) {
        assert(NULL == li->busEvent);
        delete li;
        return;
    }

    dbg.output(CALL_INFO, "Time to send load for 0x%"PRIx64"\n", addr);

    /* Check to see if we're still in ASSIGNED state.  If not, we've probably
     * already been processed.
     * Unless this is a SEND_UP fetch, and we're DIRTY.
     */
    if ( (!li->uncached) &&
             !((CacheBlock::DIRTY_UPSTREAM == block->status) && (SEND_UP == li->loadDirection)) &&
             ((CacheBlock::ASSIGNED != block->status) || (block->baseAddr != addr) || (li != block->loadInfo))) {
        dbg.output(CALL_INFO, "Not going to bother loading.  Somebody else has moved block 0x%"PRIx64" to state [%d]\n",
                block->baseAddr, block->status);
        return;
    }

    if ( SEND_UP == li->loadDirection ) {
        if ( n_upstream > 0 && !isL1 ) {
            for ( int i = 0 ; i < n_upstream ; i++ ) {
                MemEvent *req = new MemEvent(this, li->addr, RequestData);
                li->loadingEvent = req->getID();
                req->setSize(blocksize);
                upstream_links[i]->send(req);
            }
        } else if ( snoop_link ) {
            MemEvent *req = new MemEvent(this, li->addr, RequestData);
            li->loadingEvent = req->getID();
            req->setSize(blocksize);
            dbg.output(CALL_INFO, "Enqueuing request to load block 0x%"PRIx64"  [li = %p]\n", li->addr, li);
            BusHandlerArgs args;
            args.loadBlock.loadInfo = li;
            li->eventScheduled = true;
            li->busEvent = req;
            snoopBusQueue.request( req, BusHandlers(NULL, &Cache::finishLoadBlockBus, args));
        }
    } else {

        /* Ordering here...  If you have both downstream & snoop, you're probably
         * at the end of the line.  You don't need to issue on snoop, somebody else
         * probably did.  Just send the load down the line to memory.
         */
        if ( downstream_link ) {
            dbg.output(CALL_INFO, "Sending request to load block 0x%"PRIx64"  [li = %p]\n", li->addr, li);
            MemEvent *req = new MemEvent(this, li->addr, RequestData);
            li->loadingEvent = req->getID();
            req->setSize(blocksize);
            downstream_link->send(req);
        } else if ( directory_link ) {
            MemEvent *req = new MemEvent(this, li->addr, RequestData);
            li->loadingEvent = req->getID();
            dbg.output(CALL_INFO, "Sending request (%"PRIu64", %d) to Directory to load block 0x%"PRIx64"  [li = %p]\n", req->getID().first, req->getID().second, li->addr, li);
            req->setSize(blocksize);
            req->setDst(findTargetDirectory(li->addr));
            directory_link->send(req);
        } else if ( snoop_link ) {
            MemEvent *req = new MemEvent(this, li->addr, RequestData);
            li->loadingEvent = req->getID();
            req->setSize(blocksize);
            if ( next_level_name != NO_NEXT_LEVEL ) req->setDst(next_level_name);
            dbg.output(CALL_INFO, "Enqueuing request to load block 0x%"PRIx64"  [li = %p]\n", li->addr, li);
            BusHandlerArgs args;
            args.loadBlock.loadInfo = li;
            li->eventScheduled = true;
            li->busEvent = req;
            snoopBusQueue.request( req, BusHandlers(NULL, &Cache::finishLoadBlockBus, args));
        }
    }
}


void Cache::finishLoadBlockBus(BusHandlerArgs &args)
{
	args.loadBlock.loadInfo->busEvent = NULL;
    args.loadBlock.loadInfo->eventScheduled = false;
    if ( args.loadBlock.loadInfo->satisfied ) {
        delete args.loadBlock.loadInfo;
    }
}



void Cache::handleCacheRequestEvent(MemEvent *ev, SourceType_t src, bool firstProcess)
{
	if ( src == SNOOP && ev->getSrc() == getName() ) {
        delete ev;
        return; // We sent it, ignore
    }

	CacheBlock *block = findBlock(ev->getAddr(), false);
	dbg.output(CALL_INFO, "0x%"PRIx64" %s %s (block 0x%"PRIx64" [%d.%d])%s\n", ev->getAddr(),
			(src == SNOOP && ev->getDst() != getName()) ? "SNOOP" : "",
			(block) ? "HIT" : "MISS",
			addrToBlockAddr(ev->getAddr()),
            (block) ? block->status : -1,
            (block) ? block->user_locked : -1,
            (block && block->isLocked()) ? " Block LOCKED" : "");

    if ( ev->getSize() != blocksize ) {
        _abort(Cache, "It appears that not all cache line/block sizes are equal.  Unsupported!\n");
    }

    if ( ev->queryFlag(MemEvent::F_UNCACHED) ) {
        loadBlock(ev, src);
        return;
    }

	if ( block ) {
        if ( CacheBlock::DIRTY_UPSTREAM == block->status ) {
            if ( src == SNOOP ) {
                /* Pretend we don't have it.  Somebody else will supply it. */
                delete ev;
                return;
            } else {
                /* TODO  - maybe? */
                assert(false);
            }
        }

		if ( firstProcess && PREFETCHER != src ) {
            listener->notifyAccess(CacheListener::READ, CacheListener::HIT, ev->getAddr());
            num_supply_hit++;
        }

        if ( PREFETCHER == src ) {
            dbg.output(CALL_INFO, "Prefetcher wants us to load what we already have.  Return.\n");
            delete ev;
            return;
        }

		/* Hit */
		if ( supplyInProgress(block->baseAddr, src) ) {
			// we're already working on this
			dbg.output(CALL_INFO, "Detected that we're already working on this\n");
            delete ev;
			return;
		}

        if ( waitingForInvalidate(block->baseAddr) ) {
            if ( /*isL1 ||*/ DIRECTORY == src ) {
                dbg.output(CALL_INFO, "Invalidation (%"PRIu64", %d) for 0x%"PRIx64" in progress.  Putting into queue.\n", invalidations[block->baseAddr].issuingEvent.first, invalidations[block->baseAddr].issuingEvent.second, block->baseAddr);
                invalidations[block->baseAddr].waitingEvents.push_back(std::make_pair(ev, src));
            } else {
                dbg.output(CALL_INFO, "Invalidation (%"PRIu64", %d) for 0x%"PRIx64" in progress.  Responding NACK.\n", invalidations[block->baseAddr].issuingEvent.first, invalidations[block->baseAddr].issuingEvent.second, block->baseAddr);
                respondNACK(ev, src);
                delete ev;
            }
            return;
        }

        dbg.output(CALL_INFO, "CacheRequest Hit for 0x%"PRIx64", will supply data\n", block->baseAddr);
        if ( block->wb_in_progress ) {
            dbg.output(CALL_INFO, "There's a WB in progress.  That will suffice.\n");
            delete ev;
        } else {
            suppliesInProgress.insert(std::make_pair(std::make_pair(block->baseAddr, src), SupplyInfo(ev)));
            block->lock();
            block->last_touched = getCurrentSimTime();
            self_link->send(1, new SelfEvent(this, &Cache::supplyData, ev, block, src));
        }
	} else {
		/* Miss */
        if ( src == DOWNSTREAM ) {
            dbg.output(CALL_INFO, "DOWNSTREAM request for 0x%"PRIx64" is a Miss.  Ignoring.  Most likely, we just recently wrote the data back anyway.\n", ev->getAddr());
            /* TODO:  May need to request upstream of us */
            delete ev;
        } else if ( SNOOP != src || ev->getDst() == getName() ) {

			if ( firstProcess && PREFETCHER != src ) {
                listener->notifyAccess(CacheListener::READ, CacheListener::MISS, ev->getAddr());
                num_supply_miss++;
            }
			loadBlock(ev, src);
		} else {
            /* Ignore  - Snoop request, or not to us */
            delete ev;
        }
	}
}


void Cache::supplyData(MemEvent *ev, CacheBlock *block, SourceType_t src)
{
    dbg.output(CALL_INFO, "Time to supply data from block 0x%"PRIx64" [%d.%d]\n", block->baseAddr, block->status, block->user_locked);
	supplyMap_t::iterator supMapI = getSupplyInProgress(block->baseAddr, src, ev->getID());
	assert(supMapI != suppliesInProgress.end());


	if ( supMapI->second.canceled ) {
		dbg.output(CALL_INFO, "Request has been canceled!\n");
        delete supMapI->second.initiatingEvent;
		suppliesInProgress.erase(supMapI);
        block->unlock();
		return;
	}

    if ( waitingForInvalidate(block->baseAddr) ) {
        dbg.output(CALL_INFO, "Invalidation (%"PRIu64", %d) for this now in progress.  Not Supplying now.\n", invalidations[block->baseAddr].issuingEvent.first, invalidations[block->baseAddr].issuingEvent.second);

        suppliesInProgress.erase(supMapI);
        if ( isL1 || src == DIRECTORY ) {
            invalidations[block->baseAddr].waitingEvents.push_back(std::make_pair(ev, src));
        } else {
            respondNACK(ev, src);
            delete ev;
        }
        block->unlock();
        return;
    }


	MemEvent *resp = new MemEvent(this, block->baseAddr, SupplyData);
    if ( ev->queryFlag(MemEvent::F_UNCACHED) ) resp->setFlag(MemEvent::F_UNCACHED);

    if ( block->user_locked ) {
        block->user_lock_needs_wb = true;
        block->user_lock_sent_delayed = false;
        resp->setFlag(MemEvent::F_DELAYED);
        resp->setSize(blocksize);
    } else {
        if ( CacheBlock::EXCLUSIVE == block->status )
            resp->setFlag(MemEvent::F_WRITEBACK);
        resp->setPayload(block->data);
    }


	switch (src) {
	case DOWNSTREAM:
        dbg.output(CALL_INFO, "Sending Supply of 0x%"PRIx64" downstream.\n", block->baseAddr);
        block->unlock();
		downstream_link->send(resp);
        suppliesInProgress.erase(supMapI);
        if ( !resp->queryFlag(MemEvent::F_DELAYED) ) {
            block->status = CacheBlock::SHARED;
            block->last_touched = getCurrentSimTime();
            handlePendingEvents(findRow(block->baseAddr), NULL);
            dbg.output(CALL_INFO, "Marking block 0x%"PRIx64" with status %d\n", block->baseAddr, block->status);
        }
        break;
	case SNOOP: {
		BusHandlerArgs args;
        args.supplyData.initiatingEvent = ev;
		args.supplyData.block = block;
		args.supplyData.src = src;
        args.supplyData.isFakeSupply = resp->queryFlag(MemEvent::F_DELAYED);
		supMapI->second.busEvent = resp;
		dbg.output(CALL_INFO, "Enqueuing request to supply%s block 0x%"PRIx64"\n",
                args.supplyData.isFakeSupply ? " delay" : "",
                block->baseAddr);
		snoopBusQueue.request( resp, BusHandlers(&Cache::prepBusSupplyData, &Cache::finishBusSupplyData, args));
        /* Don't just break, we don't want to delete the event yet */
        return;
    }
	case DIRECTORY:
        dbg.output(CALL_INFO, "Sending Supply of 0x%"PRIx64" to Directory.\n", block->baseAddr);
        block->unlock();
		directory_link->send(resp);
        suppliesInProgress.erase(supMapI);
        assert( !resp->queryFlag(MemEvent::F_DELAYED) );
        block->last_touched = getCurrentSimTime();
        block->status = CacheBlock::SHARED;
        dbg.output(CALL_INFO, "Marking block 0x%"PRIx64" with status %d\n", block->baseAddr, block->status);
        handlePendingEvents(findRow(block->baseAddr), NULL);
        break;
	case UPSTREAM:
        dbg.output(CALL_INFO, "Sending Supply of 0x%"PRIx64" upstream.\n", block->baseAddr);
        block->unlock();
		upstream_links[upstreamLinkMap[ev->getLinkId()]]->send(resp);
        suppliesInProgress.erase(supMapI);
        block->status = (block->isDirty()) ? CacheBlock::DIRTY_PRESENT : CacheBlock::SHARED;
        dbg.output(CALL_INFO, "Marking block 0x%"PRIx64" with status %d\n", block->baseAddr, block->status);
        handlePendingEvents(findRow(block->baseAddr), NULL);
        break;
    default:
        block->unlock();
        break;
    }
	delete ev;
}

void Cache::prepBusSupplyData(BusHandlerArgs &args, MemEvent *ev)
{
    CacheBlock *block = args.supplyData.block;
    if (block->baseAddr != addrToBlockAddr(args.supplyData.initiatingEvent->getAddr())) {
        _abort(Cache, "0x%"PRIx64" != 0x%"PRIx64"\n", block->baseAddr, args.supplyData.initiatingEvent->getAddr());
    }
    /* Chance to recover from sending a DELAYED packet */
    if ( args.supplyData.isFakeSupply && !block->user_locked ) {
        dbg.output(CALL_INFO, "Changing writeback 0x%"PRIx64" from a DELAYED response to a real one.\n", ev->getAddr());
        args.supplyData.isFakeSupply = false;
        block->user_lock_needs_wb = false;

        ev->clearFlag(MemEvent::F_DELAYED);
        ev->setFlag(MemEvent::F_WRITEBACK);

        /* Need to check to see if we've already enqueued the actual writeback as well */

    }
    ev->setPayload(block->data);
}


void Cache::finishBusSupplyData(BusHandlerArgs &args)
{
    CacheBlock *block = args.supplyData.block;
    assert(block->baseAddr == args.supplyData.initiatingEvent->getAddr());
	dbg.output(CALL_INFO, "Supply Message sent for block 0x%"PRIx64"\n", block->baseAddr);
	if ( !args.supplyData.isFakeSupply ) {
        block->status = (cacheMode == INCLUSIVE && block->isDirty() ) ? CacheBlock::DIRTY_PRESENT : CacheBlock::SHARED;
        block->last_touched = getCurrentSimTime();
        dbg.output(CALL_INFO, "Marking block 0x%"PRIx64" with status %d\n", block->baseAddr, block->status);
    } else {
        dbg.output(CALL_INFO, "Sent F_DELAYED for 0x%"PRIx64"\n", block->baseAddr);
        block->user_lock_sent_delayed = true;
    }

    block->unlock();

    if ( args.supplyData.initiatingEvent->queryFlag(MemEvent::F_UNCACHED) ) {
        /* This is a fake block... delete it */
        delete block;
    }

	supplyMap_t::iterator supMapI = getSupplyInProgress(args.supplyData.initiatingEvent->getAddr(), args.supplyData.src, args.supplyData.initiatingEvent->getID());
	assert(supMapI != suppliesInProgress.end());
	suppliesInProgress.erase(supMapI);

    delete args.supplyData.initiatingEvent;

	if ( !args.supplyData.isFakeSupply ) {
        handlePendingEvents(findRow(block->baseAddr), NULL);
    }
}



void Cache::handleCacheSupplyEvent(MemEvent *ev, SourceType_t src)
{
	if ( src == SNOOP && ev->getSrc() == getName() ) {
        delete ev;
        return; // We sent it
    }

    if ( ev->queryFlag(MemEvent::F_UNCACHED) ) {
        dbg.output(CALL_INFO, "Received UNCACHED SupplyEvent 0x%"PRIx64"\n", ev->getAddr());
        handleUncachedWrite(ev, src);
        return;
    }

	/*
	 * If snoop, cancel any supplies we're trying to do
	 * Check to see if we're trying to load this data, if so, handle
	 */
	if ( src == SNOOP ) {
		if ( ev->getSize() >= blocksize ) {
			Addr blkAddr = addrToBlockAddr(ev->getAddr());
			while ( blkAddr < (ev->getAddr() + ev->getSize()) ) {
                CacheBlock *b = findBlock(blkAddr);
                assert ( !b || CacheBlock::EXCLUSIVE != b->status );
				supplyMap_t::iterator supMapI = getSupplyInProgress(blkAddr, src);
				if ( supMapI != suppliesInProgress.end() ) {
					// Mark it canceled
					dbg.output(CALL_INFO, "Marking request for 0x%"PRIx64" as canceled\n", ev->getAddr());
					supMapI->second.canceled = true;
					if ( NULL != supMapI->second.busEvent ) {
						// Bus requested.  Cancel it, too
						dbg.output(CALL_INFO, "Canceling Bus Request for Supply on 0x%"PRIx64" (%p) (%"PRIu64", %d)\n", supMapI->second.busEvent->getAddr(), supMapI->second.busEvent, supMapI->second.busEvent->getID().first, supMapI->second.busEvent->getID().second);
                        bool canceled = snoopBusQueue.cancelRequest(supMapI->second.busEvent);
						if ( canceled ) {
                            b->unlock();
                            delete supMapI->second.busEvent;
							supMapI->second.busEvent = NULL;
                            delete supMapI->second.initiatingEvent;
                            suppliesInProgress.erase(supMapI);
						}
					}
				}
				blkAddr += blocksize;
			}
		}
	}


	/* Check to see if we're trying to load this data */
    LoadList_t::iterator i = waitingLoads.find(ev->getAddr());
    if ( waitingLoads.end() != i ) {
        LoadInfo_t* li = i->second;
        dbg.output(CALL_INFO, "We were waiting for block 0x%"PRIx64".  Processing.  [li: %p]\n", ev->getAddr(), li);

        if ( (!li->uncached) &&  ((NULL == li->targetBlock) || (li->targetBlock->baseAddr != ev->getAddr())) ) {
            dbg.output(CALL_INFO, "No block available yet.  We didn't ask for it.  Ignoring.\n");
            /* We still don't have a block assigned, so we didn't ask for
             * this.  Must be a snoop that we can ignore.
             * (no room in the inn) */
            assert( src == SNOOP );
        } else {

            if ( li->busEvent ) {
                dbg.output(CALL_INFO, "Canceling Bus Request for Load on 0x%"PRIx64"\n", li->busEvent->getAddr());
                if ( snoopBusQueue.cancelRequest(li->busEvent) ) {
                    delete li->busEvent;
                }
                li->busEvent = NULL;
            }


            CacheBlock *targetBlock = li->targetBlock;

            if ( ev->queryFlag(MemEvent::F_DELAYED) ) {
                /* If we're trying to load this, but this is locked elsewhere, we need to wait longer for the data. */
                dbg.output(CALL_INFO, "Got a DELAYED Response.  Purge snoop work.\n");
                uint32_t deleted = 0;
                for ( uint32_t n = 0 ; n < li->list.size() ; n++ ) {
                    LoadInfo_t::LoadElement_t &oldEV = li->list[n];
                    if ( src == SNOOP && oldEV.src == SNOOP ) {
                        delete oldEV.ev;
                        oldEV.ev = NULL;
                        deleted++;
                    }
                }
                if ( deleted == li->list.size() ) {
                    /* Deleted all reasons to load this block */
                    waitingLoads.erase(i);
                    li->targetBlock->loadInfo = NULL;
                    if ( li->eventScheduled )
                        li->satisfied = true;
                    else
                        delete li;
                    if ( targetBlock->isAssigned() ){
                        targetBlock->status = CacheBlock::INVALID;
                        dbg.output(CALL_INFO, "Marking block 0x%"PRIx64" with status %d\n", targetBlock->baseAddr, targetBlock->status);
                    }
                    targetBlock->unlock();
                }

            } else {
                bool use_cache = !li->uncached;
                if ( use_cache ) {
                    updateBlock(ev, li->targetBlock);
                    li->targetBlock->loadInfo = NULL;
                    li->targetBlock->status = (cacheMode == INCLUSIVE && ev->queryFlag(MemEvent::F_WRITEBACK)) ? CacheBlock::DIRTY_PRESENT : CacheBlock::SHARED;
                    li->targetBlock->last_touched = getCurrentSimTime();
                    dbg.output(CALL_INFO, "Marking block 0x%"PRIx64" with status %d\n", li->targetBlock->baseAddr, li->targetBlock->status);
                    li->targetBlock->unlock();
                }

                std::deque<LoadInfo_t::LoadElement_t> list = li->list;
                waitingLoads.erase(i);

                if ( li->eventScheduled )
                    li->satisfied = true;
                else
                    delete li;

                for ( uint32_t n = 0 ; n < list.size() ; n++ ) {
                    LoadInfo_t::LoadElement_t &oldEV = list[n];
                    /* If this was from the Snoop Bus, and we've got other cache's asking
                     * for this data over the snoop bus, we can assume they saw it, and
                     * we don't need to reprocess them.
                     */
                    if ( src == SNOOP && oldEV.src == SNOOP && oldEV.ev->getAddr() == ev->getAddr() ) {
                        dbg.output(CALL_INFO, "Ignoring old event because it came over snoop.\n");
                        delete oldEV.ev;
                    } else {
                        if ( NULL != oldEV.ev ) { // handled before?
                            if ( use_cache ) {
                                handleIncomingEvent(oldEV.ev, oldEV.src, false, true);
                            } else {
                                assert(oldEV.ev->queryFlag(MemEvent::F_UNCACHED));

                                /* We need to send upstream the data that came in, without loading into our cache. */
                                CacheBlock* fakeBlock = new CacheBlock(this);
                                fakeBlock->baseAddr = ev->getAddr();
                                updateBlock(ev, fakeBlock);
                                switch ( oldEV.ev->getCmd() ) {
                                case ReadReq: {
                                    MemEvent *ev = makeCPUResponse(oldEV.ev, fakeBlock, UPSTREAM);
                                    sendCPUResponse(ev, NULL, UPSTREAM);
                                    delete fakeBlock;
                                    break;
                                }
                                case RequestData: {
                                    suppliesInProgress.insert(std::make_pair(std::make_pair(fakeBlock->baseAddr, src), SupplyInfo(oldEV.ev)));
                                    fakeBlock->lock();
                                    supplyData(oldEV.ev, fakeBlock, oldEV.src);
                                    if ( SNOOP != oldEV.src ) delete fakeBlock;
                                    break;
                                }
                                default:
                                    _abort(Cache, "Unexpected command uncached command %s\n", CommandString[oldEV.ev->getCmd()]);
                                }
                            }
                        } else {
                            dbg.output(CALL_INFO, "Ignoring old event as it appears to have been handled.\n");
                        }
                    }
                }
            }

            if ( targetBlock ) // Wont be set if we're handling an uncached request
                handlePendingEvents(findRow(targetBlock->baseAddr), targetBlock);
        }

    } else {
        bool forwardDownstream = true;
        if ( cacheMode == INCLUSIVE ) {
            /* Not waiting for this load, and we're INCLUSIVE -> Must be a writeback. */
            CacheBlock *block = findBlock(ev->getAddr(), false);
            assert(NULL != block);
            dbg.output(CALL_INFO, "Current block status for 0x%"PRIx64" is [%d.%d]\n", block->baseAddr, block->status, block->user_locked);
            assert(CacheBlock::DIRTY_UPSTREAM == block->status || SNOOP == src);
            updateBlock(ev, block);
            block->status = (block->isDirty() ) ? CacheBlock::DIRTY_PRESENT : CacheBlock::SHARED;
            block->last_touched = getCurrentSimTime();
            forwardDownstream = false;
            dbg.output(CALL_INFO, "Marking block 0x%"PRIx64" with status %d\n", block->baseAddr, block->status);
        }
        if ( SNOOP == src ) {
            dbg.output(CALL_INFO, "No matching waitingLoads for 0x%"PRIx64"%s.\n", ev->getAddr(), ev->queryFlag(MemEvent::F_WRITEBACK) ? " [WRITEBACK]" : "");
            if ( ev->getDst() == getName() ) {
                dbg.output(CALL_INFO, "WARNING:  Unmatched message.  Hopefully we recently just canceled this request, and our sender didn't get the memo.\n");
            } else if ( forwardDownstream && downstream_link && ev->queryFlag(MemEvent::F_WRITEBACK) ) {
                // We're on snoop, but have a downstream, and this is a writeback.
                // We should probably pass it on.
                dbg.output(CALL_INFO, "Forwarding writeback of 0x%"PRIx64" downstream.\n", ev->getAddr());
                downstream_link->send(new MemEvent(ev));
            } else if ( forwardDownstream && directory_link && ev->queryFlag(MemEvent::F_WRITEBACK) ) {
                // Need to set src properly for the network.
                dbg.output(CALL_INFO, "Forwarding writeback of 0x%"PRIx64" to directory.\n", ev->getAddr());
                MemEvent *newev = new MemEvent(ev);
                newev->setSrc(getName());
                newev->setDst(findTargetDirectory(ev->getAddr()));
                directory_link->send(newev);
            }
        } else if ( forwardDownstream && UPSTREAM == src ) {
            assert(ev->queryFlag(MemEvent::F_WRITEBACK));
            dbg.output(CALL_INFO, "Passing on writeback to next level\n");
            if ( downstream_link ) {
                downstream_link->send(new MemEvent(ev));
            } else if ( directory_link ) {
                // Need to set src properly for the network.
                MemEvent *newev = new MemEvent(ev);
                newev->setSrc(getName());
                newev->setDst(findTargetDirectory(ev->getAddr()));
                directory_link->send(newev);
            } else {
                _abort(Cache, "Not sure where to send this.  Directory?\n");
            }
        } else {
            //_abort(Cache, "Unhandled case of unmatched SupplyData coming from non-SNOOP, non-UPSTREAM\n");
        }
        handlePendingEvents(findRow(ev->getAddr()), NULL);
    }

    delete ev;
}




void Cache::handleInvalidate(MemEvent *ev, SourceType_t src, bool finishedUpstream)
{
	if ( SNOOP == src && ev->getSrc() == getName() ) {
        ackInvalidate(ev);
        return;
    }

	CacheBlock *block = findBlock(ev->getAddr());
    assert( !block || block->baseAddr == addrToBlockAddr(ev->getAddr()) );

    dbg.output(CALL_INFO, "Received Invalidate Event 0x%"PRIx64"  (%"PRIu64", %d) [Block status: %d] \n", ev->getAddr(), ev->getID().first, ev->getID().second, block ? block->status : -1);

    if ( block ) {
        supplyMap_t::iterator supMapI = getSupplyInProgress(block->baseAddr, SNOOP);
        if ( supMapI != suppliesInProgress.end() ) {
            dbg.output(CALL_INFO, "Supply is in progress for 0x%"PRIx64".  Cancel supply and re-issue.\n", ev->getAddr());
            supMapI->second.canceled = true;
            if ( NULL != supMapI->second.busEvent ) {
                // Bus requested.  Cancel it, too
                dbg.output(CALL_INFO, "Canceling Bus Request for Supply on 0x%"PRIx64" (%p)\n", supMapI->second.busEvent->getAddr(), supMapI->second.busEvent);
                if ( snoopBusQueue.cancelRequest(supMapI->second.busEvent) ) {
                    block->unlock();
                    delete supMapI->second.busEvent;
                    supMapI->second.busEvent = NULL;
                }
            }
            self_link->send(1, new SelfEvent(this, &Cache::retryEvent, new MemEvent(supMapI->second.initiatingEvent), block, supMapI->first.second));
        }


        if ( waitingForInvalidate(block->baseAddr) ) {
            bool ok = cancelInvalidate(block); /* Should cause a re-issue of the write */
            if ( !ok ) {
                /* This was an un-cancelable invalidate that is in progress.
                 * NACK this
                 */
                if ( DIRECTORY == src ) {
                     invalidations[block->baseAddr].waitingEvents.push_back(std::make_pair(ev, src));
                     return;
                } else  {
                    respondNACK(ev, src);
                    delete ev;
                    return;
                }
            }
        }

        /* If we're inclusive, the block is already dirty, and the request came from upstream,
         * we don't need to pass this down, or anything like that.  Just ack, and return.
         */
        if ( INCLUSIVE == cacheMode && (UPSTREAM == src || SNOOP == src) ) {
            if ( CacheBlock::DIRTY_UPSTREAM == block->status ) {
                goto done;
            } else if ( CacheBlock::DIRTY_PRESENT == block->status ) {
                finishedUpstream = true; // Don't need to ask downstream, we already own it.
            }
        }

    }



    if ( !finishedUpstream && (DOWNSTREAM == src || DIRECTORY == src) && !isL1 ) {
        dbg.output(CALL_INFO, "Forwarding invalidate 0x%"PRIx64" on upstream.\n", ev->getAddr());
        issueInvalidate(ev, src, ev->getAddr(), SEND_UP, false);
        return;
    }

    if ( !finishedUpstream && ((UPSTREAM == src) || (SNOOP == src && NULL != directory_link))) {
        dbg.output(CALL_INFO, "Forwarding invalidate 0x%"PRIx64" downstream\n", ev->getAddr());
        if ( block ) {
            issueInvalidate(ev, src, block, (cacheMode == INCLUSIVE) ? CacheBlock::DIRTY_UPSTREAM : CacheBlock::INVALID, SEND_DOWN, true);
        } else {
            issueInvalidate(ev, src, ev->getAddr(), SEND_DOWN, true);
        }
        return;
    }

    if ( block ) {
        if ( CacheBlock::SHARED == block->status || CacheBlock::DIRTY_PRESENT == block->status ) {
            dbg.output(CALL_INFO, "Invalidating block 0x%"PRIx64"\n", block->baseAddr);
            /* If we're trying to supply this block, cancel that. */

            supplyMap_t::iterator supMapI = getSupplyInProgress(block->baseAddr, SNOOP);
            if ( suppliesInProgress.end() != supMapI ) {
                supMapI->second.canceled = true;
                if ( NULL != supMapI->second.busEvent ) {
                    // Bus requested.  Cancel it, too
                    dbg.output(CALL_INFO, "Canceling Bus Request for Supply on 0x%"PRIx64" (%p)\n", supMapI->second.busEvent->getAddr(), supMapI->second.busEvent);
                    if ( snoopBusQueue.cancelRequest(supMapI->second.busEvent) ) {
                        block->unlock();
                        delete supMapI->second.busEvent;
                        supMapI->second.busEvent = NULL;
                    }
                }
            }


            if ( (INCLUSIVE == cacheMode) && (DOWNSTREAM != src) && (DIRECTORY != src) ) {
                block->status = CacheBlock::DIRTY_UPSTREAM;
                block->last_touched = getCurrentSimTime();
                dbg.output(CALL_INFO, "Marking block 0x%"PRIx64" as DIRTY.\n", block->baseAddr);
            } else {
                block->status = CacheBlock::INVALID;
                block->last_touched = getCurrentSimTime();
                ++num_invalidates;
                dbg.output(CALL_INFO, "Marking block 0x%"PRIx64" as INVALID.\n", block->baseAddr);
                /* TODO:  Lock status? */
                handlePendingEvents(findRow(block->baseAddr), NULL);
            }
        } else if ( CacheBlock::EXCLUSIVE == block->status ) {
            dbg.output(CALL_INFO, "Invalidating EXCLUSIVE block 0x%"PRIx64" -> Issue writeback, pend invalidate\n", block->baseAddr);
            CacheRow *row = findRow(block->baseAddr);
            row->addWaitingEvent(ev, src);
            writebackBlock(block, CacheBlock::INVALID);
            ++num_invalidates;
            return;
        }

    }

done:

    /* We'll need to ACK this (unless optimized ACK mode)  */
    if ( SNOOP != src || snoopBusQueue.getNumPeers() > 1 )
        sendInvalidateACK(ev, src);
    delete ev;
}


void Cache::sendInvalidateACK(MemEvent *ev, SourceType_t src)
{
    MemEvent *resp = ev->makeResponse(this);
    dbg.output(CALL_INFO, "Sending ACK for %s 0x%"PRIx64" to %s\n", CommandString[ev->getCmd()], ev->getAddr(), resp->getDst().c_str());
    switch (src) {
    case SNOOP:
        snoopBusQueue.request(resp);
        break;
    case UPSTREAM:
        upstream_links[upstreamLinkMap[ev->getLinkId()]]->send(resp);
        break;
    case DOWNSTREAM:
        downstream_link->send(resp);
        break;
    case DIRECTORY:
        directory_link->send(resp);
        break;
    case SELF:
        _abort(Cache, "Why are we acking to ourselfs?\n");
        break;
    case PREFETCHER:
        _abort(Cache, "Check this:  Sending Invalidate ACK to the prefetcher?\n");
        break;
    }
}


bool Cache::waitingForInvalidate(Addr addr)
{
    std::map<Addr, Invalidation>::iterator i = invalidations.find(addr);
	return ( i != invalidations.end() );
}

bool Cache::cancelInvalidate(CacheBlock *block)
{
    std::map<Addr, Invalidation>::iterator i = invalidations.find(block->baseAddr);
    assert(i != invalidations.end() );

    if ( i->second.canCancel ) {
        dbg.output(CALL_INFO, "Attempting cancel for Invalidate 0x%"PRIx64" (%"PRIu64", %d)\n", block->baseAddr, i->second.issuingEvent.first, i->second.issuingEvent.second);

        if ( NULL != i->second.busEvent && snoop_link && snoopBusQueue.cancelRequest(i->second.busEvent) ) {
            delete i->second.busEvent;
        }

        std::deque<std::pair<MemEvent*, SourceType_t> > waitingEvents = i->second.waitingEvents;
        /* Only unlock if we locked it before */
        if ( i->second.block == block ) block->unlock();
        /* Erase before processing, otherwise, we'll think we're still waiting for invalidations */
        invalidations.erase(i);

        dbg.output(CALL_INFO, "Due to cancel of Invalidate 0x%"PRIx64", re-issuing %zu events.\n", block->baseAddr, waitingEvents.size());
        while ( waitingEvents.size() > 0 ) {
            MemEvent *origEV = waitingEvents.front().first;
            SourceType_t origSRC = waitingEvents.front().second;
            waitingEvents.pop_front();
            self_link->send(1, new SelfEvent(this, &Cache::retryEvent, origEV, NULL, origSRC));
        }
        return true;
    } else {
        dbg.output(CALL_INFO, "Cannot cancel Invalidate 0x%"PRIx64"  (%"PRIu64", %d)\n", block->baseAddr, i->second.issuingEvent.first, i->second.issuingEvent.second);
        return false;
    }
}


void Cache::ackInvalidate(MemEvent *ev)
{
    Addr addr = ev->getAddr();

    dbg.output(CALL_INFO, "Attempting to acknowledge invalidate 0x%"PRIx64" event (%"PRIu64", %d)\n", ev->getAddr(), ev->getResponseToID().first, ev->getResponseToID().second);
    if ( waitingForInvalidate(addr) ) {
        if ( (ev->getResponseToID() == invalidations[addr].issuingEvent) ||
                (ev->getSrc() == getName()) /* Self event on bus */) {
            int remaining = --invalidations[addr].waitingACKs;
            dbg.output(CALL_INFO, "Acknoweldging an Invalidate (%"PRIx64", %d).  [%d remain]\n", invalidations[addr].issuingEvent.first, invalidations[addr].issuingEvent.second, remaining);

            assert(0 <= remaining);
            if ( 0 == remaining )
                finishIssueInvalidate(addr);
        } else {
            dbg.output(CALL_INFO, "We aren't waiting for this ACK.  Ignore.\n");
        }
    } else {
        dbg.output(CALL_INFO, "We aren't waiting for any ACKs to address 0x%"PRIx64".  Ignore.\n", addr);
    }

    delete ev;
}


void Cache::writebackBlock(CacheBlock *block, CacheBlock::BlockStatus newStatus)
{
    if ( block->wb_in_progress ) {
		dbg.output(CALL_INFO, "Writeback already in progress for block 0x%"PRIx64"\n", block->baseAddr);
        return;
    }
    block->wb_in_progress = true;
    if ( !(downstream_link || directory_link) && snoop_link) {
		block->lock();
		BusHandlerArgs args;
		args.writebackBlock.block = block;
		args.writebackBlock.newStatus = newStatus;
		args.writebackBlock.decrementLock = true;
		dbg.output(CALL_INFO, "Enqueuing request to writeback block 0x%"PRIx64"\n", block->baseAddr);

        MemEvent *ev = new MemEvent(this, block->baseAddr, SupplyData);
        ev->setFlag(MemEvent::F_WRITEBACK);
        ev->setPayload(block->data);
		snoopBusQueue.request(ev, BusHandlers(&Cache::prepWritebackBlock, &Cache::finishWritebackBlockVA, args));
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

    /* TODO:  Verify that block->data can't change between snoop write and now */

	if ( downstream_link ) {
        MemEvent *ev = new MemEvent(this, block->baseAddr, SupplyData);
        ev->setFlag(MemEvent::F_WRITEBACK);
        ev->setPayload(block->data);
		downstream_link->send(ev);
    }
	if ( directory_link ) {
        MemEvent *ev = new MemEvent(this, block->baseAddr, SupplyData);
        ev->setFlag(MemEvent::F_WRITEBACK);
        ev->setDst(findTargetDirectory(block->baseAddr));
        ev->setPayload(block->data);
		directory_link->send(ev);
    }


    dbg.output(CALL_INFO, "Wrote Back Block 0x%"PRIx64"\tNew Status: %d\n", block->baseAddr, newStatus);

    CacheRow *row = findRow(block->baseAddr);
	block->status = newStatus;
    block->last_touched = getCurrentSimTime();
    dbg.output(CALL_INFO, "Marking block 0x%"PRIx64" with status %d\n", block->baseAddr, block->status);

    if ( CacheBlock::INVALID == newStatus ) {
        assert(!block->isLocked());
        block = NULL;
    }

    handlePendingEvents(row, block);
}





void Cache::handleFetch(MemEvent *ev, bool invalidate, bool hasInvalidated)
{
    CacheBlock *block = findBlock(ev->getAddr(), false);
    /* Fetches should only come from directory controllers that know we have the block */
    assert(directory_link);
    if ( !block ) {
        dbg.output(CALL_INFO, "We were asked for 0x%"PRIx64", but we don't have it.  Punting.  Hope we recently did a return of it.\n", ev->getAddr());
        delete ev;
        return;
    }

    dbg.output(CALL_INFO, "0x%"PRIx64" block status: %d\n", block->baseAddr, block->status);

    if ( waitingForInvalidate(block->baseAddr) ) {
        dbg.output(CALL_INFO, "Fetch for 0x%"PRIx64", but Invalidation in progress.\n", ev->getAddr());
        invalidations[block->baseAddr].waitingEvents.push_back(std::make_pair(ev, DIRECTORY));
        return;
    }

    if ( block->wb_in_progress || supplyInProgress(block->baseAddr, DIRECTORY) ) {
        dbg.output(CALL_INFO, "Fetch for 0x%"PRIx64", but WB or supply already in progress.\n", ev->getAddr());
        delete ev;
        return;
    }

    if ( invalidate && !hasInvalidated ) {
        dbg.output(CALL_INFO, "Issuing invalidation for 0x%"PRIx64" upstream.\n", block->baseAddr);
        issueInvalidate(ev, DIRECTORY, block, CacheBlock::SHARED, SEND_UP);
        return;
    }

    switch ( block->status ) {
    case CacheBlock::EXCLUSIVE:
    case CacheBlock::DIRTY_PRESENT:
        block->status = CacheBlock::SHARED;
        /* Fall through to shared state. */
    case CacheBlock::SHARED: {
        MemEvent *me = ev->makeResponse(this);
        me->setDst(ev->getSrc());
        me->setPayload(block->data);
        directory_link->send(me);
        block->last_touched = getCurrentSimTime();
        delete ev;
        break;
    }
    case CacheBlock::DIRTY_UPSTREAM:
        fetchBlock(ev, block, DIRECTORY);
        return; // Can't invalidate yet.
        break;
    default:
        _abort(Cache, "%d Not a legal status in a Fetch situation.\n", block->status);
    }

    if ( invalidate ) {
        block->status = CacheBlock::INVALID;
        dbg.output(CALL_INFO, "Marking block 0x%"PRIx64" with status %d\n", block->baseAddr, block->status);
    }

}


/* Fetch a dirty block from upstream */
void Cache::fetchBlock(MemEvent *ev, CacheBlock *block, SourceType_t src)
{
    std::pair<LoadInfo_t*, bool> initRes = initLoad(block->baseAddr, ev, src);
    LoadInfo_t *li = initRes.first;
    bool reprocess = !initRes.second;

    li->targetBlock = block;
    li->loadDirection = SEND_UP;
    block->loadInfo = li;
    block->lock();

    if ( reprocess )
        li->list.push_front(LoadInfo_t::LoadElement_t(ev, src, getCurrentSimTime()));
    else
        li->list.push_back(LoadInfo_t::LoadElement_t(ev, src, getCurrentSimTime()));

    li->eventScheduled = true;
    self_link->send(1, new SelfEvent(this, &Cache::finishLoadBlock, li, block->baseAddr, block));
}


void Cache::handleNACK(MemEvent *ev, SourceType_t src)
{
    if ( ev->getDst() != getName() ) {
        /* This isn't to us. */
        delete ev;
        return;
    }

    /* Check the various queues to see what was NACK'd
     * Invalidations
     * Waiting Loads
     */

    std::map<Addr, Invalidation>::iterator i = invalidations.find(ev->getAddr());
    if ( invalidations.end() != i ) {
        dbg.output(CALL_INFO, "NACK for Invalidation of 0x%"PRIx64" (%"PRIu64", %d)\n", ev->getAddr(), i->second.issuingEvent.first, i->second.issuingEvent.second );
        if (ev->getResponseToID() == i->second.issuingEvent) {
            assert(i->second.canCancel);
            std::deque<std::pair<MemEvent*, SourceType_t> > waitingEvents = i->second.waitingEvents;
            invalidations.erase(i);
            while ( waitingEvents.size() > 0 ) {
                std::pair<MemEvent*, SourceType_t> oldEV = waitingEvents.front();
                waitingEvents.pop_front();

                CacheBlock *block = findBlock(ev->getAddr(), false);
                if ( block && block->isLocked() ) block->unlock();

                if ( isL1 ) {
                    /* Can't propagate the NACK any higher, just re-issue */
                    dbg.output(CALL_INFO, "Rescheduling event (%"PRIu64", %d) %s 0x%"PRIx64".\n",
                            oldEV.first->getID().first, oldEV.first->getID().second,
                            CommandString[oldEV.first->getCmd()], oldEV.first->getAddr());
                    self_link->send(1, new SelfEvent(this, &Cache::retryEvent, oldEV.first, NULL, oldEV.second));
                } else {
                    respondNACK(oldEV.first, oldEV.second);
                    delete oldEV.first;
                }
            }
            delete ev;
            return;
        } else {
            dbg.output(CALL_INFO, "NACK for Invalidation of 0x%"PRIx64" does not match request id.  Passing.\n", ev->getAddr());
        }
    }


	LoadList_t::iterator li = waitingLoads.find(ev->getAddr());
    if ( waitingLoads.end() != li ) {
        if ( li->second->loadingEvent == ev->getResponseToID() ) {
            if ( !li->second->nackRescheduled ) {
                dbg.output(CALL_INFO, "NACK for RequestData of 0x%"PRIx64".  Rescheduling load.\n", ev->getAddr());
                CacheBlock *block = li->second->targetBlock;
                //block->unlock();
                li->second->eventScheduled = true;
                li->second->nackRescheduled = true;
                self_link->send(1, new SelfEvent(this, &Cache::finishLoadBlock, li->second, block->baseAddr, block));
            } else {
                dbg.output(CALL_INFO, "NACK for RequestData of 0x%"PRIx64".  NACK already rescheduled.\n", ev->getAddr());
            }
            delete ev;
            return;
        } else {
            dbg.output(CALL_INFO, "NACK for RequestData of 0x%"PRIx64" is for request (%"PRIu64", %d).  Our request is (%"PRIu64", %d).  Ignorning.\n",
                    ev->getAddr(), ev->getResponseToID().first, ev->getResponseToID().second,
                    li->second->loadingEvent.first, li->second->loadingEvent.second);
        }

    }

    dbg.output(CALL_INFO, "Unexpected NACK for 0x%"PRIx64" received.  Ignoring.\n", ev->getAddr());

    delete ev;
}


void Cache::respondNACK(MemEvent *ev, SourceType_t src)
{
    MemEvent *nack = ev->makeResponse(this);
    nack->setCmd(NACK);
    switch ( src ) {
    case  SNOOP:
        dbg.output(CALL_INFO, "Sending NACK for %s 0x%"PRIx64" on bus to %s\n", CommandString[ev->getCmd()], ev->getAddr(), nack->getDst().c_str());
        snoopBusQueue.request(nack);
        break;
    case UPSTREAM:
        dbg.output(CALL_INFO, "Sending NACK for %s 0x%"PRIx64" upstream\n", CommandString[ev->getCmd()], ev->getAddr());
        upstream_links[upstreamLinkMap[ev->getLinkId()]]->send(nack);
        break;
    case DOWNSTREAM:
        dbg.output(CALL_INFO, "Sending NACK for %s 0x%"PRIx64" downstream\n", CommandString[ev->getCmd()], ev->getAddr());
        downstream_link->send(nack);
        break;
    case DIRECTORY:
        assert(false); // Don't send to directory
        break;
    case SELF:
        _abort(Cache, "Shouldn't happen... NACK'ing an event we sent ourself?\n");
        break;
    case PREFETCHER:
        _abort(Cache, "Check this:  Trying to send NACK to PREFETCHER.\n");
        break;
    }
}



void Cache::handleUncachedWrite(MemEvent *ev, SourceType_t src)
{
    MemEvent *newev = new MemEvent(this, ev->getAddr(), SupplyData);
    newev->setPayload(ev->getPayload());
    newev->setFlag(MemEvent::F_UNCACHED);
    newev->setFlag(MemEvent::F_WRITEBACK);

    // Send on towards destination
    switch (src) {
    case UPSTREAM:
        if ( downstream_link ) {
            dbg.output(CALL_INFO, "UNCACHED Write 0x%"PRIx64" from UPSTREAM forwarding DOWNSTREAM\n", ev->getAddr());
            downstream_link->send(newev);
        } else if ( directory_link ) {
            dbg.output(CALL_INFO, "UNCACHED Write 0x%"PRIx64" from UPSTREAM forwarding DIRECTORY\n", ev->getAddr());
            newev->setDst(findTargetDirectory(ev->getAddr()));
            directory_link->send(newev);
        } else if ( snoop_link ) {
            dbg.output(CALL_INFO, "UNCACHED Write 0x%"PRIx64" from UPSTREAM forwarding SNOOP\n", ev->getAddr());
            snoopBusQueue.request(newev);
        } else {
            _abort(Cache, "Don't know where to send Uncached Write.\n");
        }
        break;
    case SNOOP:
        if ( downstream_link ) {
            dbg.output(CALL_INFO, "UNCACHED Write 0x%"PRIx64" from SNOOP forwarding DOWNSTREAM\n", ev->getAddr());
            downstream_link->send(newev);
        } else if ( directory_link ) {
            dbg.output(CALL_INFO, "UNCACHED Write 0x%"PRIx64" from SNOOP forwarding DIRECTORY\n", ev->getAddr());
            newev->setDst(findTargetDirectory(ev->getAddr()));
            directory_link->send(newev);
        } else {
            dbg.output(CALL_INFO, "UNCACHED Write 0x%"PRIx64" from SNOOP discarding\n", ev->getAddr());
            // We can't forward this on.  Exit early.
            delete newev;
            return;
        }
        break;
    default:
        _abort(Cache, "Unexpected Uncached %s coming over %d\n", CommandString[ev->getCmd()], src);
    }

    outstandingWrites[newev->getID()] = std::make_pair(ev, src);
}


void Cache::handleWriteResp(MemEvent *ev, SourceType_t src)
{
    MemEvent::id_type srcID = ev->getResponseToID();
    std::map<MemEvent::id_type, std::pair<MemEvent*, SourceType_t> >::iterator i = outstandingWrites.find(srcID);
    if ( outstandingWrites.end() != i ) {
        MemEvent *origEv = i->second.first;
        SourceType_t origSrc = i->second.second;
        outstandingWrites.erase(i);

        dbg.output(CALL_INFO, "Matched WriteResp to orig event %s 0x%"PRIx64" (%"PRIu64", %d) [from %d]\n",
                CommandString[origEv->getCmd()], origEv->getAddr(), origEv->getID().first, origEv->getID().second, origSrc);


        MemEvent *resp = origEv->makeResponse(this);
        switch (origSrc) {
        case UPSTREAM:
            upstream_links[upstreamLinkMap[origEv->getLinkId()]]->send(resp);
            break;
        case SNOOP:
            snoopBusQueue.request(resp);
            break;
        default:
            _abort(Cache, "Expected WriteResp for command coming from %d, which is unexpected\n", origSrc);
        }

        delete origEv;
    }

    delete ev;
}


/**** Utility Functions ****/

void Cache::handlePendingEvents(CacheRow *row, CacheBlock *block)
{
    if ( row->waitingEvents.size() > 0 ) {

        std::map<Addr, CacheRow::eventQueue_t>::iterator q = row->waitingEvents.end();
        if ( block ) {
            q = row->waitingEvents.find(block->baseAddr);
        }

        /* Pick one, if we don't have a specific queue. */
        if ( row->waitingEvents.end() == q ) {
            q = row->waitingEvents.begin();
        }

        if ( row->waitingEvents.end() != q ) {
            CacheRow::eventQueue_t &queue = q->second;
            while ( queue.size() ) {
                std::pair<MemEvent*, SourceType_t> ev = queue.front();
                queue.pop_front();
                dbg.output(CALL_INFO, "Issuing Retry for event (%"PRIu64", %d) %s [0x%"PRIx64"]\n",
                        ev.first->getID().first, ev.first->getID().second, CommandString[ev.first->getCmd()], ev.first->getAddr());
                self_link->send(1, new SelfEvent(this, &Cache::retryEvent, ev.first, NULL, ev.second));
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
    dbg.output(CALL_INFO, "Updating block 0x%"PRIx64" %s\n", block->baseAddr, buffer);
#else
    dbg.output(CALL_INFO, "Updating block 0x%"PRIx64"\n", block->baseAddr);
#endif
	block->last_touched = getCurrentSimTime();
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
	return addr & ~((uint64_t)blocksize-1);
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


bool Cache::supplyInProgress(Addr addr, SourceType_t src)
{
    uint32_t numInProgress = 0;
    supplyMap_t::key_type supplyMapKey = std::make_pair(addr, src);
    std::pair<supplyMap_t::iterator, supplyMap_t::iterator> range = suppliesInProgress.equal_range(supplyMapKey);
    for ( supplyMap_t::iterator i = range.first ; i != range.second ; ++i ) {
        if ( false == i->second.canceled ) numInProgress++;
    }

    /* Safety check:  Should only ever have 0 or 1 in progress. */
    assert(numInProgress <= 1);

    return (1 == numInProgress);
}


Cache::supplyMap_t::iterator Cache::getSupplyInProgress(Addr addr, SourceType_t src)
{
    supplyMap_t::key_type supplyMapKey = std::make_pair(addr, src);
    std::pair<supplyMap_t::iterator, supplyMap_t::iterator> range = suppliesInProgress.equal_range(supplyMapKey);
    for ( supplyMap_t::iterator i = range.first ; i != range.second ; ++i ) {
        if ( false == i->second.canceled ) return i;
    }

    return suppliesInProgress.end();
}


Cache::supplyMap_t::iterator Cache::getSupplyInProgress(Addr addr, SourceType_t src, MemEvent::id_type id)
{
    supplyMap_t::key_type supplyMapKey = std::make_pair(addr, src);
    std::pair<supplyMap_t::iterator, supplyMap_t::iterator> range = suppliesInProgress.equal_range(supplyMapKey);
    for ( supplyMap_t::iterator i = range.first ; i != range.second ; ++i ) {
        if ( i->second.initiatingEvent->getID() == id ) return i;
    }

    return suppliesInProgress.end();
}



std::string Cache::findTargetDirectory(Addr addr)
{
    for ( std::vector<MemNIC::ComponentInfo>::iterator i = directories.begin() ; i != directories.end() ; ++i ) {
        MemNIC::ComponentTypeInfo &di = i->typeInfo;
        dbg.output(CALL_INFO, "Comparing address 0x%"PRIx64" to %s [0x%"PRIx64" - 0x%"PRIx64" by 0x%"PRIx64", 0x%"PRIx64"]\n",
                addr, i->name.c_str(), di.dirctrl.rangeStart, di.dirctrl.rangeEnd, di.dirctrl.interleaveStep, di.dirctrl.interleaveSize);
        if ( addr >= di.dirctrl.rangeStart && addr < di.dirctrl.rangeEnd ) {
            if ( 0 == di.dirctrl.interleaveSize ) {
                return i->name;
            } else {
                Addr temp = addr - di.dirctrl.rangeStart;
                Addr offset = temp % di.dirctrl.interleaveStep;
                if ( offset < di.dirctrl.interleaveSize ) {
                    return i->name;
                }
            }
        }
    }
    _abort(Cache, "Unable to find directory for address 0x%"PRIx64"\n", addr);
    return "";
}



void Cache::printCache(Output &out)
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
    if ( !waitingLoads.empty() ) {
        ss << "Waiting Loads\n";
        for ( LoadList_t::iterator i = waitingLoads.begin() ; i != waitingLoads.end() ; ++i ) {
            Addr addr = i->first;
            LoadInfo_t* li = i->second;

            ss << "0x" << std::setw(4) << std::hex << addr << std::dec;
            if ( NULL != li->targetBlock )
                ss << " slated for [" << li->targetBlock->row << ", " << li->targetBlock->col << "]";
            ss << "\n";

            for ( std::deque<LoadInfo_t::LoadElement_t>::iterator j = li->list.begin() ; j != li->list.end() ; ++j ) {
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

    out.output(ss.str().c_str());
}





/************************ Cache::CacheRow ************************/

Cache::CacheBlock* Cache::CacheRow::getLRU(void)
{
    SimTime_t t = (SimTime_t)-1;
    int lru = -1;
    for ( int i = 0 ; i < cache->n_ways ; i++ ) {
        if ( blocks[i].isAssigned() ) continue; // Assigned, waiting for incoming
        if (  0 < blocks[i].locked ) continue; // Currently in use
        if ( NULL != blocks[i].loadInfo ) continue; // Assigned, but other work is being done

        if ( !blocks[i].isValid() ) {
            lru = i;
            break;
        }
        if ( blocks[i].last_touched <= t ) {
            t = blocks[i].last_touched;
            lru = i;
        }
    }
    if ( lru < 0 ) {
        cache->dbg.output(CALL_INFO, "No empty slot in this row.\n");
        return NULL;
    }

    cache->dbg.output(CALL_INFO, "Row LRU is block %p [0x%"PRIx64", [%d.%d]].\n", &blocks[lru], blocks[lru].baseAddr, blocks[lru].status, blocks[lru].user_locked);
    return &blocks[lru];
}


void Cache::CacheRow::addWaitingEvent(MemEvent *ev, SourceType_t src)
{
    Addr blockaddr = cache->addrToBlockAddr(ev->getAddr());
    waitingEvents[blockaddr].push_back(std::make_pair(ev, src));
    cache->dbg.output(CALL_INFO, "Event is number %zu in queue for this row.\n",
            waitingEvents[blockaddr].size());
    printRow();
}


void Cache::CacheRow::printRow(void)
{
    for ( int i = 0 ; i < cache->n_ways ; i++ ) {
        cache->dbg.output(CALL_INFO, "\t\tBlock [0x%"PRIx64" [%d.%d]] is: %s\n",
                blocks[i].baseAddr,
                blocks[i].status, blocks[i].user_locked,
                blocks[i].isAssigned() ?
                "Assigned" :
                (blocks[i].locked>0) ?
                "Locked!":
                "Open."
             );
    }
}



/************************ Cache::BusQueue ************************/

void Cache::BusQueue::init(const std::string &infoStr)
{
    std::istringstream is(infoStr);
    std::string header;
    is >> header;
    assert(!header.compare(Bus::BUS_INFO_STR));
    std::string tag;
    is >> tag;
    if ( !tag.compare("NumACKPeers:") ) {
        is >> numPeers;
    } else {
        numPeers = 1;
    }
}


void Cache::BusQueue::setup(Cache *_comp, SST::Link *_link)
{
    comp = _comp;
    link = _link;
}

void Cache::BusQueue::request(MemEvent *event, BusHandlers handlers)
{
    queue.push_back(event);
    comp->dbg.output(CALL_INFO, "%s: Queued event (%"PRIu64", %d) in position %zu!\n", comp->getName().c_str(), event->getID().first, event->getID().second, queue.size());
    map[event] = handlers;
    link->send(new BusEvent(BusEvent::RequestBus, makeBusKey(event)));
}


bool Cache::BusQueue::cancelRequest(MemEvent *event)
{
    bool retval = false;
    queue.remove(event);
    std::map<MemEvent*, BusHandlers>::iterator i = map.find(event);
    if ( map.end() != i ) {
        map.erase(i);
        link->send(new BusEvent(BusEvent::CancelRequest, makeBusKey(event)));
        comp->dbg.output(CALL_INFO, "%s: Sending cancel for req (%"PRIu64", %d)\n", comp->getName().c_str(), event->getID().first, event->getID().second);
        retval = true;
    } else {
        comp->dbg.output(CALL_INFO, "%s: Unable to find a request to cancel!\n", comp->getName().c_str());
    }
    return retval;
}

void Cache::BusQueue::clearToSend(BusEvent *busEvent)
{
    if ( 0 == size() ) {
        comp->dbg.output(CALL_INFO, "%s: No Requests to send!\n", comp->getName().c_str());
        /* Must have canceled the request */
        link->send(new BusEvent(BusEvent::CancelRequest, busEvent->getKey()));
    } else {
        MemEvent *ev = queue.front();
        if ( busEvent->getKey() != makeBusKey(ev) ) {
            comp->dbg.output(CALL_INFO, "%s: Bus asking for event (%"PRIu64", %d).  That's not top of queue.  Perhaps we canceled it, and the cancels crossed in mid-flight.\n", comp->getName().c_str(), busEvent->getKey().first, busEvent->getKey().second);
            return;
        }
        queue.pop_front();

        comp->dbg.output(CALL_INFO, "%s: Sending Event (%s, 0x%"PRIx64" (%"PRIu64", %d)) [Queue: %zu]!\n", comp->getName().c_str(), CommandString[ev->getCmd()], ev->getAddr(), ev->getID().first, ev->getID().second, queue.size());


        BusHandlers bh;
        std::map<MemEvent*, BusHandlers>::iterator i = map.find(ev);
        if ( map.end() != i ) {
            bh = i->second;
            map.erase(i);
        }

        if ( bh.init ) {
            (comp->*(bh.init))(bh.args, ev);
        }

        link->send(new BusEvent(ev));

        if ( bh.finish ) {
            (comp->*(bh.finish))(bh.args);
        }
    }
}


void Cache::BusQueue::printStatus(Output &out)
{
    out.output("\tBus Queue Size: %zu\n", queue.size());
    for ( std::list<MemEvent*>::iterator i = queue.begin() ; i != queue.end() ; ++i ) {
        out.output("\t\t(%"PRIu64", %d)\n", (*i)->getID().first, (*i)->getID().second);
    }
}


void Cache::registerNewCPURequest(MemEvent::id_type id)
{
    responseTimes[id] = getCurrentSimTimeNano();
}


void Cache::clearCPURequest(MemEvent::id_type id)
{
    std::map<MemEvent::id_type, SimTime_t>::iterator i = responseTimes.find(id);
    if ( responseTimes.end() == i ) {
        _abort(Cache, "Responding to event (%"PRIu64", %d), which isn't in the queue.\n", id.first, id.second);
    }
    SimTime_t elapsed = getCurrentSimTimeNano() - i->second;
    latStats.insertLatency(elapsed);
    responseTimes.erase(i);
}


void Cache::LatencyStats::insertLatency(SimTime_t lat)
{
    numPkts++;
    if ( 1 == numPkts ) {
        minLat = lat;
        maxLat = lat;
        m_n = m_old = lat;
        s_old = 0.0;
    } else {
        minLat = std::min(minLat, lat);
        maxLat = std::max(maxLat, lat);
        m_n = m_old + (lat - m_old) / numPkts;
        s_n = s_old + (lat - m_old) * (lat - m_n);

        m_old = m_n;
        s_old = s_n;
    }
}
