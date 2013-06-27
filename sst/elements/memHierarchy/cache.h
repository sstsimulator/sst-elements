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

#ifndef _CACHE_H
#define _CACHE_H

#include <deque>
#include <map>
#include <list>

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include <sst/core/output.h>

#include <sst/core/interfaces/memEvent.h>

#include "memNIC.h"


using namespace SST::Interfaces;

namespace SST {
namespace MemHierarchy {

class CacheListener;

class Cache : public SST::Component {

private:
	typedef enum {DOWNSTREAM, SNOOP, DIRECTORY, UPSTREAM, SELF, PREFETCHER} SourceType_t;
	typedef enum {INCLUSIVE, EXCLUSIVE, STANDARD} CacheMode_t;
    typedef enum {SEND_DOWN, SEND_UP, SEND_BOTH} ForwardDir_t;

	class Request;
	class CacheRow;
	class CacheBlock;
	class SelfEvent;
	struct LoadInfo_t;
	typedef std::map<Addr, LoadInfo_t*> LoadList_t;


	class CacheBlock {
	public:
		/* Flags */
		typedef enum {
            INVALID,        // Nothing is here
            ASSIGNED,       // Reserved status, waiting on a load
            SHARED,         // Others may have an up to date copy
            EXCLUSIVE,      // Only in this cache, data dirty
            DIRTY_UPSTREAM, // for Inclusive mode, data not here, dirty/exclusive upstream
            DIRTY_PRESENT,  // for Inclusive mode, data here, modified
        } BlockStatus;

		Addr tag;
		Addr baseAddr;
		SimTime_t last_touched;
		BlockStatus status;
		Cache *cache;
		std::vector<uint8_t> data;
		uint32_t locked;

        LoadInfo_t *loadInfo;
		uint16_t row, col;
        bool wb_in_progress;
        bool user_lock_needs_wb;
        bool user_lock_sent_delayed;
        uint16_t user_locked;

		CacheBlock() {}
		CacheBlock(Cache *_cache) :
            tag(0), baseAddr(0), last_touched(0), status(INVALID),
            cache(_cache), data(std::vector<uint8_t>(cache->blocksize)),
            locked(0), loadInfo(NULL), wb_in_progress(false),
            user_lock_needs_wb(false), user_lock_sent_delayed(false),
            user_locked(0)
		{ }

		~CacheBlock()
		{ }

		void activate(Addr addr)
		{
			assert(status != ASSIGNED);
			assert(locked == 0);
			tag = cache->addrToTag(addr);
			baseAddr = cache->addrToBlockAddr(addr);
            cache->dbg.output(CALL_INFO, "CacheBlock:  %s: Activating block (%u, %u) for Address 1x%"PRIx64".\t"
					"baseAddr: 0x%"PRIx64"  Tag: 0x%"PRIx64"\n", cache->getName().c_str(), row, col, addr, baseAddr, tag);
			status = ASSIGNED;
		}

		bool isValid(void) const { return (status != INVALID && status != ASSIGNED); }
		bool isInvalid(void) const { return (status == INVALID); }
		bool isAssigned(void) const { return (status == ASSIGNED); }
        bool isDirty(void) const { return (status == DIRTY_UPSTREAM || status == DIRTY_PRESENT || status == EXCLUSIVE); }

        void lock() {
            cache->dbg.output(CALL_INFO, "CacheBlock:  Locking block %p [0x%"PRIx64"] (%u, %u) {%d -> %d}\n", this, baseAddr, row, col, locked, locked+1);
            locked++;
        }
        void unlock() {
            cache->dbg.output(CALL_INFO, "CacheBlock:  UNLocking block %p [0x%"PRIx64"] (%u, %u) {%d -> %d}\n", this, baseAddr, row, col, locked, locked-1);
            assert(locked);
            locked--;
        }
        bool isLocked() const {
            return locked > 0;
        }
	};

	class CacheRow {
	public:
		std::vector<CacheBlock> blocks;
        typedef std::deque<std::pair<MemEvent*, SourceType_t> > eventQueue_t;
        std::map<Addr, eventQueue_t> waitingEvents;
		Cache *cache;

		CacheRow() {}
		CacheRow(Cache *_cache) : cache(_cache)
		{
			blocks = std::vector<CacheBlock>(cache->n_ways, CacheBlock(cache));
		}

		CacheBlock* getLRU(void);

        void addWaitingEvent(MemEvent *ev, SourceType_t src);

        void printRow(void);
	};


    struct Invalidation {
        int waitingACKs;
        std::deque<std::pair<MemEvent*, SourceType_t> > waitingEvents;
        MemEvent::id_type issuingEvent;
        MemEvent *busEvent; // Snoop Bus event (for cancelation)
        CacheBlock *block;  // Optional
        CacheBlock::BlockStatus newStatus; // Optional
        bool canCancel;

        Invalidation() :
            waitingACKs(0), issuingEvent(0,-1), busEvent(NULL), block(NULL),
            newStatus(CacheBlock::INVALID), canCancel(true)
        { }
    };


