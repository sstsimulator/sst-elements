// Copyright 2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _MEMHIERARCHY_DIRCONTROLLER_H_
#define _MEMHIERARCHY_DIRCONTROLLER_H_

#include <map>
#include <set>
#include <list>
#include <vector>

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include <sst/core/output.h>
#include <sst/core/interfaces/memEvent.h>

using namespace SST::Interfaces;

namespace SST {
namespace MemHierarchy {

class MemNIC;

class DirectoryController : public Component {


	struct DirEntry;

	typedef void(DirectoryController::*ProcessFunc)(DirEntry *entry, MemEvent *new_ev);

	struct DirEntry {
		/* These items are bookkeeping for in-progress commands */
		MemEvent *activeReq;
		ProcessFunc nextFunc;
        std::string waitingOn; // waiting to hear from this source
        Command nextCommand;  // Command which we're waiting for
        MemEvent::id_type lastRequest;  // ID of message we're wanting a response to
        static const MemEvent::id_type NO_LAST_REQUEST;
		uint32_t waitingAcks;
        bool inController; // Whether this is present in the controller, or needs to be fetched

		Addr baseAddr;

		/* Standard directory data */
		bool dirty;
		std::vector<bool> sharers;

		DirEntry(Addr address, uint32_t bitlength)
        {
			activeReq = NULL;
			nextFunc = NULL;
            lastRequest = NO_LAST_REQUEST;
			waitingAcks = 0;
            inController = true;
			baseAddr = address;
			dirty = false;
			sharers.resize(bitlength);
		}

		uint32_t countRefs(void)
        {
			uint32_t count = 0;
			for ( std::vector<bool>::iterator i = sharers.begin() ; i != sharers.end() ; ++i ) {
				if ( *i ) count++;
			}
			return count;
		}

        void clearSharers(void)
        {
			for ( std::vector<bool>::iterator i = sharers.begin() ; i != sharers.end() ; ++i ) {
				*i = false;
			}
        }

        uint32_t findOwner(void)
        {
            assert(dirty);
            for ( uint32_t i = 0 ; i < sharers.size() ; i++ ) {
                if ( sharers[i] ) return i;
            }
            // SHOULD NEVER GET HERE
            assert(false);
            return 0;
        }

		bool inProgress(void) { return activeReq != NULL; }
	};

    Output dbg;

	uint64_t lookupBaseAddr;

    std::set<MemEvent::id_type> ignorableResponses;

	/* Range of addresses supported by this directory */
	Addr addrRangeStart;
	Addr addrRangeEnd;
    Addr interleaveSize;
    Addr interleaveStep;

	/* Total number of cache blocks we are responsible for */
	/* ie, sum of all caches we talk to */
	uint32_t numTargets;
    uint32_t blocksize;

	std::map<Addr, DirEntry*> directory;
	std::map<std::string, uint32_t> node_lookup;
    std::vector<std::string> nodeid_to_name;
	uint32_t targetCount;

	/* Queue of packets to work on */
	// list to support constant time removal
	std::list<MemEvent*> workQueue;
    std::map<MemEvent::id_type, Addr> memReqs;

    std::map<MemEvent::id_type, MemEvent*> uncachedWrites;

	SST::Link *memLink;
    MemNIC *network;

    size_t entryCacheSize;
    std::list<DirEntry*> entryCache;

    uint64_t numReqsProcessed;
    uint64_t totalReqProcessTime;
    uint64_t numCacheHits;
    Output::output_location_t printStatsLoc;



	DirEntry* getDirEntry(Addr target);
	DirEntry* createDirEntry(Addr target);


	void handleACK(MemEvent *ev);
	void handleInvalidate(DirEntry* entry, MemEvent *new_ev);
	void finishInvalidate(DirEntry *entry, MemEvent *new_ev);
    void sendInvalidate(int target, Addr address);

	void handleRequestData(DirEntry* entry, MemEvent *new_ev);
	void finishFetch(DirEntry* entry, MemEvent *new_ev);
	void sendRequestedData(DirEntry* entry, MemEvent *new_ev);
	void getExclusiveDataForRequest(DirEntry* entry, MemEvent *new_ev);

	void handleWriteback(DirEntry *entry, MemEvent *ev);

    void handleEviction(DirEntry *entry, MemEvent *ev);

	void advanceEntry(DirEntry *entry, MemEvent *ev = NULL);

	uint32_t node_id(const std::string &name);

    void updateCacheEntry(DirEntry *entry);
	void requestDirEntryFromMemory(DirEntry *entry);
    /* Requests data from Memory */
    void requestDataFromMemory(DirEntry *entry);
	/* Write updated entry to memory */
	void updateEntryToMemory(DirEntry *entry);
    void sendEntryToMemory(DirEntry *entry);
	/* Called to clear "active items" from an entry */
	void resetEntry(DirEntry *entry);

	/* Sends MemEvent to a target */
	void sendResponse(MemEvent *ev);

	/* Writes data packet to Memory
     * Returns the MemEvent ID of the data written to memory */
    MemEvent::id_type writebackData(MemEvent *data_event);

    bool isRequestAddressValid(MemEvent *ev);
    Addr convertAddressToLocalAddress(Addr addr);

    const char* printDirectoryEntryStatus(Addr addr);

public:
	DirectoryController(ComponentId_t id, Params_t &params);
    ~DirectoryController();
	void setup(void);
    void init(unsigned int phase);
	void finish(void);

	void handlePacket(SST::Event *event);
	bool processPacket(MemEvent *ev);
	void handleMemoryResponse(SST::Event *event);
	bool clock(SST::Cycle_t cycle);



};

}
}

#endif /* _MEMHIERARCHY_DIRCONTROLLER_H_ */
