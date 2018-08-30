// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
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
#include <unordered_map>

#include "arielmemmgr.h"
#include "arielevent.h"
#include "arielreadev.h"
#include "arielwriteev.h"
#include "arielexitev.h"
#include "arielallocev.h"
#include "arielfreeev.h"
#include "arielnoop.h"
#include "arielflushev.h"
#include "arielfenceev.h"
#include "arielswitchpool.h"
#include "arielalloctrackev.h"

#include "ariel_shmem.h"
#include "arieltracegen.h"
#include <sst/elements/Opal/Opal_Event.h>

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

        bool isCoreHalted() const;
        bool isCoreStalled() const;
        bool isCoreFenced() const;
        bool hasDrainCompleted() const;
        void tick();
        void halt();
        void stall();
        void fence();
        void unfence();
        void finishCore();
        void createReadEvent(uint64_t addr, uint32_t size);
        void createWriteEvent(uint64_t addr, uint32_t size, const uint8_t* payload);
        void createAllocateEvent(uint64_t vAddr, uint64_t length, uint32_t level, uint64_t ip);
        void createMmapEvent(uint32_t fileID, uint64_t vAddr, uint64_t length, uint32_t level, uint64_t instPtr);
        void createNoOpEvent();
        void createFreeEvent(uint64_t vAddr);
        void createExitEvent();
        void createFlushEvent(uint64_t vAddr);
        void createFenceEvent();
        void createSwitchPoolEvent(uint32_t pool);

        void setCacheLink(SimpleMem* newCacheLink, Link* allocLink);

        void handleEvent(SimpleMem::Request* event);
        void handleReadRequest(ArielReadEvent* wEv);
        void handleWriteRequest(ArielWriteEvent* wEv);
        void handleAllocationEvent(ArielAllocateEvent* aEv);
        void handleMmapEvent(ArielMmapEvent* aEv);
        void handleFreeEvent(ArielFreeEvent* aFE);
        void handleSwitchPoolEvent(ArielSwitchPoolEvent* aSPE);
        void handleFlushEvent(ArielFlushEvent *flEv);
        void handleFenceEvent(ArielFenceEvent *fEv);

        // interrupt handlers
        void handleInterruptEvent(SST::Event *event);
        void ISR_Opal(SST::OpalComponent::OpalEvent *ev);
        void setOpal() { opal_enabled = true; }
        void setOpalLink(Link * opallink);

        void commitReadEvent(const uint64_t address, const uint64_t virtAddr, const uint32_t length);
        void commitWriteEvent(const uint64_t address, const uint64_t virtAddr, const uint32_t length, const uint8_t* payload);
        void commitFlushEvent(const uint64_t address, const uint64_t virtAddr, const uint32_t length);

        // Setting the max number of instructions to be simulated
        void setMaxInsts(uint64_t i){max_insts=i;}

        void printCoreStatistics();
        void printTraceEntry(const bool isRead, const uint64_t address, const uint32_t length);

    private:
        bool processNextEvent();
        bool refillQueue();
        bool opal_enabled;
        bool writePayloads;
        uint32_t coreID;
        uint32_t maxPendingTransactions;
        Output* output;
        std::queue<ArielEvent*>* coreQ;
        bool isStalled;
        bool isHalted;
        bool isFenced;
        SimpleMem* cacheLink;
        Link* allocLink;
        Link* OpalLink;
        ArielTunnel *tunnel;
        std::unordered_map<SimpleMem::Request::id_t, SimpleMem::Request*>* pendingTransactions;
        uint32_t maxIssuePerCycle;
        uint32_t maxQLength;
        uint64_t cacheLineSize;
        SST::Component* owner;
        ArielMemoryManager* memmgr;
        const uint32_t verbosity;
        const uint32_t perform_checks;
        bool enableTracing;
        uint64_t currentCycles;
        bool updateCycle;

        // This indicates the current number of executed instructions by this core
        uint64_t inst_count;

        // This indicates the max number of instructions before halting the simulation
        uint64_t max_insts;

        ArielTraceGenerator* traceGen;

        Statistic<uint64_t>* statReadRequests;
        Statistic<uint64_t>* statWriteRequests;
	Statistic<uint64_t>* statFlushRequests;
	Statistic<uint64_t>* statFenceRequests;
        Statistic<uint64_t>* statReadRequestSizes;
        Statistic<uint64_t>* statWriteRequestSizes;
        Statistic<uint64_t>* statSplitReadRequests;
        Statistic<uint64_t>* statSplitWriteRequests;
        Statistic<uint64_t>* statNoopCount;
        Statistic<uint64_t>* statInstructionCount;
        Statistic<uint64_t>* statCycles;
        Statistic<uint64_t>* statActiveCycles;

        Statistic<uint64_t>* statFPDPIns;
        Statistic<uint64_t>* statFPDPSIMDIns;
        Statistic<uint64_t>* statFPDPScalarIns;
        Statistic<uint64_t>* statFPDPOps;
        Statistic<uint64_t>* statFPSPIns;
        Statistic<uint64_t>* statFPSPSIMDIns;
        Statistic<uint64_t>* statFPSPScalarIns;
        Statistic<uint64_t>* statFPSPOps;

        uint32_t pending_transaction_count;

};

}
}

#endif
