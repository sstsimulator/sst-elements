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

#ifndef BALAR_MMIO_H
#define BALAR_MMIO_H

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/interfaces/stdMem.h>
#include <sst/core/rng/marsaglia.h>

#include "sst/elements/memHierarchy/util.h"
#include "sst/elements/memHierarchy/memTypes.h"

// Other Includes, from original balar file
#include "mempool.h"
#include "host_defines.h"
#include "builtin_types.h"
#include "driver_types.h"
#include "vector_types.h"
#include "cuda_runtime_api.h"
#include "balar_event.h"
#include "util.h"


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
using namespace SST::MemHierarchy;

namespace SST {
namespace BalarComponent {

/* 
 * Interface to GPGPU-Sim via MMIO
 */

class BalarMMIO : public SST::Component {
public:
    SST_ELI_REGISTER_COMPONENT(BalarMMIO, "balar", "balarMMIO", SST_ELI_ELEMENT_VERSION(1,0,0),
        "GPGPU simulator based on GPGPU-Sim via MMIO interface", COMPONENT_CATEGORY_PROCESSOR)
    SST_ELI_DOCUMENT_PARAMS( 
        {"verbose",                 "(uint) Determine how verbose the output from the device is", "0"},
        {"clock",                   "(UnitAlgebra/string) Clock frequency", "1GHz"},
        {"base_addr",               "(uint) Starting addr mapped to the device", "0"},
        {"mem_accesses",            "(uint) Number of memory accesses to do", "0"},
        {"max_addr",                "(uint64) Max memory address for requests issued by this device. Required if mem_accesses > 0.", "0"},
        {"latency", "The time to be spent to service a memory request", "1000"},
        {"num_nodes", "number of disaggregated nodes in the system", "1"},
        {"num_cores", "Number of GPUs", "1"},
        {"maxtranscore", "Maximum number of pending transactions", "16"},
        {"maxcachetrans", "Maximum number of pending cache transactions", "512"},
        {"mmio_size", "Size of the MMIO memory range (Bytes)", "512"},
    )
    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS( 
        {"iface", "Interface into memory subsystem", "SST::Interfaces::StandardMem"}
    )
    SST_ELI_DOCUMENT_STATISTICS(
        {"read_latency", "Latency of reads issued by this device to memory", "simulation time units", 1},
        {"write_latency", "Latency of writes issued by this device to memory", "simulation time units", 1},
    )
    SST_ELI_DOCUMENT_PORTS(
        {"requestLink%(num_cores)d", "Handle CUDA API calls", {} },
        {"requestMemLink%(num_cores)d", "Link to CPU memH (cache)", {} },
        {"requestGPUCacheLink%(num_cores)d", "Link to GPU memH (cache)", {} }
    )

    BalarMMIO(ComponentId_t id, Params &params);
    ~BalarMMIO() {}

    virtual void init(unsigned int);
    virtual void setup();
    void finish() {};

    // Copy from original balar
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
    uint32_t mmio_size;

protected:
    /* Handle event from MMIO interface */
    void handleEvent(StandardMem::Request* req);

    /* Handle event from gpu cache */
    void handleGPUCache(StandardMem::Request* req);
    
    /* Handlers for StandardMem::Request types we handle */
    class mmioHandlers : public StandardMem::RequestHandler {
    public:
        friend class BalarMMIO;

        mmioHandlers(BalarMMIO* mmio, SST::Output* out) : StandardMem::RequestHandler(out), mmio(mmio) {}
        virtual ~mmioHandlers() {}
        virtual void handle(StandardMem::Read* read) override;
        virtual void handle(StandardMem::Write* write) override;
        virtual void handle(StandardMem::ReadResp* resp) override;
        virtual void handle(StandardMem::WriteResp* resp) override;

        // Converter for testing purpose
        void intToData(int32_t num, std::vector<uint8_t>* data);
        int32_t dataToInt(std::vector<uint8_t>* data);
        uint64_t dataToUInt64(std::vector<uint8_t>* data);

        // TODO
        // void cudaErrorToData(cudaError_t err, std::vector<uint8_t>* data);
        
        // TODO Change to a more descriptive name?
        BalarMMIO* mmio;
    };

    /* Debug -triggered by output.fatal() and/or SIGUSR2 */
    virtual void printStatus(Output &out);
    //virtual void emergencyShutdown();
    
    /* Output */
    Output out;

    Addr mmio_addr;
    mmioHandlers* handlers;

    /* Debug -triggered by output.fatal() and/or SIGUSR2 */
    virtual void emergencyShutdown() {};

private:

    // Last cuda call info
    // Return value from last cuda function call
    // TODO Make this a union for different return values
    BalarCudaCallReturnPacket_t cuda_ret;

    // Last cuda function call packet
    BalarCudaCallPacket_t *last_packet;

    // Response to a blocked API request (like cudaMemcpy)
    StandardMem::Request* blocked_response;

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

    // If this device also is testing memory accesses, these are used
    uint32_t mem_access;
    SST::RNG::MarsagliaRNG rng;
    StandardMem::Request* createWrite(uint64_t addr);
    StandardMem::Request* createRead(Addr addr);
    std::map<StandardMem::Request::id_t, std::pair<SimTime_t, std::string>> requests;
    virtual bool clockTic( SST::Cycle_t );
    Statistic<uint64_t>* statReadLatency;
    Statistic<uint64_t>* statWriteLatency;
    Addr max_addr;

    // The memH interface into the memory system
    StandardMem* iface;

    // Copy from original balar
    BalarMMIO(const BalarMMIO&); // do not implement
    void operator=(const BalarMMIO&); // do not implement
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

    Output* output;

}; // end BalarMMIO
        
}
}


#endif /*  BALAR_MMIO_H */
