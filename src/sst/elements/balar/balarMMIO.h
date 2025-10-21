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

#ifndef BALAR_MMIO_H
#define BALAR_MMIO_H

#include <unordered_map>
#include <cstdint>
#include <stack>
#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/interfaces/stdMem.h>
#include <sst/core/rng/marsaglia.h>

#include "sst/elements/memHierarchy/util.h"
#include "balar_packet.h"

// Other Includes, from original balar file
#include "mempool.h"
#include "builtin_types.h"
#include "driver_types.h"
#include "cuda_runtime_api.h"


#include <cstring>
#include <string>
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
        {"gpu_cores",               "(uint) Number of GPU cores", "1"},
        {"mmio_size",               "(uint) Size of the MMIO memory range (Bytes)", "512"},
        {"dma_addr",                "(uint) Starting addr mapped to the DMA Engine", "512"},
        {"cuda_executable",         "(string) CUDA executable file path to extract PTX info", ""},
    )
    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
        {"mmio_iface", "Command packet MMIO interface", "SST::Interfaces::StandardMem"},
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

    // Callbacks from GPGPU-Sim
    void SST_callback_event_done(const char* event_name, cudaStream_t stream, uint8_t* payload, size_t payload_size);

    uint32_t mmio_size;

protected:
    /* Handle event from MMIO interface */
    void handleEvent(StandardMem::Request* req);

    /* Handle DMA transfers */
    void handleDMAEvent(StandardMem::Request* req);

    /* Handle event from gpu cache */
    void handleGPUCache(StandardMem::Request* req);

    /* Handlers for command and data requests/responses on the two interfaces */
    class BalarHandlers : public StandardMem::RequestHandler {
    public:
        friend class BalarMMIO;

        BalarHandlers(BalarMMIO* balar, SST::Output* out) : StandardMem::RequestHandler(out), balar(balar) {}
        virtual ~BalarHandlers() {}
        // These two handle read/write from `mmio_iface` and issue read/write of CUDA packets via `mmio_iface` to DMA engine
        virtual void handle(StandardMem::Read* read) override;
        virtual void handle(StandardMem::Write* write) override;
        // Handler for previous read is currently not used
        virtual void handle(StandardMem::ReadResp* resp) override;
        // Handle responses from DMA engine command (R/W cuda packets and cuda memcpy)
        virtual void handle(StandardMem::WriteResp* resp) override;

        // Converter for testing purpose
        void intToData(int32_t num, std::vector<uint8_t>* data);
        int32_t dataToInt(std::vector<uint8_t>* data);
        void UInt64ToData(uint64_t num, std::vector<uint8_t>* data);
        uint64_t dataToUInt64(std::vector<uint8_t>* data);

        BalarMMIO* balar;
    };

    /* Debug -triggered by output.fatal() and/or SIGUSR2 */
    virtual void printStatus(Output &out);

    /* Output */
    Output out;

    Addr mmio_addr;
    BalarHandlers* handlers;
    Addr dma_addr;

    /* Debug -triggered by output.fatal() and/or SIGUSR2 */
    virtual void emergencyShutdown() {};

private:

    // CUDA executable path, overwrites BalarCudaCallPacket_t.register_fatbin.file_name
    std::string cudaExecutable;

    // Last cuda call info
    // Return value from last cuda function call
    BalarCudaCallReturnPacket_t cuda_ret;

    // Last cuda function call packet
    BalarCudaCallPacket_t last_packet;
    Addr packet_scratch_mem_addr;

    // Indicating that an API has been blocked from issuing
    // This should be marked for every CUDA API in GPGPU-Sim that
    // would push an operation to its stream manager
    bool has_blocked_issue;
    // Indicating that an API has issued, but is blocking on completion
    bool has_blocked_response;
    // Response to a blocked API request (like cudaMemcpy)
    StandardMem::Request* blocked_response;

    // A currently pending write and read
    // as we need readresp from reading the CUDA
    // packet within SST memory
    // As well as writeresp when we finish writing
    // CUDA return packet
    StandardMem::Write* pending_write = nullptr;
    StandardMem::Read* pending_read = nullptr;

    // Requests sent in this class with simulation time, command, and the associated cuda call packet
    std::map<Interfaces::StandardMem::Request::id_t, std::tuple<SimTime_t, std::string, BalarCudaCallPacket_t*>> requests;
    // CUDA call packets with pending response (callback from GPGPU-Sim)
    // calls to different streams are stored in separate queue
    // as within stream, order of calls should be preserved
    // Particularly, default stream is map with key 0
    // Packet are inserted into the queue on receiving the packet
    // and are removed from the queue on receiving the corresponding callback
    // TL;DR: This is a lightweight stream manager for CUDA API calls
    std::map<cudaStream_t, std::queue<BalarCudaCallPacket_t*>> pending_packets_per_stream;
    // CUDA Launch config stream stack
    std::stack<cudaStream_t> cudalaunch_stream_stack;

    // CUDA API management related
    // Mapping from Vanadis's texture pointer to the pointer of copy in simulator memspace
    std::map<uint64_t, struct textureReference *> cudaTextureMapping;


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

    virtual bool clockTic( SST::Cycle_t );

    // For checking if the stream operation will be blocking
    bool isStreamBlocking(cudaStream_t);

    // The command mmio interface into the memory system
    StandardMem* mmio_iface;

    // Copy from original balar
    BalarMMIO(const BalarMMIO&); // do not implement
    void operator=(const BalarMMIO&); // do not implement
    uint32_t gpu_core_count;
    StandardMem** gpu_to_cpu_cache_links;
    Link** gpu_to_core_links;

    StandardMem** gpu_to_cache_links;
    uint32_t maxPendingCacheTrans;
    std::unordered_map<StandardMem::Request::id_t, struct cache_req_params>* gpuCachePendingTransactions;
    uint32_t* numPendingCacheTransPerCore;
    bool isLaunchBlocking;

    Output* output;

}; // end BalarMMIO

}
}


#endif /*  BALAR_MMIO_H */