	typedef union {
		struct {
			CacheBlock *block;
			CacheBlock::BlockStatus newStatus;
			bool decrementLock;
		} writebackBlock;
		struct {
            MemEvent *initiatingEvent;
			CacheBlock *block;
			SourceType_t src;
            bool isFakeSupply;
		} supplyData;
		struct {
			LoadInfo_t *loadInfo;
		} loadBlock;
	} BusHandlerArgs;

	typedef void(Cache::*BusFinishHandlerFunc)(BusHandlerArgs &args);
    typedef void(Cache::*BusInitHandlerFunc)(BusHandlerArgs &args, MemEvent *ev);

    struct BusHandlers {
        BusInitHandlerFunc init;
        BusFinishHandlerFunc finish;
        BusHandlerArgs args;
        BusHandlers(void) :
            init(NULL), finish(NULL)
        { }
        BusHandlers(BusInitHandlerFunc bihf, BusFinishHandlerFunc bfhf, BusHandlerArgs & args) :
            init(bihf), finish(bfhf), args(args)
        { }
    };

	class BusQueue {
        uint64_t makeBusKey(MemEvent *ev)
        {
            return ((ev->getID().first & 0xffffffff) << 32) | ev->getID().second;
        }

	public:
		BusQueue(void) :
			comp(NULL), link(NULL), numPeers(0)
		{ }

		BusQueue(Cache *comp, SST::Link *link) :
			comp(comp), link(link), numPeers(0)
		{ }


        void init(const std::string &infoStr);
		void setup(Cache *_comp, SST::Link *_link);

        int getNumPeers(void) const { return numPeers; }
		int size(void) const { return queue.size(); }

		void request(MemEvent *event, BusHandlers handlers = BusHandlers());
		bool cancelRequest(MemEvent *event);
		void clearToSend(MemEvent *busEvent);


	private:
		Cache *comp;
		SST::Link *link;
		std::list<MemEvent*> queue;
		std::map<MemEvent*, BusHandlers> map;
        int numPeers;

	};

	typedef void (Cache::*SelfEventHandler)(MemEvent*, CacheBlock*, SourceType_t);
	typedef void (Cache::*SelfEventHandler2)(LoadInfo_t*, Addr addr, CacheBlock*);
	class SelfEvent : public SST::Event {
	public:
		SelfEvent() {} // For serialization

		SelfEvent(Cache *cache, SelfEventHandler handler, MemEvent *event, CacheBlock *block,
				SourceType_t event_source = SELF) :
			cache(cache), handler(handler), handler2(NULL),
            event(event), block(block), event_source(event_source),
            li(NULL), addr(0)
		{ }

		SelfEvent(Cache *cache, SelfEventHandler2 handler, LoadInfo_t *li, Addr addr, CacheBlock *block) :
			cache(cache), handler(NULL), handler2(handler),
            event(NULL), block(block), event_source(SELF),
            li(li), addr(addr)
		{ }


        void fire(void) {
            if ( handler ) {
                (cache->*(handler))(event, block, event_source);
            } else if ( handler2 ) {
                (cache->*(handler2))(li, addr, block);
            }
        }

        Cache *cache;
		SelfEventHandler handler;
		SelfEventHandler2 handler2;
		MemEvent *event;
		CacheBlock *block;
		SourceType_t event_source;
        LoadInfo_t *li;
        Addr addr;
	};

	struct LoadInfo_t {
		Addr addr;
		CacheBlock *targetBlock;
		MemEvent *busEvent;
        MemEvent::id_type initiatingEvent;
        bool uncached;
        bool satisfied;
        bool eventScheduled; // True if a self-event has been scheduled that will need this (finishLoadBlock)
        ForwardDir_t loadDirection;
		struct LoadElement_t {
			MemEvent * ev;
			SourceType_t src;
			SimTime_t issueTime;
			LoadElement_t(MemEvent *ev, SourceType_t src, SimTime_t issueTime) :
				ev(ev), src(src), issueTime(issueTime)
			{ }
		};
		std::deque<LoadElement_t> list;
		LoadInfo_t() : addr(0), targetBlock(NULL), busEvent(NULL), uncached(false), satisfied(false), eventScheduled(false), loadDirection(SEND_BOTH) { }
		LoadInfo_t(Addr addr) : addr(addr), targetBlock(NULL), busEvent(NULL), uncached(false), satisfied(false), eventScheduled(false), loadDirection(SEND_BOTH) { }
	};

	struct SupplyInfo {
        MemEvent *initiatingEvent;
		MemEvent *busEvent;
		bool canceled;
		SupplyInfo(MemEvent *id) : initiatingEvent(id), busEvent(NULL), canceled(false) { }
	};
	// Map from <addr, from where req came> to SupplyInfo
	typedef std::multimap<std::pair<Addr, SourceType_t>, SupplyInfo> supplyMap_t;



public:

