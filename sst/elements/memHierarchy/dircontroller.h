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
#include <list>
#include <vector>

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include <sst/core/interfaces/memEvent.h>

#include "memNIC.h"

using namespace SST::Interfaces;

namespace SST {
namespace MemHierarchy {

class DirectoryController : public Component {


	struct DirEntry;

	typedef void(DirectoryController::*ProcessFunc)(DirEntry *entry, MemEvent *new_ev);

	struct DirEntry {
		/* First 3 items are bookkeeping for in-progress commands */
		MemEvent *activeReq;
		ProcessFunc nextFunc;
        std::string waitingOn; // waiting to hear from this source
        Command nextCommand;  // Command which we're waiting for
		uint32_t waitingAcks;

		Addr baseAddr;

		/* Standard directory data */
		bool dirty;
		std::vector<bool> sharers;

		DirEntry(Addr address, uint32_t bitlength) {
			activeReq = NULL;
			nextFunc = NULL;
			waitingAcks = 0;
			baseAddr = address;
			dirty = false;
			sharers.resize(bitlength);
		}

		uint32_t countRefs(void) {
			uint32_t count = 0;
			for ( std::vector<bool>::iterator i = sharers.begin() ; i != sharers.end() ; ++i ) {
				if ( *i ) count++;
			}
			return count;
		}

        uint32_t findOwner(void) {
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

	uint64_t lookupBaseAddr;

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


	SST::Link *memLink;
    MemNIC *network;


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

	void handleExclusiveEviction(DirEntry *entry, MemEvent *ev);

	void advanceEntry(DirEntry *entry, MemEvent *ev = NULL);

	void registerSender(const std::string &name);
	uint32_t node_id(const std::string &name);

	void requestDirEntryFromMemory(DirEntry *entry);
    void requestDataFromMemory(DirEntry *entry);
	/* Write updated entry to memory */
	void updateEntryToMemory(DirEntry *entry);
	/* Called to clear "active items" from an entry */
	void resetEntry(DirEntry *entry);

	/* Sends MemEvent to a target */
	void sendResponse(MemEvent *ev);

	/* Writes data packet to Memory */
	void writebackData(MemEvent *data_event);
    /* Requests data from Memory */

    bool isRequestAddressValid(MemEvent *ev);
    Addr convertAddressToLocalAddress(Addr addr);

public:
	DirectoryController(ComponentId_t id, Params_t &params);
	void setup(void);
    void init(unsigned int phase);
	int Finish(void) { return 0; }

	void handlePacket(SST::Event *event);
	void handleMemoryResponse(SST::Event *event);
	bool clock(SST::Cycle_t cycle);



};

}
}

#endif /* _MEMHIERARCHY_DIRCONTROLLER_H_ */
