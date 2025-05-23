// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_SST_ARIEL_CORE
#define _H_SST_ARIEL_CORE

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/componentExtension.h>
#include <sst/core/link.h>
#include <sst/core/output.h>
#include <sst/core/interfaces/stdMem.h>
#include <sst/core/params.h>
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
#include "arielrtlev.h"
#include "tb_header.h"

#include "ariel_shmem.h"
#include "arieltracegen.h"

using namespace SST;
using namespace SST::Interfaces;
using namespace SST::ArielComponent;

struct RequestInfo {
        StandardMem::Request *req;
        uint64_t start;
};

namespace SST {
namespace ArielComponent {


class ArielCore : public ComponentExtension {

    public:
        ArielCore(ComponentId_t id, ArielTunnel *tunnel,
            uint32_t thisCoreID, uint32_t maxPendTans, Output* out,
            uint32_t maxIssuePerCyc, uint32_t maxQLen, uint64_t cacheLineSz,
            ArielMemoryManager* memMgr, const uint32_t perform_address_checks, Params& params,
            TimeConverter *timeconverter);
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

        void setFilePath(std::string fp) {
          getcwd(file_path, sizeof(file_path));
          strcat(file_path, "/");
          strcat(file_path, fp.c_str());
          if (!(access(file_path, F_OK) == 0)) {
            strcpy(file_path, fp.c_str());
          }
      }

        void setCacheLink(StandardMem* newCacheLink);
        void createRtlEvent(void*, void*, void*, size_t, size_t, size_t);
        void setRtlLink(Link* rtllink);

        void handleEvent(StandardMem::Request* event);
        void handleReadRequest(ArielReadEvent* wEv);
        void handleWriteRequest(ArielWriteEvent* wEv);
        void handleAllocationEvent(ArielAllocateEvent* aEv);
        void handleMmapEvent(ArielMmapEvent* aEv);
        void handleFreeEvent(ArielFreeEvent* aFE);
        void handleSwitchPoolEvent(ArielSwitchPoolEvent* aSPE);
        void handleFlushEvent(ArielFlushEvent *flEv);
        void handleFenceEvent(ArielFenceEvent *fEv);
        void handleRtlEvent(ArielRtlEvent* RtlEv);
        void handleRtlAckEvent(SST::Event* e);

        /* Handler class for StandardMem responses */
        class StdMemHandler : public StandardMem::RequestHandler {
        public:
            friend class ArielCore;
            StdMemHandler(ArielCore* coreInst, SST::Output* out) : StandardMem::RequestHandler(out), core(coreInst) {}
            virtual ~StdMemHandler() {}

            ArielCore* core;
        };


        // interrupt handlers
        bool handleInterrupt(ArielMemoryManager::InterruptAction action);

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
        bool writePayloads;
        uint32_t coreID;
        uint32_t maxPendingTransactions;

        Output* output;
        std::queue<ArielEvent*>* coreQ;
        bool isStalled;
        bool isHalted;
        bool isFenced;

        StandardMem* cacheLink;
        ArielTunnel *tunnel;
        StdMemHandler* stdMemHandlers;
        Link* RtlLink;
        TimeConverter timeconverter; // TimeConverter for the associated ArielCPU

        std::unordered_map<StandardMem::Request::id_t, RequestInfo>* pendingTransactions;
        uint32_t maxIssuePerCycle;
        uint32_t maxQLength;
        uint64_t cacheLineSize;
        void* rtl_inp_ptr = nullptr;
        ArielMemoryManager* memmgr;
        const uint32_t verbosity;
        const uint32_t perform_checks;
        bool enableTracing;
        uint64_t currentCycles;
        bool updateCycle;
        char file_path[256];

        // This indicates the current number of executed instructions by this core
        uint64_t inst_count;

        // This indicates the max number of instructions before halting the simulation
        uint64_t max_insts;

        ArielTraceGenerator* traceGen;

        Statistic<uint64_t>* statReadRequests;
        Statistic<uint64_t>* statWriteRequests;
        Statistic<uint64_t>* statReadLatency;
        Statistic<uint64_t>* statWriteLatency;
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
