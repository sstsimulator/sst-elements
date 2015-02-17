// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_SST_ARIEL_CORE
#define _H_SST_ARIEL_CORE

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/output.h>
#include <sst/core/interfaces/simpleMem.h>
#include <sst/core/element.h>
#include <sst/core/params.h>
#include <sst/core/simulation.h>
#include <sst/core/timeConverter.h>
#include <sst/core/timeLord.h>

#include <stdio.h>
#include <stdint.h>
#include <poll.h>

#include <string>
#include <queue>
#include <map>

#include "arielmemmgr.h"
#include "arielevent.h"
#include "arielreadev.h"
#include "arielwriteev.h"
#include "arielexitev.h"
#include "arielallocev.h"
#include "arielfreeev.h"
#include "arielnoop.h"
#include "arielswitchpool.h"

#include "ariel_shmem.h"
#include "arieltracegen.h"

using namespace SST;
using namespace SST::Interfaces;
using namespace SST::ArielComponent;

namespace SST {
namespace ArielComponent {


class ArielCore {

	public:
		ArielCore(ArielTunnel *tunnel, SimpleMem *coreToCacheLink,
                uint32_t thisCoreID, uint32_t maxPendTans,
                Output* out, uint32_t maxIssuePerCyc, uint32_t maxQLen,
                uint64_t cacheLineSz, SST::Component* owner,
			ArielMemoryManager* memMgr, const uint32_t perform_address_checks, Params& params);
		~ArielCore();
		bool isCoreHalted();
		void tick();
		void halt();
		void finishCore();
		void createReadEvent(uint64_t addr, uint32_t size);
		void createWriteEvent(uint64_t addr, uint32_t size);
		void createAllocateEvent(uint64_t vAddr, uint64_t length, uint32_t level);
		void createNoOpEvent();
		void createFreeEvent(uint64_t vAddr);
		void createExitEvent();
		void createSwitchPoolEvent(uint32_t pool);

		void setCacheLink(SimpleMem* newCacheLink);
		void handleEvent(SimpleMem::Request* event);
		void handleReadRequest(ArielReadEvent* wEv);
		void handleWriteRequest(ArielWriteEvent* wEv);
		void handleAllocationEvent(ArielAllocateEvent* aEv);
		void handleFreeEvent(ArielFreeEvent* aFE);
		void handleSwitchPoolEvent(ArielSwitchPoolEvent* aSPE);

		void commitReadEvent(const uint64_t address, const uint32_t length);
		void commitWriteEvent(const uint64_t address, const uint32_t length);

		void printCoreStatistics();
		void printTraceEntry(const bool isRead, const uint64_t address, const uint32_t length);

	private:
		bool processNextEvent();
		bool refillQueue();
		uint32_t coreID;
		uint32_t maxPendingTransactions;
		Output* output;
		std::queue<ArielEvent*>* coreQ;
		bool isHalted;
		SimpleMem* cacheLink;
		ArielTunnel *tunnel;
		std::map<SimpleMem::Request::id_t, SimpleMem::Request*>* pendingTransactions;
		uint32_t maxIssuePerCycle;
		uint32_t maxQLength;
		uint64_t cacheLineSize;
		SST::Component* owner;
		ArielMemoryManager* memmgr;
		uint32_t verbosity;
		const uint32_t perform_checks;
		bool enableTracing;
		uint64_t currentCycles;

		ArielTraceGenerator* traceGen;

		Statistic<uint64_t>* statReadRequests;
		Statistic<uint64_t>* statWriteRequests;
		Statistic<uint64_t>* statSplitReadRequests;
		Statistic<uint64_t>* statSplitWriteRequests;
		Statistic<uint64_t>* statNoopCount;

		uint64_t pending_transaction_count;

};

}
}

#endif
