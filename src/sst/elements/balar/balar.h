// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

// SST includes
#include <sst/core/sst_types.h>
#include <sst/core/link.h>
#include <sst/core/event.h>
#include <sst/core/output.h>
#include <sst/core/timeConverter.h>
#include <sst/core/interfaces/stdMem.h>
#include <sst/core/component.h>
#include <../ariel/ariel_shmem.h>

// Other Includes
#include "mempool.h"
#include "host_defines.h"
#include "builtin_types.h"
#include "driver_types.h"
#include "cuda_runtime_api.h"
#include "balar_event.h"


#include <cstring>
#include <string>
#include <fstream>
#include <sstream>
#include <map>

#include <stdio.h>
#include <stdint.h>
#include <poll.h>


using namespace std;
using namespace SST;
using namespace SST::Interfaces;
using namespace SST::BalarComponent;

namespace SST {
namespace BalarComponent {
class Balar : public SST::Component {
public:
   Balar( SST::ComponentId_t id, SST::Params& params);
   void init(unsigned int phase);
   void setup()  {};
   void finish() {};

   // NEW HANDLERS
   void cpuHandler( SST::Event* e );
   void memoryHandler(StandardMem::Request* event);

   void gpuCacheHandler(StandardMem::Request* event);

   void handleCPUWriteRequest(uint64_t txSize, uint64_t pAddr);
   void handleCPUReadRequest(uint64_t txSize, uint64_t pAddr);

   class StdMemHandler : public StandardMem::RequestHandler {
      public:
        friend class Balar;
        StdMemHandler(Balar* coreInst, SST::Output* out) : StandardMem::RequestHandler(out), core(coreInst) {}
        virtual ~StdMemHandler() {}
        virtual void handle(StandardMem::ReadResp* rsp) override;
        virtual void handle(StandardMem::WriteResp* rsp) override;

        Balar* core;
        uint64_t pAddr;
        uint64_t vAddr;
        std::vector<uint8_t> data;
   };

   // TODO Need separate GPU memHierarchy first
   void handleReadRequest();
   void commitReadRequest();

   // TODO Need separate GPU memHierarchy first
   void handleWriteRequest();
   void commitWriteRequest();

   bool is_SST_buffer_full(unsigned core_id);
   void send_read_request_SST(unsigned core_id, uint64_t address, uint64_t size, void* mem_req);
   void send_write_request_SST(unsigned core_id, uint64_t address, uint64_t size, void* mem_req);
   void SST_callback_memcpy_H2D_done();
   void SST_callback_memcpy_D2H_done();

   bool tick(SST::Cycle_t x);
   cudaMemcpyKind memcpyKind;
   bool is_stalled = false;
   unsigned int transferNumber;
   std::vector< uint64_t > physicalAddresses;
   uint64_t totalTransfer;
   uint64_t ackTransfer;
   uint64_t remainingTransfer;
   uint64_t baseAddress;
   uint64_t currentAddress;
   std::vector< uint8_t > dataAddress;
   uint32_t pending_transactions_count = 0;
   uint32_t maxPendingTransCore;

   ~Balar() { };
   SST_ELI_REGISTER_COMPONENT(
      Balar,
      "balar",
      "balar",
      SST_ELI_ELEMENT_VERSION(3,2,0),
      "GPGPU simulator based on GPGPU-Sim",
      COMPONENT_CATEGORY_PROCESSOR
   )

   SST_ELI_DOCUMENT_PARAMS(
      {"verbose", "Verbosity for debugging. Increased numbers for increased verbosity.", "0"},
      {"clock", "Internal Controller Clock Rate.", "1.0 Ghz"},
      {"latency", "The time to be spent to service a memory request", "1000"},
      {"num_nodes", "number of disaggregated nodes in the system", "1"},
      {"num_cores", "Number of GPUs", "1"},
      {"maxtranscore", "Maximum number of pending transactions", "16"},
      {"maxcachetrans", "Maximum number of pending cache transactions", "512"},
   )

   // Optional since there is nothing to document
   SST_ELI_DOCUMENT_STATISTICS(
   )

   SST_ELI_DOCUMENT_PORTS(
      {"requestLink%(num_cores)d", "Handle CUDA API calls", { "BalarComponent.BalarEvent", "" } },
      {"requestMemLink%(num_cores)d", "Link to CPU memH (cache)", {} },
      {"requestGPUCacheLink%(num_cores)d", "Link to GPU memH (cache)", {} }
   )

   // Optional since there is nothing to document
   SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
   )

private:
   struct cache_req_params {
      cache_req_params( unsigned m_core_id,  void* mem_fetch, StandardMem::Request* req) {
            core_id = m_core_id;
            mem_fetch_pointer = mem_fetch;
            original_sst_req = req;
      }

      void* mem_fetch_pointer;
      unsigned core_id;
      StandardMem::Request* original_sst_req;
   };

   Balar();  // for serialization only
   Balar(const Balar&); // do not implement
   void operator=(const Balar&); // do not implement
   uint32_t cpu_core_count;
   uint32_t gpu_core_count;
   uint32_t pending_transaction_count = 0;
   std::unordered_map<StandardMem::Request::id_t, StandardMem::Request*>* pendingTransactions;
   StandardMem** gpu_to_cpu_cache_links;
   Link** gpu_to_core_links;
   uint32_t latency; // The page fault latency/ the time spent by Balar to service a memory allocation request

   StandardMem** gpu_to_cache_links;
   uint32_t maxPendingCacheTrans;
   std::unordered_map<StandardMem::Request::id_t, struct cache_req_params>* gpuCachePendingTransactions;
   uint32_t* numPendingCacheTransPerCore;
   StdMemHandler* stdMemHandlers;

   Output* output;
}; //END class Balar
} //END namespace BalarComponent
} //END namespace SST