	Cache(SST::ComponentId_t id, SST::Component::Params_t& params);
    bool clockTick(Cycle_t);
	virtual void init(unsigned int);
	virtual void setup();
	virtual void finish();

private:
	void handleIncomingEvent(SST::Event *event, SourceType_t src);
	void handleIncomingEvent(SST::Event *event, SourceType_t src, bool firstTimeProcessed, bool firstPhaseComplete = false);
	void handleSelfEvent(SST::Event *event);
	void retryEvent(MemEvent *ev, CacheBlock *block, SourceType_t src);

    void handlePrefetchEvent(SST::Event *event);

	void handleCPURequest(MemEvent *ev, bool firstProcess);
	MemEvent* makeCPUResponse(MemEvent *ev, CacheBlock *block, SourceType_t src);
	void sendCPUResponse(MemEvent *ev, CacheBlock *block, SourceType_t src);

	void issueInvalidate(MemEvent *ev, SourceType_t src, CacheBlock *block, CacheBlock::BlockStatus newStatus, ForwardDir_t direction, bool cancelable = true);
	void issueInvalidate(MemEvent *ev, SourceType_t src, Addr addr, ForwardDir_t direction, bool cancelable = true);
	void loadBlock(MemEvent *ev, SourceType_t src);
    std::pair<LoadInfo_t*, bool> initLoad(Addr addr, MemEvent *ev, SourceType_t src);
	void finishLoadBlock(LoadInfo_t *li, Addr addr, CacheBlock *block);
	void finishLoadBlockBus(BusHandlerArgs &args);

	void handleCacheRequestEvent(MemEvent *ev, SourceType_t src, bool firstProcess);
	void supplyData(MemEvent *ev, CacheBlock *block, SourceType_t src);
    void prepBusSupplyData(BusHandlerArgs &args, MemEvent *ev);
	void finishBusSupplyData(BusHandlerArgs &args);
	void handleCacheSupplyEvent(MemEvent *ev, SourceType_t src);
	void handleInvalidate(MemEvent *ev, SourceType_t src, bool finishedUpstream);
    void sendInvalidateACK(MemEvent *ev, SourceType_t src);

	bool waitingForInvalidate(Addr addr);
	bool cancelInvalidate(CacheBlock *block);
    void ackInvalidate(MemEvent *ev);
	void finishIssueInvalidate(Addr addr);

	void writebackBlock(CacheBlock *block, CacheBlock::BlockStatus newStatus);
    void prepWritebackBlock(BusHandlerArgs &args, MemEvent *ev);
	void finishWritebackBlockVA(BusHandlerArgs &args);
	void finishWritebackBlock(CacheBlock *block, CacheBlock::BlockStatus newStatus, bool decrementLock);

    void handleFetch(MemEvent *ev, bool invalidate, bool hasInvalidated);
    void fetchBlock(MemEvent *ev, CacheBlock *block, SourceType_t src);

    void handleNACK(MemEvent *ev, SourceType_t src);
    void respondNACK(MemEvent *ev, SourceType_t src);

    void handleUncachedWrite(MemEvent *ev, SourceType_t src);
    void handleWriteResp(MemEvent *ev, SourceType_t src);


    void handlePendingEvents(CacheRow *row, CacheBlock *block);
	void updateBlock(MemEvent *ev, CacheBlock *block);
	int numBits(int x);
	Addr addrToTag(Addr addr);
	Addr addrToBlockAddr(Addr addr);
	CacheBlock* findBlock(Addr addr, bool emptyOK = false);
	CacheRow* findRow(Addr addr);

    bool supplyInProgress(Addr addr, SourceType_t src);
    supplyMap_t::iterator getSupplyInProgress(Addr addr, SourceType_t src);
    supplyMap_t::iterator getSupplyInProgress(Addr addr, SourceType_t src, MemEvent::id_type id);

    std::string findTargetDirectory(Addr addr);

	void printCache(Output &out);




    Output dbg;
    CacheListener* listener;
	int n_ways;
	int n_rows;
	uint32_t blocksize;
	std::string access_time;
	std::vector<CacheRow> database;
	std::string next_level_name;
    CacheMode_t cacheMode;
    bool isL1;

	int rowshift;
	unsigned rowmask;
	int tagshift;

    std::map<Addr, Invalidation> invalidations;
	LoadList_t waitingLoads;
	supplyMap_t suppliesInProgress;
    std::map<MemEvent::id_type, std::pair<MemEvent*, SourceType_t> > outstandingWrites;

	BusQueue snoopBusQueue;

	int n_upstream;
	SST::Link *snoop_link; // Points to a snoopy bus, or snoopy network (if any)
	MemNIC *directory_link; // Points to a network for directory lookups (if any)
	SST::Link **upstream_links; // Points to directly upstream caches or cpus (if any) [no snooping]
	SST::Link *downstream_link; // Points to directly downstream cache (if any)
	SST::Link *self_link; // Used for scheduling access
	std::map<LinkId_t, int> upstreamLinkMap;
    std::vector<MemNIC::ComponentInfo> directories;

	/* Stats */
	uint64_t num_read_hit;
	uint64_t num_read_miss;
	uint64_t num_supply_hit;
	uint64_t num_supply_miss;
	uint64_t num_write_hit;
	uint64_t num_write_miss;
	uint64_t num_upgrade_miss;


};

}
}

#endif
