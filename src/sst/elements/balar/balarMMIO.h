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
        {"gpu_cores",               "(uint) Number of GPU cores", "1"},
        {"mmio_size",               "(uint) Size of the MMIO memory range (Bytes)", "512"},
        {"dma_addr",               "(uint) Starting addr mapped to the DMA Engine", "512"},
    
    )
    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS( 
        {"iface", "Interface into memory subsystem", "SST::Interfaces::StandardMem"},
        // {"dma_if", "Interface into DMA engine", "SST::Interfaces::StandardMem"},
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

    uint32_t mmio_size;

protected:
    /* Handle event from MMIO interface */
    void handleEvent(StandardMem::Request* req);

    /* Handle DMA transfers */
    void handleDMAEvent(StandardMem::Request* req);


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
        void UInt64ToData(uint64_t num, std::vector<uint8_t>* data);
        uint64_t dataToUInt64(std::vector<uint8_t>* data);

        BalarMMIO* mmio;
    };

    class DMAHandlers : public StandardMem::RequestHandler {
    public:
        friend class BalarMMIO;

        DMAHandlers(BalarMMIO* mmio, SST::Output* out) : StandardMem::RequestHandler(out), mmio(mmio) {}
        virtual ~DMAHandlers() {}
        virtual void handle(StandardMem::ReadResp* resp) override;
        virtual void handle(StandardMem::WriteResp* resp) override;

    private:
        BalarMMIO* mmio;
    };

    /* Debug -triggered by output.fatal() and/or SIGUSR2 */
    virtual void printStatus(Output &out);
    //virtual void emergencyShutdown();
    
    /* Output */
    Output out;

    Addr mmio_addr;
    mmioHandlers* handlers;
    Addr dma_addr;
    DMAHandlers* dmaHandlers;

    // Tmp buffer to hold D2H and H2D dst data
    uint8_t* memcpyD2H_dst;
    uint8_t* memcpyH2D_dst;

    /* Debug -triggered by output.fatal() and/or SIGUSR2 */
    virtual void emergencyShutdown() {};

private:

    // Last cuda call info
    // Return value from last cuda function call
    BalarCudaCallReturnPacket_t cuda_ret;

    // Last cuda function call packet
    BalarCudaCallPacket_t *last_packet;
    Addr packet_scratch_mem_addr;

    // Response to a blocked API request (like cudaMemcpy)
    StandardMem::Request* blocked_response;

    // A currently pending write and read 
    // as we need readresp from reading the CUDA 
    // packet within SST memory
    // As well as writeresp when we finish writing
    // CUDA return packet
    StandardMem::Write* pending_write;
    StandardMem::Read* pending_read;

    // Requests sent in this class
    std::map<Interfaces::StandardMem::Request::id_t, std::pair<SimTime_t, std::string>> requests;


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

    // The memH interface into the memory system
    StandardMem* iface;

    // The interface to dma engine
    StandardMem* dma_if;

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

    Output* output;

}; // end BalarMMIO
        
}
}


#endif /*  BALAR_MMIO_H */
