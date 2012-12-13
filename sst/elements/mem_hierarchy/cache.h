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
#include "memEvent.h"

namespace SST {
namespace MemHierarchy {

class Cache : public SST::Component {
public:

	Cache(SST::ComponentId_t id, SST::Component::Params_t& params);
	int Setup()  { return 0; }
	int Finish() {
		printf("STATS for %s\n"
				"\tnum_read_hit:\t%lu\n"
				"\tnum_read_miss:\t%lu\n"
				"\tnum_write_hit:\t%lu\n"
				"\tnum_write_miss:\t%lu\n"
				"\tnum_snoops_provided:\t%lu\n"
				"\tnum_miss_held:\t%lu\n",
				getName().c_str(),
				num_read_hit, num_read_miss,
				num_write_hit, num_write_miss,
				num_snoops_provided, num_miss_held);
		return 0;
	}

private:
	struct CacheBlock {
		/* Flags */
		typedef enum {INVALID, ASSIGNED, SHARED, EXCLUSIVE} BlockStatus;
		Addr tag;
		Addr baseAddr;
		SimTime_t last_touched;
		BlockStatus status;
		Cache *cache;
		std::vector<uint8_t> data;

		CacheBlock() {}
		CacheBlock(Cache *_cache)
		{
			tag = 0;
			baseAddr = 0;
			last_touched = 0;
			status = INVALID;
			cache = _cache;
			data = std::vector<uint8_t>(cache->blocksize);
			//data = new uint8_t[cache->blocksize];
		}
		~CacheBlock()
		{
			//delete [] data;
		}
		void activate(Addr addr)
		{
			assert(status != ASSIGNED);
			tag = cache->addrToTag(addr);
			baseAddr = cache->addrToBlockAddr(addr);
			__DBG( DBG_CACHE, CacheBlock, "Activating block %p for Address 0x%lx.\t"
					"baseAddr: 0x%lx  Tag: 0x%lx\n", this, addr, baseAddr, tag);
			status = ASSIGNED;
		}

		bool isValid(void) { return (status != INVALID && status != ASSIGNED); }
		bool isAssigned(void) { return (status == ASSIGNED); }
	};

	struct CacheRow {
		std::vector<CacheBlock> blocks;
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
			for ( uint32_t i = 0 ; i < cache->n_ways ; i++ ) {
				if ( blocks[i].isAssigned() ) continue; // Assigned, waiting for incoming

				if ( !blocks[i].isValid() ) {
					lru = i;
					break;
				}
				if ( blocks[i].last_touched <= t ) {
					t = blocks[i].last_touched;
					lru = i;
				}
			}
			assert(lru >= 0); // If this fails, no block available.

			return &blocks[lru];
		}
	};

	struct BusRequest;
	typedef void (Cache::*postSendHandler)(BusRequest *req);

	struct BusRequest {
		BusRequest() {} // For serialization
		BusRequest(MemEvent *_msg, CacheBlock *_block, bool _canCancel,
				postSendHandler _finish = NULL,
				postSendHandler _cancel = NULL,
				void* _ud = NULL) :
			msg(_msg), block(_block), canCancel(_canCancel),
			finishFunc(_finish), cancelFunc(_cancel), ud(_ud)
		{ }

		~BusRequest(void) { }

		MemEvent *msg; /* Assume We're owner of this MemEvent */
		CacheBlock *block;
		bool canCancel;
		postSendHandler finishFunc;
		postSendHandler cancelFunc;
		void *ud;
	};

	class RequestHold {
	public:
		struct RequestList {
			CacheBlock *block;
			std::deque<MemEvent*> requests;

			RequestList(CacheBlock *b) : block(b), requests(std::deque<MemEvent*>()) { }
		};
	private:
		std::map<Addr, RequestList*> reqs;
	public:

		void storeReq(CacheBlock *block, MemEvent *event)
		{
			std::map<Addr, RequestList*>::iterator i = reqs.find(block->baseAddr);
			struct RequestList *req;
			if ( i == reqs.end() ) {
				req = new RequestList(block);
				reqs.insert(std::make_pair(block->baseAddr, req));
			} else {
				req = i->second;
			}
			req->requests.push_back(event);
		}

		void addReq(Addr addr, MemEvent *event)
		{
			std::map<Addr, RequestList*>::iterator i = reqs.find(addr);
			assert ( i != reqs.end() );
			struct RequestList *req = i->second;
			req->requests.push_back(event);
		}

		RequestList* findReqs(Addr addr)
		{
			struct RequestList *req = NULL;
			std::map<Addr, RequestList*>::iterator i = reqs.find(addr);
			if ( i != reqs.end() ) req = i->second;

			return req;
		}

		void clearReqs(Addr addr)
		{
			std::map<Addr, RequestList*>::iterator i = reqs.find(addr);
			if ( i != reqs.end() ) {
				delete i->second;
				reqs.erase(i);
			}
		}

		bool exists(Addr addr) {
			return reqs.find(addr) != reqs.end();
		}

	};


	Cache();  // for serialization only
	Cache(const Cache&); // do not implement
	void operator=(const Cache&); // do not implement

	void handleCpuEvent( SST::Event *ev );
	void handleBusEvent( SST::Event *ev );

	void requestBus(BusRequest *req, SimTime_t delay = 0);
	void sendBusPacket(void);

	void handleReadReq(MemEvent *ev, bool isSnoop);
	void handleReadResponse(MemEvent *ev, bool for_me);
	void handleWriteReq(MemEvent *ev, bool isSnoop);
	void handleWriteBack(MemEvent *ev);
	void handleInvalidate(MemEvent *ev);
	void handleFetch(MemEvent *ev, bool invalidate);
	void finishFetch(BusRequest *req);


	void handleMiss(MemEvent *ev);
	void advanceMiss(BusRequest *req);
	void cancelMiss(BusRequest *req);


	void writeBack(CacheBlock *block, postSendHandler finishFunc, void *ud = NULL);
	void finishWriteBackAsShared(BusRequest *req);
	void finishWriteBack(BusRequest *req);
	void sendData(MemEvent *ev, CacheBlock *block, bool isSnoop);

	void sendInvalidate(MemEvent *ev, CacheBlock *block);
	void finishInvalidate(BusRequest *req);
	void cancelInvalidate(BusRequest *req);
	void updateBlock(MemEvent *ev, CacheBlock *block);


	BusRequest* findWorkInProgress(Addr addr);
	void cancelWorkInProgress(Addr addr);

	Addr addrToTag(Addr addr);
	Addr addrToBlockAddr(Addr addr);
	CacheBlock* findBlock(Addr addr);
	CacheRow* findRow(Addr addr);


	uint32_t n_ways;
	uint32_t n_rows;
	uint32_t blocksize;
	uint32_t access_time; // TODO:  Type this properly
	std::string next_level_name;

	Addr tagshift;
	Addr rowshift;
	Addr rowmask;

	bool busRequested;

	std::vector<CacheRow> database;
	std::list<BusRequest*> packetQueue;

	RequestHold awaitingResponse;

	SST::Link* cpu_side_link;
	SST::Link* mem_side_link;
	SST::Link* coherency_link;

	/* Stats */
	uint64_t num_read_hit;
	uint64_t num_read_miss;
	uint64_t num_write_hit;
	uint64_t num_write_miss;
	uint64_t num_snoops_provided;
	uint64_t num_miss_held;

};

};
};
#endif /* _CACHE_H */
