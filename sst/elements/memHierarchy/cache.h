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

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>

#include <sst/core/interfaces/memEvent.h>
using namespace SST::Interfaces;

namespace SST {
namespace MemHierarchy {

class Cache : public SST::Component {

private:
	typedef enum {DOWNSTREAM, SNOOP, DIRECTORY, UPSTREAM, SELF} SourceType_t;

	class Request;
	class CacheRow;
	class CacheBlock;
	class SelfEvent;
	struct LoadInfo_t;
	typedef std::map<Addr, LoadInfo_t*> LoadList_t;


	class CacheBlock {
	public:
		/* Flags */
		typedef enum {INVALID, ASSIGNED, SHARED, EXCLUSIVE} BlockStatus;

		Addr tag;
		Addr baseAddr;
		SimTime_t last_touched;
		BlockStatus status;
		Cache *cache;
		std::vector<uint8_t> data;
		uint32_t locked;

        LoadInfo_t *loadInfo;
        std::deque<MemEvent*> blockedEvents;
		uint16_t row, col;
        bool wb_in_progress;
        bool user_lock_needs_wb;
        uint16_t user_locked;
        int16_t invalidate_acks;
        std::pair<MemEvent*, MemEvent*> invalidateEvent;
        /* ^^^^
         * .first == event that caused the invalidation (retry event)
         * .second = bus event to cause invalidate in others.
         * */

		CacheBlock() {}
		CacheBlock(Cache *_cache)
		{
			tag = 0;
			baseAddr = 0;
			last_touched = 0;
			status = INVALID;
			cache = _cache;
			data = std::vector<uint8_t>(cache->blocksize);
			locked = 0;
            wb_in_progress = false;
            user_lock_needs_wb = false;
            user_locked = 0;
            invalidate_acks = 0;
			invalidateEvent = std::pair<MemEvent*, MemEvent*>(NULL, NULL);
		}

		~CacheBlock()
		{ }

		void activate(Addr addr)
		{
			assert(status != ASSIGNED);
			assert(locked == 0);
			tag = cache->addrToTag(addr);
			baseAddr = cache->addrToBlockAddr(addr);
			__DBG( DBG_CACHE, CacheBlock, "%s: Activating block (%u, %u) for Address 0x%lx.\t"
					"baseAddr: 0x%lx  Tag: 0x%lx\n", cache->getName().c_str(), row, col, addr, baseAddr, tag);
			status = ASSIGNED;
		}

		bool isValid(void) const { return (status != INVALID && status != ASSIGNED); }
		bool isInvalid(void) const { return (status == INVALID); }
		bool isAssigned(void) const { return (status == ASSIGNED); }

        void lock() {
            __DBG(DBG_CACHE, CacheBlock, "Locking block %p [0x%lx] (%u, %u) {%d -> %d}\n", this, baseAddr, row, col, locked, locked+1);
            locked++;
        }
        void unlock() {
            __DBG(DBG_CACHE, CacheBlock, "UNLocking block %p [0x%lx] (%u, %u) {%d -> %d}\n", this, baseAddr, row, col, locked, locked-1);
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
		//std::deque<std::pair<MemEvent*, SourceType_t> > waitingEvents;
		Cache *cache;

		CacheRow() {}
		CacheRow(Cache *_cache) : cache(_cache)
		{
			blocks = std::vector<CacheBlock>(cache->n_ways, CacheBlock(cache));
		}

		CacheBlock* getLRU(void)
		{
			SimTime_t t = (SimTime_t)-1;
			int lru = -1;
			for ( int i = 0 ; i < cache->n_ways ; i++ ) {
				if ( blocks[i].isAssigned() ) continue; // Assigned, waiting for incoming
				if ( blocks[i].locked > 0 ) continue; // Currently in use

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
				__DBG( DBG_CACHE, CacheRow, "No empty slot in this row.\n");
				return NULL;
			}

			return &blocks[lru];
		}

        void addWaitingEvent(MemEvent *ev, SourceType_t src)
        {
            waitingEvents[cache->addrToBlockAddr(ev->getAddr())].push_back(std::make_pair(ev, src));
            __DBG( DBG_CACHE, CacheRow, "Event is number %zu in queue for this row.\n",
                    waitingEvents[cache->addrToBlockAddr(ev->getAddr())].size());
			for ( int i = 0 ; i < cache->n_ways ; i++ ) {
                __DBG( DBG_CACHE, CacheRow, "\t\tBlock [0x%lx] is: %s\n",
                        blocks[i].baseAddr,
                        blocks[i].isAssigned() ?
                            "Assigned" :
                            (blocks[i].locked>0) ?
                                "Locked!":
                                "Open."
                     );
            }

        }
	};


