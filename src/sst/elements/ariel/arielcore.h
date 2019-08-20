// Copyright 2009-2019 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2019, NTESS
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
#include <sst/core/componentExtension.h>
#include <sst/core/link.h>
#include <sst/core/output.h>
#include <sst/core/interfaces/simpleMem.h>
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

#include "ariel_shmem.h"
#include "arieltracegen.h"

#ifdef HAVE_CUDA
#include "arielgpuev.h"
#endif

using namespace SST;
using namespace SST::Interfaces;
using namespace SST::ArielComponent;

namespace SST {
namespace ArielComponent {


class ArielCore : public ComponentExtension {

    public:
        ArielCore(ComponentId_t id, ArielTunnel *tunnel, 
#ifdef HAVE_CUDA
            GpuReturnTunnel *tunnelR, GpuDataTunnel *tunnelD,
#endif
            uint32_t thisCoreID, uint32_t maxPendTans, Output* out,
            uint32_t maxIssuePerCyc, uint32_t maxQLen, uint64_t cacheLineSz,
            ArielMemoryManager* memMgr, const uint32_t perform_address_checks,
            SST::Link* notifyEmptyLink, Params& params);
        ~ArielCore();

        bool isCoreHalted() const;
        bool isCoreStalled() const;
#ifdef HAVE_CUDA
        cudaMemcpyKind getKind() const;
        bool getMidTransfer() const;
        size_t getTotalTransfer() const;
        size_t getPageAckTransfer() const;
        size_t getPageTransfer() const;
        size_t getAckTransfer() const;
        size_t getRemainingTransfer() const;
        size_t getRemainingPageTransfer() const;
        uint64_t getBaseAddress() const;
        uint64_t getCurrentAddress() const;
        uint8_t* getDataAddress() const;
        uint8_t* getBaseDataAddress() const;
        int getOpenTransactions() const;
        void setMidTransfer(bool midTx);
        void setKind(cudaMemcpyKind memcpyKind);
        void setTotalTransfer(size_t tx);
        void setPageTransfer(size_t tx);
        void setPageAckTransfer(size_t tx);
        void setRemainingPageTransfer(size_t tx);
        void setAckTransfer(size_t tx);
        void setRemainingTransfer(size_t tx);
        void setBaseAddress(uint64_t virtAddress);
        void setCurrentAddress(uint64_t virtAddress);
        void setDataAddress(uint8_t* virtAddress);
        void setBaseDataAddress(uint8_t* virtAddress);
        void setPhysicalAddresses(SST::Event *ev);
        bool isGpuEx() const;
        void gpu();
#endif
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

        void setCacheLink(SimpleMem* newCacheLink);

#ifdef HAVE_CUDA
        void createGpuEvent(GpuApi_t API, CudaArguments CA);
        void setGpu() { gpu_enabled = true; }
        void setGpuLink(Link* gpulink);
#endif

        void handleEvent(SimpleMem::Request* event);
        void handleReadRequest(ArielReadEvent* wEv);
        void handleWriteRequest(ArielWriteEvent* wEv);
        void handleAllocationEvent(ArielAllocateEvent* aEv);
        void handleMmapEvent(ArielMmapEvent* aEv);
        void handleFreeEvent(ArielFreeEvent* aFE);
        void handleSwitchPoolEvent(ArielSwitchPoolEvent* aSPE);
        void handleFlushEvent(ArielFlushEvent *flEv);
        void handleFenceEvent(ArielFenceEvent *fEv);

#ifdef HAVE_CUDA
        void handleGpuEvent(ArielGpuEvent* gEv);
        void handleGpuAckEvent(SST::Event* e);
        void handleGpuMemcpy(ArielGpuEvent* gEv);
#endif

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

#ifdef HAVE_CUDA
        size_t totalTransfer;
        bool gpu_enabled;
        size_t pageTransfer;
        size_t ackTransfer;
        size_t pageAckTransfer;
        size_t remainingTransfer;
        size_t remainingPageTransfer;
        uint64_t baseAddress;
        uint64_t currentAddress;
        uint8_t* dataAddress;
        uint8_t* baseDataAddress;
        bool midTransfer;
        std::vector<uint64_t> physicalAddresses;
        cudaMemcpyKind kind;
        bool isGpu;
#endif

        Output* output;
        std::queue<ArielEvent*>* coreQ;
        bool isStalled;
        bool isHalted;
        bool isFenced;

        SimpleMem* cacheLink;
        ArielTunnel *tunnel;

#ifdef HAVE_CUDA
        Link* GpuLink;
        GpuReturnTunnel *tunnelR;
        GpuDataTunnel *tunnelD;
        std::unordered_map<SimpleMem::Request::id_t, SimpleMem::Request*>* pendingGpuTransactions;
#endif

        std::unordered_map<SimpleMem::Request::id_t, SimpleMem::Request*>* pendingTransactions;
        uint32_t maxIssuePerCycle;
        uint32_t maxQLength;
        uint64_t cacheLineSize;
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
        uint32_t pending_gpu_transaction_count;

        SST::Link* m_notifyEmptyLink;

};

}
}

#endif