	typedef union {
		struct {
			CacheBlock *block;
			CacheBlock::BlockStatus newStatus;
			bool decrementLock;
		} writebackBlock;
		struct {
			CacheBlock *block;
			SourceType_t src;
            bool isFakeSupply;
		} supplyData;
		struct {
			LoadInfo_t *loadInfo;
		} loadBlock;
	} BusHandlerArgs;
	typedef void(Cache::*BusFinishHandlerFunc)(BusHandlerArgs &args);

	class BusFinishHandler {
	public:
		BusFinishHandler(BusFinishHandlerFunc func, BusHandlerArgs args) :
			func(func), args(args)
		{ }

		BusFinishHandlerFunc func;
		BusHandlerArgs args;
	};

    typedef void(Cache::*BusInitHandlerFunc)(BusHandlerArgs &args, MemEvent *ev);
	class BusInitHandler {
	public:
		BusInitHandler(BusInitHandlerFunc func, BusHandlerArgs args) :
			func(func), args(args)
		{ }

		BusInitHandlerFunc func;
		BusHandlerArgs args;
	};
    struct BusHandlers {
        BusInitHandler *init;
        BusFinishHandler *finish;
    };

	class BusQueue {
	public:
		BusQueue(void) :
			comp(NULL), link(NULL)
		{ }

		BusQueue(Cache *comp, SST::Link *link) :
			comp(comp), link(link)
		{ }

		void setup(Cache *_comp, SST::Link *_link) {
			comp = _comp;
			link = _link;
		}

		int size(void) const { return queue.size(); }
		void request(MemEvent *event, BusFinishHandler *finishHandler = NULL, BusInitHandler *initHandler = NULL)
		{
			if ( event ) {
				queue.push_back(event);
				__DBG( DBG_CACHE, BusQueue, "Queued event in position %zu!\n", queue.size());
                BusHandlers bh = {initHandler, finishHandler};
                map[event] = bh;
			}
				link->Send(new MemEvent(comp, NULL, RequestBus));
		}

		BusHandlers cancelRequest(MemEvent *event)
		{
			BusHandlers retval = {0};
			queue.remove(event);
			std::map<MemEvent*, BusHandlers>::iterator i = map.find(event);
			if ( i != map.end() ) {
				retval = i->second;
				map.erase(i);
			} else {
                __DBG( DBG_CACHE, BusQueue, "Unable to find a request to cancel!\n");
            }
			//delete event; // We have responsibility for this event, due to the contract of Link::Send()
			return retval;
		}

		void clearToSend(void)
		{
			if ( size() == 0 ) {
				__DBG( DBG_CACHE, BusQueue, "No Requests to send!\n");
				/* Must have canceled the request */
				link->Send(new MemEvent(comp, NULL, CancelBusRequest));
			} else {
				MemEvent *ev = queue.front();
				queue.pop_front();

				__DBG( DBG_CACHE, BusQueue, "Sending Event (%s, 0x%lx) [Queue: %zu]!\n", CommandString[ev->getCmd()], ev->getAddr(), queue.size());


                BusHandlers bh = {NULL, NULL};
				std::map<MemEvent*, BusHandlers>::iterator i = map.find(ev);
                if ( i != map.end() ) {
                    bh = i->second;
					map.erase(i);
                }

                if ( bh.init ) {
                    (comp->*(bh.init->func))(bh.init->args, ev);
                    delete bh.init;
                }

				link->Send(ev);

                if ( bh.finish ) {
                    (comp->*(bh.finish->func))(bh.finish->args);
                    delete bh.finish;
                }
			}
		}


	private:
		Cache *comp;
		SST::Link *link;
		std::list<MemEvent*> queue;
		std::map<MemEvent*, BusHandlers> map;

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

public:

	Cache(SST::ComponentId_t id, SST::Component::Params_t& params);
    bool clockTick(Cycle_t);
	void init(unsigned int);
	int Finish();

private:
	void handleIncomingEvent(SST::Event *event, SourceType_t src);
	void handleIncomingEvent(SST::Event *event, SourceType_t src, bool firstTimeProcessed);
	void handleSelfEvent(SST::Event *event);
	void retryEvent(MemEvent *ev, CacheBlock *block, SourceType_t src);

	void handleCPURequest(MemEvent *ev, bool firstProcess);
	MemEvent* makeCPUResponse(MemEvent *ev, CacheBlock *block, SourceType_t src);
	void sendCPUResponse(MemEvent *ev, CacheBlock *block, SourceType_t src);

	void issueInvalidate(MemEvent *ev, CacheBlock *block);
	void loadBlock(MemEvent *ev, SourceType_t src);
    /* Note, this MemEvent* is actually a LoadInfo_t* */
	void finishLoadBlock(LoadInfo_t *li, Addr addr, CacheBlock *block);
	void finishLoadBlockBus(BusHandlerArgs &args);

	void handleCacheRequestEvent(MemEvent *ev, SourceType_t src, bool firstProcess);
	void supplyData(MemEvent *ev, CacheBlock *block, SourceType_t src);
	void finishBusSupplyData(BusHandlerArgs &args);
	void handleCacheSupplyEvent(MemEvent *ev, SourceType_t src);
	void finishSupplyEvent(MemEvent *origEV, CacheBlock *block, SourceType_t src);
	void handleInvalidate(MemEvent *ev, SourceType_t src);

	bool waitingForInvalidate(CacheBlock *block);
	void cancelInvalidate(CacheBlock *block);
    void ackInvalidate(MemEvent *ev);
	void finishIssueInvalidate(CacheBlock *block);

	void writebackBlock(CacheBlock *block, CacheBlock::BlockStatus newStatus);
    void prepWritebackBlock(BusHandlerArgs &args, MemEvent *ev);
	void finishWritebackBlockVA(BusHandlerArgs &args);
	void finishWritebackBlock(CacheBlock *block, CacheBlock::BlockStatus newStatus, bool decrementLock);

	void busClear(SST::Link *busLink);

    void handlePendingEvents(CacheRow *row, CacheBlock *block);
	void updateBlock(MemEvent *ev, CacheBlock *block);
	SST::Link *getLink(SourceType_t type, int link_id);
	int numBits(int x);
	Addr addrToTag(Addr addr);
	Addr addrToBlockAddr(Addr addr);
	CacheBlock* findBlock(Addr addr, bool emptyOK = false);
	CacheRow* findRow(Addr addr);

	void printCache(void);

	int n_ways;
	int n_rows;
	uint32_t blocksize;
	std::string access_time;
	std::vector<CacheRow> database;
	std::string next_level_name;

	int rowshift;
	unsigned rowmask;
	int tagshift;

	int n_upstream;
	SST::Link *snoop_link; // Points to a snoopy bus, or snoopy network (if any)
	SST::Link *directory_link; // Points to a network for directory lookups (if any)
	SST::Link **upstream_links; // Points to directly upstream caches or cpus (if any) [no snooping]
	SST::Link *downstream_link; // Points to directly downstream cache (if any)
	SST::Link *self_link; // Used for scheduling access
	std::map<LinkId_t, int> upstreamLinkMap;

	/* Stats */
	uint64_t num_read_hit;
	uint64_t num_read_miss;
	uint64_t num_supply_hit;
	uint64_t num_supply_miss;
	uint64_t num_write_hit;
	uint64_t num_write_miss;
	uint64_t num_upgrade_miss;

	struct LoadInfo_t {
		Addr addr;
		CacheBlock *targetBlock;
		MemEvent *busEvent;
        MemEvent::id_type initiatingEvent;
		struct LoadElement_t {
			MemEvent * ev;
			SourceType_t src;
			SimTime_t issueTime;
			LoadElement_t(MemEvent *ev, SourceType_t src, SimTime_t issueTime) :
				ev(ev), src(src), issueTime(issueTime)
			{ }
		};
		std::deque<LoadElement_t> list;
		LoadInfo_t() : addr(0), targetBlock(NULL), busEvent(NULL) { }
		LoadInfo_t(Addr addr) : addr(addr), targetBlock(NULL), busEvent(NULL) { }
	};
	LoadList_t waitingLoads;


	struct SupplyInfo {
		MemEvent *busEvent;
		bool canceled;
		SupplyInfo() : busEvent(NULL), canceled(false) { }
		SupplyInfo(MemEvent *event) : busEvent(event), canceled(false) { }
	};
	// Map from <addr, from where req came> to SupplyInfo
	typedef std::map<std::pair<Addr, SourceType_t>, SupplyInfo> supplyMap_t;
	supplyMap_t supplyInProgress;

	BusQueue snoopBusQueue;

};

}
}

#endif
