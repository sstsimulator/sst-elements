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

#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <tuple>
#include <utility>
#include <vector>
#include <sst_config.h>
#include "balarMMIO.h"
#include "dmaEngine.h"
#include "output.h"
#include "util.h"

using namespace SST;
using namespace SST::Interfaces;
using namespace SST::MemHierarchy;

SST::BalarComponent::BalarMMIO* g_balarmmio_component = NULL;

/* Example MMIO device */
BalarMMIO::BalarMMIO(ComponentId_t id, Params &params) : SST::Component(id) {

    // Output for warnings
    out.init("BalarMMIOComponent[@f:@l:@p] ", params.find<int>("verbose", 1), 0, Output::STDOUT);

    // Configurations for GPU
    gpu_core_count = (uint32_t) params.find<uint32_t>("gpu_cores", 1);
    maxPendingCacheTrans = (uint32_t) params.find<uint32_t>("maxcachetrans", 512);
    mmio_size = (uint32_t) params.find<uint32_t>("mmio_size", 512);

    // Ensure that GPGP-sim has the same as SST gpu cores
    SST_gpgpusim_numcores_equal_check(gpu_core_count);
    // Know if we are in launch blocking mode
    isLaunchBlocking = SST_gpgpusim_launch_blocking();

    bool found = false;

    // CUDA executable path, if exists, overrides path from Balar packet when registering fatbin
    cudaExecutable = params.find<std::string>("cuda_executable", "", found);
    if (found)
        out.verbose(CALL_INFO, 1, 0, "Use CUDA executable at %s for __cudaRegisterFatbin", cudaExecutable.c_str());

    // Memory address
    mmio_addr = params.find<uint64_t>("base_addr", 0);
    dma_addr = params.find<uint64_t>("dma_addr", 0);

    std::string clockfreq = params.find<std::string>("clock", "1GHz");
    UnitAlgebra clock_ua(clockfreq);
    if (!(clock_ua.hasUnits("Hz") || clock_ua.hasUnits("s")) || clock_ua.getRoundedValue() <= 0) {
        out.fatal(CALL_INFO, -1, "%s, Error - Invalid param: clock. Must have units of Hz or s and be > 0. "
                "(SI prefixes ok). You specified '%s'\n", getName().c_str(), clockfreq.c_str());
    }
    TimeConverter* tc = getTimeConverter(clockfreq);

    // Bind tick function
    registerClock(tc, new Clock::Handler2<BalarMMIO,&BalarMMIO::clockTic>(this));

    // Link names
    char* link_buffer = (char*) malloc(sizeof(char) * 256);
    char* link_buffer_mem = (char*) malloc(sizeof(char) * 256);
    char* link_cache_buffer = (char*) malloc(sizeof(char) * 256);

    // Buffer to keep track of pending transactions
    gpuCachePendingTransactions = new std::unordered_map<StandardMem::Request::id_t, cache_req_params>();

    // Interface to CPU via MMIO
    mmio_iface = loadUserSubComponent<SST::Interfaces::StandardMem>("mmio_iface", ComponentInfo::SHARE_NONE, tc,
            new StandardMem::Handler2<BalarMMIO,&BalarMMIO::handleEvent>(this));
    if (!mmio_iface) {
        out.fatal(CALL_INFO, -1, "%s, Error: No interface found loaded into 'mmio_iface' subcomponent slot. Please check input file\n", getName().c_str());
    }

    mmio_iface->setMemoryMappedAddressRegion(mmio_addr, mmio_size);

    // Handlers for cpu interactions
    handlers = new BalarHandlers(this, &out);

    // GPU Cache interface configuration
    gpu_to_cache_links = (StandardMem**) malloc( sizeof(StandardMem*) * gpu_core_count );
    numPendingCacheTransPerCore = new uint32_t[gpu_core_count];

    SubComponentSlotInfo* gpu_to_cache = getSubComponentSlotInfo("gpu_cache");
    if (gpu_to_cache) {
        if (!gpu_to_cache->isAllPopulated())
            out.fatal(CALL_INFO, -1, "%s, Error: loading 'gpu_cache' subcomponents. All subcomponent slots from 0 to gpu core count must be populated. "
                    "Check your input config for non-populated slots\n", getName().c_str());

        uint32_t subCompCount = gpu_to_cache->getMaxPopulatedSlotNumber() == -1 ? 0 : gpu_to_cache->getMaxPopulatedSlotNumber() + 1;
        if (subCompCount != gpu_core_count)
            out.fatal(CALL_INFO, -1, "%s, Error: loading 'gpu_cache' subcomponents and the number of subcomponents does not match the number of GPU cores. "
                    "Cores: %" PRIu32 ", SubComps: %" PRIu32 ". Check your input config.\n",
                    getName().c_str(), gpu_core_count, subCompCount);
    }

    // Create and initialize GPU memHierarchy links (StandardMem)
    for (uint32_t i = 0; i < gpu_core_count; i++) {
        if (gpu_to_cache) {
            gpu_to_cache_links[i] = gpu_to_cache->create<Interfaces::StandardMem>(i, ComponentInfo::INSERT_STATS, tc, new StandardMem::Handler2<BalarMMIO,&BalarMMIO::handleGPUCache>(this));
        } else {
    	    // Create a unique name for all links, the configure file links need to match this
    	    sprintf(link_cache_buffer, "requestGPUCacheLink%" PRIu32, i);
            out.verbose(CALL_INFO, 1, 0, "Starts to configure cache link (%s) for core %d\n", link_cache_buffer, i);
            Params param;
            param.insert("port", link_cache_buffer);

            gpu_to_cache_links[i] = loadAnonymousSubComponent<SST::Interfaces::StandardMem>("memHierarchy.standardInterface",
                    "gpu_cache", i, ComponentInfo::SHARE_PORTS | ComponentInfo::INSERT_STATS,
                    param, tc, new StandardMem::Handler2<BalarMMIO,&BalarMMIO::handleGPUCache>(this));
            out.verbose(CALL_INFO, 1, 0, "Finish configure cache link for core %d\n", i);
        }

        numPendingCacheTransPerCore[i] = 0;
    }

    g_balarmmio_component = this;

    // Initial values
    has_blocked_response = false;

    // Initialize pending packets queue with default stream
    pending_packets_per_stream.insert({0, std::queue<BalarCudaCallPacket_t*>()});
}

void BalarMMIO::init(unsigned int phase) {
    mmio_iface->init(phase);
    for (uint32_t i = 0; i < gpu_core_count; i++)
        gpu_to_cache_links[i]->init(phase);
}

void BalarMMIO::setup() {
    mmio_iface->setup();
    for (uint32_t i = 0; i < gpu_core_count; i++)
        gpu_to_cache_links[i]->setup();
}

bool BalarMMIO::is_SST_buffer_full(unsigned core_id) {
    return (numPendingCacheTransPerCore[core_id] == maxPendingCacheTrans);
}

/**
 * @brief Callback in sst-gpgpusim to send memory read request to balarMMIO/SST
 *        and use the SST backend for timing simulation?
 *        Check shader.cc:void simt_core_cluster::icnt_inject_request_packet_to_SST
 *
 * @param core_id:  GPU core id
 * @param address:  memory request addr
 * @param size   :  request size
 * @param mem_req:  pointer to memory fetch object in GPGPUSim
 */
void BalarMMIO::send_read_request_SST(unsigned core_id, uint64_t address, uint64_t size, void* mem_req) {
    assert(numPendingCacheTransPerCore[core_id] < maxPendingCacheTrans);
    StandardMem::Read* req = new Interfaces::StandardMem::Read(address, size);
    req->vAddr = address;
    cache_req_params crp(core_id, mem_req, req);
    gpuCachePendingTransactions->insert(std::pair<StandardMem::Request::id_t, cache_req_params>(req->getID(), crp));
    numPendingCacheTransPerCore[core_id]++;
    gpu_to_cache_links[core_id]->send(req);

    out.verbose(CALL_INFO, 1, 0, "Sent a read request with id (%ld) to addr %lx\n", req->getID(), req->pAddr);
}

/**
 * @brief Callback in sst-gpgpusim to send memory write request to balarMMIO/SST
 *        and use the SST backend for timing simulation?
 *        Check shader.cc:void simt_core_cluster::icnt_inject_request_packet_to_SST
 *
 * @param core_id:  GPU core id
 * @param address:  memory request addr
 * @param size   :  request size
 * @param mem_req:  pointer to memory fetch object in GPGPUSim
 */
void BalarMMIO::send_write_request_SST(unsigned core_id, uint64_t address, uint64_t size, void* mem_req) {
    assert(numPendingCacheTransPerCore[core_id] < maxPendingCacheTrans);
    std::vector<uint8_t> mock_data(size);
    StandardMem::Write* req = new Interfaces::StandardMem::Write(address, size, mock_data, false);
    req->vAddr = address;
    gpuCachePendingTransactions->insert(std::pair<StandardMem::Request::id_t, cache_req_params>(req->getID(), cache_req_params(core_id, mem_req, req)));
    numPendingCacheTransPerCore[core_id]++;
    gpu_to_cache_links[core_id]->send(req);
    out.verbose(CALL_INFO, 1, 0, "Sent a write request with id (%ld) to addr: %lx\n", req->getID(), req->pAddr);
}

/**
 * @brief Callback function called by GPGPU-Sim when certain CUDA calls
 *        are actually done. Each if is a handler for different
 *        callbacks.
 *
 * @param event_name
 * @param stream
 * @param payload
 * @param payload_size
 */
void BalarMMIO::SST_callback_event_done(const char* event_name, cudaStream_t stream, uint8_t* payload, size_t payload_size) {
    out.verbose(CALL_INFO, 1, 0, "Receiving event %s from GPGPU-Sim at stream %p\n", event_name, stream);
    std::string event_string = std::string(event_name);

    // Get ref to the corresponding stream packet queue
    auto& stream_queue = pending_packets_per_stream.at(stream);
    // Get the head packet of the stream
    BalarCudaCallPacket_t * head_packet = stream_queue.front();

    // Handling different types of done events
    if (event_string == "memcpy_H2D_done") { // Handle H2D done
        out.verbose(CALL_INFO, 1, 0, "Prepare memcpyH2D finish off\n");

        // Unpack the payload packet
        if (payload_size != sizeof(BalarCudaCallPacket_t)) {
            out.fatal(CALL_INFO, -1, "memcpy_H2D_done: Invalid payload size %ld\n", payload_size);
        }
        BalarCudaCallPacket_t * payload_packet = (BalarCudaCallPacket_t *) payload;

        // Check if the head packet is a memcpy call
        if (head_packet->cuda_call_id != CUDA_MEMCPY &&
            head_packet->cuda_call_id != CUDA_MEMCPY_ASYNC) {
            out.fatal(CALL_INFO, -1, "memcpy_H2D_done: Head packet cuda_call_id %d is not a memcpy call\n", head_packet->cuda_call_id);
        }
        // Validate that the head packet matches with the callback's payload
        if (head_packet->cuda_call_id != payload_packet->cuda_call_id) {
            out.fatal(CALL_INFO, -1, "memcpy_H2D_done: Head packet cuda_call_id %d does not match with the callback's payload %d\n", head_packet->cuda_call_id, payload_packet->cuda_call_id);
        }
        // Handle async memcpy
        if (head_packet->cuda_call_id == CUDA_MEMCPY_ASYNC) {
            // Check that payload's memcpy params matches
            // Cannot just use isSameBalarCudaCallPacket as we dont know from callback
            // whether it is a SST mem cuda call or not, so the src field might be
            // different due to the fact that we will assign a buffer to hold the src data
            // from SST memspace. So instead, we will just compare the count and dst
            // field
            if (head_packet->cudaMemcpyAsync.count != payload_packet->cudaMemcpyAsync.count ||
                head_packet->cudaMemcpyAsync.dst != payload_packet->cudaMemcpyAsync.dst ||
                (head_packet->cudaMemcpyAsync.kind != cudaMemcpyHostToDevice)) {
                out.fatal(CALL_INFO, -1, "memcpy_H2D_done: Head packet does not match with the callback's payload's dst/count/src/kind\n");
            }

            // Otherwise, since async call is already returned, we will just
            // free allocated memory if we allocate the buffer (with DMA), pop the head packet and return
            if (head_packet->isSSTmem)
                free(head_packet->cudaMemcpyAsync.src_buf);
        } else {
            // Check that payload's memcpy params matches
            if (head_packet->cuda_memcpy.count != payload_packet->cuda_memcpy.count ||
                head_packet->cuda_memcpy.dst != payload_packet->cuda_memcpy.dst ||
                (head_packet->cuda_memcpy.kind != cudaMemcpyHostToDevice)) {
                out.fatal(CALL_INFO, -1, "memcpy_H2D_done: Head packet does not match with the callback's payload's dst/count/src/kind\n");
            }

            // This is a blocking memcpy, we should not have another cuda call coming
            if (!has_blocked_response) {
                out.fatal(CALL_INFO, -1, "memcpy_H2D_done: expecting a blocked response for sync memcpy\n");
            }
            if (!isSameBalarCudaCallPacket(head_packet, &last_packet, true)) {
                out.fatal(CALL_INFO, -1, "memcpy_H2D_done: another cuda call (%s) get in the way of a blocking memcpy\n", CudaAPIEnumToString(last_packet.cuda_call_id));
            }

            // Check if we are in SSTMemsapce, if so, need to free allocated buffer
            if (head_packet->isSSTmem &&
                (cuda_ret.is_cuda_call_done == false)) {
                // Safely free the source data buffer
                free(head_packet->cuda_memcpy.src_buf);

                // Mark the CUDA call as done
                cuda_ret.is_cuda_call_done = true;
            }

            // Send blocked response
            mmio_iface->send(blocked_response);
            has_blocked_response = false;
        }
    } else if (event_string == "memcpy_D2H_done") { // Handle memcpy_D2H_done
        out.verbose(CALL_INFO, 1, 0, "Prepare memcpyD2H writes\n");

        // Unpack the payload
        if (payload_size != sizeof(BalarCudaCallPacket_t)) {
            out.fatal(CALL_INFO, -1, "memcpy_D2H_done: Invalid payload size %ld\n", payload_size);
        }
        BalarCudaCallPacket_t * payload_packet = (BalarCudaCallPacket_t *) payload;

        // Check if the head packet is a memcpy call
        if (head_packet->cuda_call_id != CUDA_MEMCPY &&
            head_packet->cuda_call_id != CUDA_MEMCPY_ASYNC) {
            out.fatal(CALL_INFO, -1, "memcpy_D2H_done: Head packet cuda_call_id %d is not a memcpy call\n", head_packet->cuda_call_id);
        }
        // Validate that the head packet matches with the callback's payload
        if (head_packet->cuda_call_id != payload_packet->cuda_call_id) {
            out.fatal(CALL_INFO, -1, "memcpy_D2H_done: Head packet cuda_call_id %d does not match with the callback's payload %d\n", head_packet->cuda_call_id, payload_packet->cuda_call_id);
        }

        // Handle async memcpy
        if (head_packet->cuda_call_id == CUDA_MEMCPY_ASYNC) {
            // Check that payload's memcpy params matches
            if (head_packet->cudaMemcpyAsync.count != payload_packet->cudaMemcpyAsync.count ||
                head_packet->cudaMemcpyAsync.src != payload_packet->cudaMemcpyAsync.src ||
                (head_packet->cudaMemcpyAsync.kind != cudaMemcpyDeviceToHost)) {
                out.fatal(CALL_INFO, -1, "memcpy_D2H_done: Head packet does not match with the callback's payload's dst/count/src/kind\n");
            }

            // Use DMA engine to send the copy from Simulator to SST memspace
            // Prepare a request to write the dst data to CPU
            if (head_packet->isSSTmem) {
                std::string cmdString = "Issue_DMA_memcpy_D2H_async";
                // Prepare DMA config

                DMAEngine::DMAEngineControlRegisters dma_registers;
                dma_registers.sst_mem_addr = head_packet->cudaMemcpyAsync.dst;
                dma_registers.simulator_mem_addr = head_packet->cudaMemcpyAsync.dst_buf;
                dma_registers.data_size = head_packet->cudaMemcpyAsync.count;
                dma_registers.transfer_size = 4;    // 4 bytes per transfer
                dma_registers.dir = DMAEngine::DMA_DIR::SIM_TO_SST; // From SST memspace to simulator memspace

                // Send the DMA request to DMA engine
                std::vector<uint8_t> * payload_ptr = encode_balar_packet<DMAEngine::DMAEngineControlRegisters>(&dma_registers);
                StandardMem::Request* dma_req = new StandardMem::Write(
                    dma_addr,
                    sizeof(DMAEngine::DMAEngineControlRegisters),
                    *payload_ptr);
                delete payload_ptr;

                // Record in hashmap
                requests[dma_req->getID()] = std::make_tuple(getCurrentSimTime(), cmdString, new BalarCudaCallPacket_t(*head_packet));

                // Send to dma via mmio_iface since it is also an MMIO device connected
                // on the same NIC
                out.verbose(CALL_INFO, 1, 0, "Sending Issue_DMA_memcpy_D2H_async request to DMA\n");
                mmio_iface->send(dma_req);
            }
        } else {
            // Check that payload's memcpy params matches
            if (head_packet->cuda_memcpy.count != payload_packet->cuda_memcpy.count ||
                head_packet->cuda_memcpy.src != payload_packet->cuda_memcpy.src ||
                (head_packet->cuda_memcpy.kind != cudaMemcpyDeviceToHost)) {
                out.fatal(CALL_INFO, -1, "memcpy_D2H_done: Head packet does not match with the callback's payload's dst/count/src/kind\n");
            }

            // This is a blocking memcpy, we should not have another cuda call coming
            if (!has_blocked_response) {
                out.fatal(CALL_INFO, -1, "memcpy_D2H_done: expecting a blocked response for sync memcpy\n");
            }
            if (!isSameBalarCudaCallPacket(head_packet, &last_packet, true)) {
                out.fatal(CALL_INFO, -1, "memcpy_D2H_done: another cuda call get in the way of a blocking memcpy\n");
            }

            // Use DMA engine to send the copy from Simulator to SST memspace
            // Should wait for the memcpy to complete
            // as memcpy is blocking
            // Prepare a request to write the dst data to CPU
            if (head_packet->isSSTmem) {
                std::string cmdString = "Issue_DMA_memcpy_D2H";
                // Prepare DMA config

                DMAEngine::DMAEngineControlRegisters dma_registers;
                dma_registers.sst_mem_addr = head_packet->cuda_memcpy.dst;
                dma_registers.simulator_mem_addr = head_packet->cuda_memcpy.dst_buf;
                dma_registers.data_size = head_packet->cuda_memcpy.count;
                dma_registers.transfer_size = 4;    // 4 bytes per transfer
                dma_registers.dir = DMAEngine::DMA_DIR::SIM_TO_SST; // From SST memspace to simulator memspace

                // Send the DMA request to DMA engine
                std::vector<uint8_t> * payload_ptr = encode_balar_packet<DMAEngine::DMAEngineControlRegisters>(&dma_registers);
                StandardMem::Request* dma_req = new StandardMem::Write(
                    dma_addr,
                    sizeof(DMAEngine::DMAEngineControlRegisters),
                    *payload_ptr);
                delete payload_ptr;

                // Record in hashmap
                requests[dma_req->getID()] = std::make_tuple(getCurrentSimTime(), cmdString, new BalarCudaCallPacket_t(*head_packet));

                // Send to dma via mmio_iface since it is also an MMIO device connected
                // on the same NIC
                out.verbose(CALL_INFO, 1, 0, "Sending Issue_DMA_memcpy_D2H request to DMA\n");
                mmio_iface->send(dma_req);
            } else {
                // Not SST mem space copying, just return
                mmio_iface->send(blocked_response);
                has_blocked_response = false;
            }
        }
    } else if (event_string == "memcpy_to_symbol_done") {
        out.verbose(CALL_INFO, 1, 0, "Prepare memcpyToSymbol finish off\n");

        // Check if the head packet is a cudaMemcpyFromSymbol call
        if (head_packet->cuda_call_id != CUDA_MEMCPY_TO_SYMBOL) {
            out.fatal(CALL_INFO, -1, "memcpy_to_symbol_done: Head packet cuda_call_id %d is not a cudaMemcpyToSymbol\n", head_packet->cuda_call_id);
        }

        // This is a blocking memcpy, we should not have another cuda call coming
        if (!has_blocked_response) {
            out.fatal(CALL_INFO, -1, "memcpy_to_symbol_done: expecting a blocked response for sync memcpy\n");
        }
        if (!isSameBalarCudaCallPacket(head_packet, &last_packet, true)) {
            out.fatal(CALL_INFO, -1, "memcpy_to_symbol_done: another cuda call (%s) get in the way of a blocking memcpy\n", CudaAPIEnumToString(last_packet.cuda_call_id));
        }

        if (last_packet.isSSTmem &&
            (cuda_ret.is_cuda_call_done == false)) {
            // Safely free the source data buffer
            free(head_packet->cuda_memcpy_to_symbol.src_buf);

            // Mark the CUDA call as done
            cuda_ret.is_cuda_call_done = true;
        }

        // Send blocked response
        mmio_iface->send(blocked_response);
        has_blocked_response = false;
    } else if (event_string == "memcpy_from_symbol_done") {
        // Use DMA engine to send the copy from Simulator to SST memspace
        // Should wait for the memcpy to complete
        // as memcpy is blocking
        // Prepare a request to write the dst data to CPU
        out.verbose(CALL_INFO, 1, 0, "Prepare memcpyFromSymbol writes\n");

        // Check if the head packet is a cudaMemcpyFromSymbol call
        if (head_packet->cuda_call_id != CUDA_MEMCPY_FROM_SYMBOL) {
            out.fatal(CALL_INFO, -1, "memcpy_from_symbol_done: Head packet cuda_call_id %d is not a cudaMemcpyFromSymbol\n", head_packet->cuda_call_id);
        }

        // This is a blocking memcpy, we should not have another cuda call coming
        if (!has_blocked_response) {
            out.fatal(CALL_INFO, -1, "memcpy_from_symbol_done: expecting a blocked response for sync memcpy\n");
        }
        if (!isSameBalarCudaCallPacket(head_packet, &last_packet, true)) {
            out.fatal(CALL_INFO, -1, "memcpy_from_symbol_done: another cuda call (%s) get in the way of a blocking memcpy\n", CudaAPIEnumToString(last_packet.cuda_call_id));
        }

        if (head_packet->isSSTmem) {
            std::string cmdString = "Issue_DMA_memcpy_from_symbol";

            // Prepare DMA config
            DMAEngine::DMAEngineControlRegisters dma_registers;
            dma_registers.sst_mem_addr = head_packet->cuda_memcpy_from_symbol.dst;
            dma_registers.simulator_mem_addr = head_packet->cuda_memcpy_from_symbol.dst_buf;
            dma_registers.data_size = head_packet->cuda_memcpy_from_symbol.count;
            dma_registers.transfer_size = 4;    // 4 bytes per transfer
            dma_registers.dir = DMAEngine::DMA_DIR::SIM_TO_SST; // From SST memspace to simulator memspace

            // Send the DMA request to DMA engine
            std::vector<uint8_t> * payload_ptr = encode_balar_packet<DMAEngine::DMAEngineControlRegisters>(&dma_registers);
            StandardMem::Request* dma_req = new StandardMem::Write(
                dma_addr,
                sizeof(DMAEngine::DMAEngineControlRegisters),
                *payload_ptr);
            delete payload_ptr;

            // Record in hashmap
            requests[dma_req->getID()] = std::make_tuple(getCurrentSimTime(), cmdString, new BalarCudaCallPacket_t(*head_packet));

            // Send to dma via mmio_iface since it is also an MMIO device connected
            // on the same NIC
            out.verbose(CALL_INFO, 1, 0, "Sending Issue_DMA_memcpy_from_symbol request to DMA\n");
            mmio_iface->send(dma_req);
        } else {
            // Not SST mem space copying, just return
            mmio_iface->send(blocked_response);
            has_blocked_response = false;
        }
    } else if (event_string == "cudaThreadSynchronize_done") { // Handle cudaThreadSynchronize_done
        // Check if the head packet is a cudaThreadSynchronize call
        if (head_packet->cuda_call_id != CUDA_THREAD_SYNC) {
            out.fatal(CALL_INFO, -1, "cudaThreadSynchronize_done: Head packet cuda_call_id %d is not a cudaThreadSynchronize\n", head_packet->cuda_call_id);
        }

        // This is a blocking memcpy, we should not have another cuda call coming
        if (!has_blocked_response) {
            out.fatal(CALL_INFO, -1, "cudaThreadSynchronize_done: expecting a blocked response for thread sync\n");
        }
        if (!isSameBalarCudaCallPacket(head_packet, &last_packet, true)) {
            out.fatal(CALL_INFO, -1, "cudaThreadSynchronize_done: another cuda call (%s) get in the way of thread sync\n", CudaAPIEnumToString(last_packet.cuda_call_id));
        }

        mmio_iface->send(blocked_response);
        has_blocked_response = false;

        // Mark the CUDA call as done
        cuda_ret.is_cuda_call_done = true;
    } else if (event_string == "cudaStreamSynchronize_done") { // Handle a requested stream sync
        // Check if the head packet is a cudaThreadSynchronize call
        if (head_packet->cuda_call_id != CUDA_STREAM_SYNC) {
            out.fatal(CALL_INFO, -1, "cudaStreamSynchronize_done: Head packet cuda_call_id %d is not a cudaStreamSynchronize\n", head_packet->cuda_call_id);
        }

        // Check if the stream we are waiting for is returning
        if (head_packet->cudaStreamSynchronize.stream != stream) {
            out.fatal(CALL_INFO, -1, "cudaStreamSynchronize_done: Head packet is not the cudaStreamSynchronize requested\n");
        }

        // This is a blocking memcpy, we should not have another cuda call coming
        if (!has_blocked_response) {
            out.fatal(CALL_INFO, -1, "cudaStreamSynchronize_done: expecting a blocked response for stream sync\n");
        }
        if (!isSameBalarCudaCallPacket(head_packet, &last_packet, true)) {
            out.fatal(CALL_INFO, -1, "cudaStreamSynchronize_done: another cuda call (%s) get in the way of stream sync\n", CudaAPIEnumToString(last_packet.cuda_call_id));
        }

        mmio_iface->send(blocked_response);
        has_blocked_response = false;

        // Mark the CUDA call as done
        cuda_ret.is_cuda_call_done = true;
    } else if (event_string == "cudaEventSynchronize_done") { // Handle cudaEventSynchronize_done
        if (payload_size != sizeof(BalarCudaCallPacket_t)) {
            out.fatal(CALL_INFO, -1, "cudaEventSynchronize_done: Invalid payload size %ld\n", payload_size);
        }
        BalarCudaCallPacket_t * payload_packet = (BalarCudaCallPacket_t *) payload;

        // Check if the head packet is a cudaEventSynchronize call
        if (head_packet->cuda_call_id != CUDA_EVENT_SYNCHRONIZE) {
            out.fatal(CALL_INFO, -1, "cudaEventSynchronize_done: Head packet cuda_call_id %d is not a cudaEventSynchronize\n", head_packet->cuda_call_id);
        }

        // Check if the payload packet is a cudaEventSynchronize call
        if (payload_packet->cuda_call_id != CUDA_EVENT_SYNCHRONIZE) {
            out.fatal(CALL_INFO, -1, "cudaEventSynchronize_done: payload_packet packet cuda_call_id %d is not a cudaEventSynchronize\n", head_packet->cuda_call_id);
        }

        // Check that payload's memcpy params matches
        if (!isSameBalarCudaCallPacket(head_packet, payload_packet, true)) {
            out.fatal(CALL_INFO, -1, "cudaEventSynchronize_done: Head packet does not match with the callback's payload\n");
        }

        // This is a blocking memcpy, we should not have another cuda call coming
        if (!has_blocked_response) {
            out.fatal(CALL_INFO, -1, "cudaEventSynchronize_done: expecting a blocked response for event sync\n");
        }
        if (!isSameBalarCudaCallPacket(head_packet, &last_packet, true)) {
            out.fatal(CALL_INFO, -1, "cudaEventSynchronize_done: another cuda call (%s) get in the way of event sync\n", CudaAPIEnumToString(last_packet.cuda_call_id));
        }

        // For the event we are waiting for
        // Send the blocked response to stop Vanadis from polling
        mmio_iface->send(blocked_response);
        has_blocked_response = false;

        // Mark the CUDA call as done
        cuda_ret.is_cuda_call_done = true;
    } else if (event_string == "Kernel_done") {
        // Check if the head packet is a cudaLaunch call
        if (head_packet->cuda_call_id != CUDA_LAUNCH) {
            out.fatal(CALL_INFO, -1, "Kernel_done: Head packet cuda_call_id %d is not a cudaLaunch\n", head_packet->cuda_call_id);
        }

        // CUDA kernel dont need any callback handling, we
        // will just remove it from the stream queue
        // for the blocking issue check
    } else {
        out.fatal(CALL_INFO, -1, "Unknown event %s from GPGPU-Sim\n", event_string.c_str());
    }

    out.verbose(CALL_INFO, 1, 0, "Popping a packet %s from GPGPU-Sim at stream %p\n", CudaAPIEnumToString(head_packet->cuda_call_id), stream);

    // Pop the head packet
    // but dont free the buffer yet for DMA, wait til DMA is done to free it
    stream_queue.pop();
}

bool BalarMMIO::clockTic(Cycle_t cycle) {
    bool done = SST_gpu_core_cycle();
    return done;
}

void BalarMMIO::handleEvent(StandardMem::Request* req) {
    // incoming CPU request, handle using BalarHandlers
    req->handle(handlers);
}

/**
 * @brief Handle a request we send to cache backend
 *        return the memory fetch object back to GPGPUSim
 *        after the request return from cache backend
 *
 * @param req
 */
void BalarMMIO::handleGPUCache(SST::Interfaces::StandardMem::Request* req) {
    // Handle cache request/response?
    SST::Interfaces::StandardMem::Request::id_t req_id = req->getID();
    auto find_entry = gpuCachePendingTransactions->find(req_id);

    if(find_entry != gpuCachePendingTransactions->end()) {
        cache_req_params req_params = find_entry->second;
        gpuCachePendingTransactions->erase(find_entry);
        assert(numPendingCacheTransPerCore[req_params.core_id] > 0);
        numPendingCacheTransPerCore[req_params.core_id]--;
        SST_receive_mem_reply(req_params.core_id,  req_params.mem_fetch_pointer);
        delete req;
    } else {
        out.fatal(CALL_INFO, -1, "GPU Cache Request (%ld) not found!\n", req_id);
    }
}

/**
 * @brief Handler for incoming Write requests via `mmio_iface`
 *        the payload will be an address to the actual cuda call packet
 *        the function will save the Write object and issue a DMA request
 *        to fetch the cuda call packet (via `mmio_iface`).
 *        The actual calling to GPGPU-Sim will be done in WriteResp handler
 *
 * @param write
 */
void BalarMMIO::BalarHandlers::handle(SST::Interfaces::StandardMem::Write* write) {
    out->verbose(_INFO_, "%s: receiving incoming write (%ld) to vaddr: %lx and paddr: %lx with size %ld\n", balar->getName().c_str(), write->getID(), write->vAddr, write->pAddr, write->size);

    // Save this write instance as we will need it to make response
    // when finish calling GPGPUSim after getting readresp
    if (balar->pending_write != nullptr)
        out->fatal(CALL_INFO, -1, "%s: Getting overlapping writes to balar! Incoming (%ld) Existing (%ld)!\n", balar->getName().c_str(), write->getID(), balar->pending_write->getID());
    balar->pending_write = write;

    // The write data is a scratch memory address to the real cuda call packet
    // Convert this into an address, assume little endian
    balar->packet_scratch_mem_addr = dataToUInt64(&(write->data));

    // On calling a new cuda call, we set the cuda return packet status
    // to be not done so that our CUDA runtime lib will sync the cudaMemcpy
    balar->cuda_ret.is_cuda_call_done = false;

    // Create a DMA request to read the cuda call packet from cache to balar
    DMAEngine::DMAEngineControlRegisters dma_registers;
    dma_registers.sst_mem_addr = balar->packet_scratch_mem_addr;
    dma_registers.simulator_mem_addr = (uint8_t *)&(balar->last_packet);
    dma_registers.data_size = sizeof(BalarCudaCallPacket_t);
    dma_registers.transfer_size = 4;    // 4 bytes per transfer
    dma_registers.dir = DMAEngine::DMA_DIR::SST_TO_SIM; // From SST memspace to simulator memspace

    // Handle the cmdString in Writeresp handler
    std::string cmdString = "Read_cuda_packet";

    // Send the DMA request to DMA engine
    std::vector<uint8_t> * payload_ptr = encode_balar_packet<DMAEngine::DMAEngineControlRegisters>(&dma_registers);
    StandardMem::Request* dma_req = new StandardMem::Write(
        balar->dma_addr,
        sizeof(DMAEngine::DMAEngineControlRegisters),
        *payload_ptr);
    delete payload_ptr;

    // Record the read request we made, the associated cuda packet haven't existed yet
    balar->requests[dma_req->getID()] = std::make_tuple(balar->getCurrentSimTime(), cmdString, nullptr);

    // Log this write
    out->verbose(_INFO_, "%s: receiving incoming write with scratch mem address: 0x%lx, issuing read to this address\n", balar->getName().c_str(), balar->packet_scratch_mem_addr);

    // Now send the DMA command to read
    balar->mmio_iface->send(dma_req);
}

/**
 * @brief Handler for return value query via `mmio_iface`.
 *        Will save the read as pending read. Then it will issue a write
 *        request to save the return packet to the previously passed in
 *        memory address in DMA request (via `mmio_iface`).
 *        Response to CPU will be done in handler for WriteResp
 *
 * @param read
 */
void BalarMMIO::BalarHandlers::handle(SST::Interfaces::StandardMem::Read* read) {
    out->verbose(_INFO_, "%s: receiving incoming read (%ld) to vaddr: %lx and paddr: %lx with size %ld\n", balar->getName().c_str(), read->getID(), read->vAddr, read->pAddr, read->size);

    out->verbose(_INFO_, "Handling Read for return value for a %s request\n", CudaAPIEnumToString(balar->cuda_ret.cuda_call_id));

    // Save this write instance as we will need it to make response
    // when finish writing return packet to memory
    if (balar->pending_read != nullptr)
        out->fatal(CALL_INFO, -1, "%s: Getting overlapping reads to balar! Incoming (%ld) Existing (%ld)!\n", balar->getName().c_str(), read->getID(), balar->pending_read->getID());
    balar->pending_read = read;

    DMAEngine::DMAEngineControlRegisters dma_registers;
    dma_registers.sst_mem_addr = balar->packet_scratch_mem_addr;
    dma_registers.simulator_mem_addr = (uint8_t *)&(balar->cuda_ret);
    dma_registers.data_size = sizeof(BalarCudaCallReturnPacket_t);
    dma_registers.transfer_size = 4;    // 4 bytes per transfer
    dma_registers.dir = DMAEngine::DMA_DIR::SIM_TO_SST; // From simulator memspace to SST memspace

    // Handle the cmdString in Writeresp handler
    std::string cmdString = "Write_cuda_ret";

    // Send the DMA request to DMA engine
    std::vector<uint8_t> * payload_ptr = encode_balar_packet<DMAEngine::DMAEngineControlRegisters>(&dma_registers);
    StandardMem::Request* dma_req = new StandardMem::Write(
        balar->dma_addr,
        sizeof(DMAEngine::DMAEngineControlRegisters),
        *payload_ptr);
    delete payload_ptr;

    // Record this request
    balar->requests[dma_req->getID()] = std::make_tuple(balar->getCurrentSimTime(), cmdString, nullptr);

    // Log this read
    out->verbose(_INFO_, "%s: receiving incoming read, writing cuda return packet to scratch mem address: 0x%lx\n", balar->getName().c_str(), balar->packet_scratch_mem_addr);

    // Now send the DMA command to copy return packet to SST cache/mem system
    balar->mmio_iface->send(dma_req);
}

/**
 * @brief Handler for the previous read request that get the cuda call packet.
 *        Right now is not used.
 *
 * @param resp
 */
void BalarMMIO::BalarHandlers::handle(SST::Interfaces::StandardMem::ReadResp* resp) {
    out->verbose(_INFO_, "%s: receiving incoming readresp (%ld) to vaddr: %lx and paddr: %lx with size %ld\n", balar->getName().c_str(), resp->getID(), resp->vAddr, resp->pAddr, resp->size);
    // Classify the read request type
    auto i = balar->requests.find(resp->getID());
    if (balar->requests.end() == i) {
        out->fatal(CALL_INFO, -1, "Event (%ld) not found!\n", resp->getID());
    } else {
        std::string request_type = std::get<1>((i)->second);
        out->verbose(_INFO_, "%s: get response from read request (%ld) with type: %s\n", balar->getName().c_str(), resp->getID(), request_type.c_str());

        // Unknown cmdstring, abort
        out->fatal(CALL_INFO, -1, "%s: Unknown read request (%ld): %s!\n", balar->getName().c_str(), resp->getID(), request_type.c_str());

        // Delete the request
        balar->requests.erase(i);
    }
}


/**
 * @brief Handler for a write we made (via `mmio_iface`), which includes
 *        read/writing of cuda packets and cuda memcpy.
 *        It will then use the pending read request to make request telling
 *        CPU the return value is ready (via `mmio_iface`).
 *
 * @param resp
 */
void BalarMMIO::BalarHandlers::handle(SST::Interfaces::StandardMem::WriteResp* resp) {
    out->verbose(_INFO_, "%s: receiving incoming writeresp (%ld) to vaddr: %lx and paddr: %lx with size %ld\n", balar->getName().c_str(), resp->getID(), resp->vAddr, resp->pAddr, resp->size);

    auto i = balar->requests.find(resp->getID());
    if (balar->requests.end() == i ) {
        out->fatal(CALL_INFO, -1, "Event (%ld) not found!\n", resp->getID());
    } else {
        std::string request_type = std::get<1>((i)->second);
        BalarCudaCallPacket_t *request_associated_packet = std::get<2>((i)->second);
        out->verbose(_INFO_, "%s: get response from write request (%ld) with type: %s\n", balar->getName().c_str(), resp->getID(), request_type.c_str());

        // Dispatch based on request type
        if (request_type.compare("Read_cuda_packet") == 0) {
            // This is a response to a request we send to cache
            // to read the cuda packet

            // Whether the request is blocking on issue
            balar->has_blocked_issue = false;
            // Whether the request is blocking on completion
            balar->has_blocked_response = false;

            // Our write instance from CUDA API Call
            StandardMem::Write* write = balar->pending_write;

            // Get cuda call arguments from the last_packet, which is the
            // simulator space pointer we passed to DMA engine
            BalarCudaCallPacket_t * packet = &(balar->last_packet);

            out->verbose(_INFO_, "Handling CUDA API Call (%d). Enum is %s\n", packet->cuda_call_id, CudaAPIEnumToString(packet->cuda_call_id));

            // Save the call type for return packet
            balar->cuda_ret.cuda_call_id = packet->cuda_call_id;

            // Most of the CUDA api will return immediately
            // except for memcpy and threadsync
            balar->cuda_ret.is_cuda_call_done = true;

            // The following are for non-SST memspace
            switch (packet->cuda_call_id) {
                case CUDA_REG_FAT_BINARY: {
                        balar->cuda_ret.cuda_error = cudaSuccess;
                        // Overwrite filename if given CUDA program path from config script
                        if (balar->cudaExecutable.empty())
                            balar->cuda_ret.fat_cubin_handle = (uint64_t) __cudaRegisterFatBinarySST(packet->register_fatbin.file_name);
                        else
                            balar->cuda_ret.fat_cubin_handle = (uint64_t) __cudaRegisterFatBinarySST((char *)balar->cudaExecutable.c_str());
                    }
                    break;
                case CUDA_REG_FUNCTION: {
                        // hostFun should be a fixed pointer value to each kernel function
                        __cudaRegisterFunctionSST(packet->register_function.fatCubinHandle,
                                                packet->register_function.hostFun,
                                                packet->register_function.deviceFun);
                    }
                    break;
                case CUDA_MEMCPY: {
                        // Block on issue
                        if (balar->isStreamBlocking(0)) {
                            balar->has_blocked_issue = true;
                            break;
                        }

                        // cudaMemcpy require callback response from GPGPUSim
                        // insert into default stream's pending packet queue
                        BalarCudaCallPacket_t * packet_copy = new BalarCudaCallPacket_t(*packet);
                        balar->pending_packets_per_stream.at(0).push(packet_copy);

                        // Handle a special case where we have a memcpy
                        // within SST memory space
                        balar->has_blocked_response = true;
                        if (packet_copy->isSSTmem) {
                            // With SST memory/vanadis, we should sync for it to complete
                            balar->cuda_ret.is_cuda_call_done = false;
                            if (packet_copy->cuda_memcpy.kind == cudaMemcpyHostToDevice) {
                                // We will end the handler early as we
                                // need to read the src data in host mem first
                                // and then proceed

                                // Assign device dst buffer
                                size_t data_size = packet_copy->cuda_memcpy.count;
                                packet_copy->cuda_memcpy.src_buf = (uint8_t *) calloc(data_size, sizeof(uint8_t));

                                DMAEngine::DMAEngineControlRegisters dma_registers;
                                dma_registers.sst_mem_addr = packet->cuda_memcpy.src;
                                dma_registers.simulator_mem_addr = packet_copy->cuda_memcpy.src_buf;
                                dma_registers.data_size = data_size;
                                dma_registers.transfer_size = 4;    // 4 bytes per transfer
                                dma_registers.dir = DMAEngine::DMA_DIR::SST_TO_SIM; // From SST memspace to simulator memspace

                                // Handle the cmdString in Writeresp handler
                                std::string cmdString = "Issue_DMA_memcpy_H2D";

                                // Send the DMA request to DMA engine
                                std::vector<uint8_t> * payload_ptr = encode_balar_packet<DMAEngine::DMAEngineControlRegisters>(&dma_registers);
                                StandardMem::Request* dma_req = new StandardMem::Write(
                                    balar->dma_addr,
                                    sizeof(DMAEngine::DMAEngineControlRegisters),
                                    *payload_ptr);
                                delete payload_ptr;

                                // Record this request for later reference
                                balar->requests[dma_req->getID()] = std::make_tuple(balar->getCurrentSimTime(), cmdString, new BalarCudaCallPacket_t(*packet_copy));

                                // Send the DMA request to the engine
                                // Since we used MMIO for DMA engine, one interface is enough
                                balar->mmio_iface->send(dma_req);

                                // Delete the request
                                balar->requests.erase(i);

                                // Delete the resp
                                delete resp;
                                return;
                            } else if (packet->cuda_memcpy.kind == cudaMemcpyDeviceToHost) {
                                // Create a dst buffer to hold the dst data
                                // The write to the SST memspace will be created
                                // once the gpgpusim finish cpy data in the callback
                                // `SST_callback_memcpy_D2H_done`
                                // Assign buffer space
                                packet_copy->cuda_memcpy.dst_buf = (uint8_t *) calloc(packet_copy->cuda_memcpy.count, sizeof(uint8_t));

                                // Call memcpy
                                balar->cuda_ret.cuda_error = cudaMemcpy(
                                        (void *) packet_copy->cuda_memcpy.dst_buf,
                                        (const void*) packet_copy->cuda_memcpy.src,
                                        packet_copy->cuda_memcpy.count,
                                        packet_copy->cuda_memcpy.kind);

                                // These two fields are for non SST mem use only
                                // for comparing the results
                                balar->cuda_ret.cudamemcpy.sim_data = NULL;
                                balar->cuda_ret.cudamemcpy.real_data = NULL;

                                balar->cuda_ret.cudamemcpy.size = packet_copy->cuda_memcpy.count;
                                balar->cuda_ret.cudamemcpy.kind = packet_copy->cuda_memcpy.kind;
                            } else {
                                out->fatal(CALL_INFO, -1, "%s: unsupported memcpy kind! Get: %d\n", balar->getName().c_str(), packet->cuda_memcpy.kind);
                            }
                        } else {
                            balar->cuda_ret.cuda_error = cudaMemcpy(
                                    (void *) packet_copy->cuda_memcpy.dst,
                                    (const void*) packet_copy->cuda_memcpy.src,
                                    packet_copy->cuda_memcpy.count,
                                    packet_copy->cuda_memcpy.kind);

                            // Payload contains the pointer actual data from hw trace run
                            // dst is the pointer to the simulation cpy
                            balar->cuda_ret.cudamemcpy.sim_data = (volatile uint8_t*) packet->cuda_memcpy.dst;
                            balar->cuda_ret.cudamemcpy.real_data = (volatile uint8_t*) packet->cuda_memcpy.payload;
                            balar->cuda_ret.cudamemcpy.size = packet_copy->cuda_memcpy.count;
                            balar->cuda_ret.cudamemcpy.kind = packet_copy->cuda_memcpy.kind;
                        }
                    }
                    break;
                case CUDA_MEMCPY_TO_SYMBOL: {
                        // Block on issue
                        if (balar->isStreamBlocking(0)) {
                            balar->has_blocked_issue = true;
                            break;
                        }

                        // cudaMemcpyToSymbol require callback response from GPGPUSim
                        // insert into default stream's pending packet queue
                        BalarCudaCallPacket_t * packet_copy = new BalarCudaCallPacket_t(*packet);
                        balar->pending_packets_per_stream.at(0).push(packet_copy);

                        balar->has_blocked_response = true;
                        if (packet_copy->isSSTmem) {
                            // With SST memory/vanadis, we should sync for it to complete
                            balar->cuda_ret.is_cuda_call_done = false;

                            // Create a buffer to hold the data to be read from
                            // Vanadis's memory space
                            size_t data_size = packet_copy->cuda_memcpy_to_symbol.count;

                            // Reusing memcpyH2D_dst member
                            packet_copy->cuda_memcpy_to_symbol.src_buf = (uint8_t *) calloc(data_size, sizeof(uint8_t));

                            DMAEngine::DMAEngineControlRegisters dma_registers;
                            dma_registers.sst_mem_addr = packet_copy->cuda_memcpy_to_symbol.src;
                            dma_registers.simulator_mem_addr = packet_copy->cuda_memcpy_to_symbol.src_buf;
                            dma_registers.data_size = data_size;
                            dma_registers.transfer_size = 4;    // 4 bytes per transfer
                            dma_registers.dir = DMAEngine::DMA_DIR::SST_TO_SIM; // From SST memspace to simulator memspace

                            // Handle the cmdString in Writeresp handler
                            std::string cmdString = "Issue_DMA_memcpy_to_symbol";

                            // Send the DMA request to DMA engine
                            std::vector<uint8_t> * payload_ptr = encode_balar_packet<DMAEngine::DMAEngineControlRegisters>(&dma_registers);
                            StandardMem::Request* dma_req = new StandardMem::Write(
                                balar->dma_addr,
                                sizeof(DMAEngine::DMAEngineControlRegisters),
                                *payload_ptr);
                            delete payload_ptr;

                            // Record this request for later reference
                            balar->requests[dma_req->getID()] = std::make_tuple(balar->getCurrentSimTime(), cmdString, new BalarCudaCallPacket_t(*packet_copy));

                            // Send the DMA request to the engine
                            // Since we used MMIO for DMA engine, one interface is enough
                            balar->mmio_iface->send(dma_req);

                            // Delete the request
                            balar->requests.erase(i);

                            // Delete the resp
                            delete resp;
                            return;
                        } else {
                            // CPU using same memory space as simulator
                            // Can directly call API without reading via DMA
                            balar->cuda_ret.cuda_error = cudaMemcpyToSymbol(
                                    (const char *) packet_copy->cuda_memcpy_to_symbol.symbol,
                                    (const void *) packet_copy->cuda_memcpy_to_symbol.src,
                                    packet_copy->cuda_memcpy_to_symbol.count,
                                    packet_copy->cuda_memcpy_to_symbol.offset,
                                    packet_copy->cuda_memcpy_to_symbol.kind);
                        }
                    }
                    break;
                case CUDA_MEMCPY_FROM_SYMBOL: {
                        // Block on issue
                        if (balar->isStreamBlocking(0)) {
                            balar->has_blocked_issue = true;
                            break;
                        }

                        // cudaMemcpyFromSymbol require callback response from GPGPUSim
                        // insert into default stream's pending packet queue
                        BalarCudaCallPacket_t * packet_copy = new BalarCudaCallPacket_t(*packet);
                        balar->pending_packets_per_stream.at(0).push(packet_copy);

                        if (packet->isSSTmem) {
                            packet_copy->cuda_memcpy_from_symbol.dst_buf = (uint8_t *) calloc(packet->cuda_memcpy_from_symbol.count, sizeof(uint8_t));
                            balar->cuda_ret.cuda_error = cudaMemcpyFromSymbol(
                                    (void *) packet_copy->cuda_memcpy_from_symbol.dst_buf,
                                    (const char *) packet_copy->cuda_memcpy_from_symbol.symbol,
                                    packet_copy->cuda_memcpy_from_symbol.count,
                                    packet_copy->cuda_memcpy_from_symbol.offset,
                                    packet_copy->cuda_memcpy_from_symbol.kind);
                        } else {
                            balar->cuda_ret.cuda_error = cudaMemcpyFromSymbol(
                                    (void *) packet_copy->cuda_memcpy_from_symbol.dst,
                                    (const char *) packet_copy->cuda_memcpy_from_symbol.symbol,
                                    packet_copy->cuda_memcpy_from_symbol.count,
                                    packet_copy->cuda_memcpy_from_symbol.offset,
                                    packet_copy->cuda_memcpy_from_symbol.kind);
                        }
                    }
                    break;
                case CUDA_CONFIG_CALL:
                    {
                        // Brackets for var visibity issue, see
                        // https://stackoverflow.com/questions/5685471/error-jump-to-case-label-in-switch-statement
                        dim3 gridDim;
                        dim3 blockDim;
                        gridDim.x = packet->configure_call.gdx;
                        gridDim.y = packet->configure_call.gdy;
                        gridDim.z = packet->configure_call.gdz;
                        blockDim.x = packet->configure_call.bdx;
                        blockDim.y = packet->configure_call.bdy;
                        blockDim.z = packet->configure_call.bdz;
                        balar->cuda_ret.cuda_error = cudaConfigureCall(
                            gridDim,
                            blockDim,
                            packet->configure_call.sharedMem,
                            (cudaStream_t) packet->configure_call.stream
                        );
                        //  Push the launch stream info a stack
                        // Similar to ctx->api->g_cuda_launch_stack during GPGPU-Sim's cudaLaunch
                        // So we can know which stream the kernel
                        // launch belongs to for managing
                        // blocking and non-blocking launch
                        balar->cudalaunch_stream_stack.push(packet->configure_call.stream);
                    }
                    break;
                case CUDA_SET_ARG:
                    balar->cuda_ret.cuda_error = cudaSetupArgumentSST(
                        packet->setup_argument.arg,
                        packet->setup_argument.value,
                        packet->setup_argument.size,
                        packet->setup_argument.offset
                    );
                    break;
                case CUDA_LAUNCH:
                    {
                    // Peek at the stream info from the stack
                    cudaStream_t config_stream = balar->cudalaunch_stream_stack.top();
                    // Check if this launch is blocking
                    if (balar->isStreamBlocking(config_stream)) {
                        // Mark as a blocked issue, will simply return with
                        // cudaErrorNotReady and let the caller retry
                        balar->has_blocked_issue = true;
                        break;
                    }

                    // If stream is drained or we are on nonblocking stream, launch the kernel
                    balar->cuda_ret.cuda_error = cudaLaunchSST(packet->cuda_launch.func);

                    // Also push this cuda launch to our pending_packets_per_stream if
                    // launch succeed
                    if (balar->cuda_ret.cuda_error == cudaSuccess) {
                        BalarCudaCallPacket_t * packet_copy = new BalarCudaCallPacket_t(*packet);
                        balar->pending_packets_per_stream.at(config_stream).push(packet_copy);
                    } else {
                        out->verbose(_INFO_, "%s: cudaLaunch failed with error %d\n", balar->getName().c_str(), balar->cuda_ret.cuda_error);
                    }

                    // Pop the stream info from the stack
                    balar->cudalaunch_stream_stack.pop();
                    }
                    break;
                case CUDA_FREE:
                    balar->cuda_ret.cuda_error = cudaFree(packet->cuda_free.devPtr);
                    break;
                case CUDA_GET_LAST_ERROR:
                    balar->cuda_ret.cuda_error = cudaGetLastError();
                    break;
                case CUDA_MALLOC: {
                        balar->cuda_ret.cudamalloc.devptr_addr = (uint64_t) packet->cuda_malloc.devPtr;
                        balar->cuda_ret.cuda_error = cudaSuccess;
                        balar->cuda_ret.cudamalloc.malloc_addr = cudaMallocSST(
                            packet->cuda_malloc.devPtr,
                            packet->cuda_malloc.size
                        );
                    }
                    break;
                case CUDA_REG_VAR: {
                        balar->cuda_ret.cuda_error = cudaSuccess;
                        __cudaRegisterVar(
                            packet->register_var.fatCubinHandle,
                            packet->register_var.hostVar,
                            packet->register_var.deviceName,
                            packet->register_var.deviceName,
                            packet->register_var.ext,
                            packet->register_var.size,
                            packet->register_var.constant,
                            packet->register_var.global
                        );
                    }
                    break;
                case CUDA_MAX_BLOCK: {
                        balar->cuda_ret.cuda_error = cudaOccupancyMaxActiveBlocksPerMultiprocessorWithFlags(
                            (int *) (packet->max_active_block.numBlock),
                            (const char *) (packet->max_active_block.hostFunc),
                            packet->max_active_block.blockSize,
                            packet->max_active_block.dynamicSMemSize,
                            packet->max_active_block.flags
                        );
                    }
                    break;
                case CUDA_PARAM_CONFIG: {
                        std::tuple<cudaError_t, size_t, unsigned> res = SST_cudaGetParamConfig(packet->cudaparamconfig.hostFun, packet->cudaparamconfig.index);
                        balar->cuda_ret.cuda_error = std::get<0>(res);
                        balar->cuda_ret.cudaparamconfig.size = std::get<1>(res);
                        balar->cuda_ret.cudaparamconfig.alignment = std::get<2>(res);
                    }
                    break;
                case CUDA_THREAD_SYNC: {
                        // Should always succeed
                        balar->cuda_ret.cuda_error = cudaSuccess;
                        cudaError_t result = cudaThreadSynchronizeSST();

                        // Check if need to make this a blocked response
                        // if so, mark this and defer the completion
                        // after GPGPU-Sim notifies us it is done with
                        // cudaThreadSynchronize()
                        if (result != cudaSuccess) {
                            // cudaThreadSynchronize require callback response from GPGPUSim
                            // insert into default stream's pending packet queue
                            BalarCudaCallPacket_t * packet_copy = new BalarCudaCallPacket_t(*packet);
                            balar->pending_packets_per_stream.at(0).push(packet_copy);

                            // Mark this is now blocking
                            balar->has_blocked_response = true;
                            // Mark this so that vanadis can poll for completion
                            balar->cuda_ret.is_cuda_call_done = false;
                        }
                    }
                    break;
                case CUDA_MEMSET: {
                        balar->cuda_ret.cuda_error = cudaMemset(
                            packet->cudamemset.mem,
                            packet->cudamemset.c,
                            packet->cudamemset.count
                        );
                    }
                    break;
                case CUDA_GET_DEVICE_COUNT: {
                        balar->cuda_ret.cuda_error = cudaGetDeviceCount(
                            &(balar->cuda_ret.cudagetdevicecount.count)
                        );
                    }
                    break;
                case CUDA_SET_DEVICE: {
                        balar->cuda_ret.cuda_error = cudaSetDevice(
                            packet->cudasetdevice.device
                        );
                    }
                    break;
                case CUDA_REG_TEXTURE: {
                        // Allocate a textureReference for GPGPU-Sim to access
                        struct textureReference *textureRef = (struct textureReference *) malloc(sizeof(struct textureReference));
                        balar->cudaTextureMapping[packet->cudaregtexture.hostVar_ptr] = textureRef;
                        // Copy textureReference
                        *textureRef = packet->cudaregtexture.texRef;

                        balar->cuda_ret.cuda_error = cudaSuccess;
                        __cudaRegisterTexture(
                            packet->cudaregtexture.fatCubinHandle,
                            textureRef,
                            packet->cudaregtexture.deviceAddress,
                            packet->cudaregtexture.deviceName,
                            packet->cudaregtexture.dim,
                            packet->cudaregtexture.norm,
                            packet->cudaregtexture.ext
                        );
                    }
                    break;
                case CUDA_BIND_TEXTURE: {
                        // Access our saved textureRef and copy from Vanadis's packet
                        struct textureReference *textureRef = balar->cudaTextureMapping.at(packet->cudabindtexture.hostVar_ptr);
                        *textureRef = packet->cudabindtexture.texRef;

                        balar->cuda_ret.cuda_error = cudaBindTexture(
                            packet->cudabindtexture.offset,
                            textureRef,
                            packet->cudabindtexture.devPtr,
                            &(packet->cudabindtexture.desc_struct),
                            packet->cudabindtexture.size
                        );
                    }
                    break;
                case CUDA_MALLOC_HOST: {
                        balar->cuda_ret.cuda_error = cudaMallocHostSST(
                            packet->cudamallochost.addr,
                            packet->cudamallochost.size
                        );
                    }
                    break;
                case CUDA_MEMCPY_ASYNC: {
                        // Block on issue
                        if (balar->isStreamBlocking(packet->cudaMemcpyAsync.stream)) {
                            balar->has_blocked_issue = true;
                            break;
                        }

                        // cudaMemcpyAsync require callback response from GPGPUSim
                        // insert into its stream's pending packet queue
                        BalarCudaCallPacket_t * packet_copy = new BalarCudaCallPacket_t(*packet);
                        balar->pending_packets_per_stream.at(packet_copy->cudaMemcpyAsync.stream).push(packet_copy);

                        if (packet->isSSTmem) {
                            if (packet->cudaMemcpyAsync.kind == cudaMemcpyHostToDevice) {
                                // Assign device buffer
                                size_t data_size = packet_copy->cudaMemcpyAsync.count;
                                packet_copy->cudaMemcpyAsync.dst = (uint64_t)(uint8_t *) calloc(data_size, sizeof(uint8_t));

                                DMAEngine::DMAEngineControlRegisters dma_registers;
                                dma_registers.sst_mem_addr = packet->cudaMemcpyAsync.src;
                                dma_registers.simulator_mem_addr = (uint8_t *) packet_copy->cudaMemcpyAsync.dst;
                                dma_registers.data_size = data_size;
                                dma_registers.transfer_size = 4;    // 4 bytes per transfer
                                dma_registers.dir = DMAEngine::DMA_DIR::SST_TO_SIM; // From SST memspace to simulator memspace

                                // Handle the cmdString in Writeresp handler
                                std::string cmdString = "Issue_DMA_memcpy_H2D_async";

                                // Send the DMA request to DMA engine
                                std::vector<uint8_t> * payload_ptr = encode_balar_packet<DMAEngine::DMAEngineControlRegisters>(&dma_registers);
                                StandardMem::Request* dma_req = new StandardMem::Write(
                                    balar->dma_addr,
                                    sizeof(DMAEngine::DMAEngineControlRegisters),
                                    *payload_ptr);
                                delete payload_ptr;

                                // Record this request for later reference
                                balar->requests[dma_req->getID()] = std::make_tuple(balar->getCurrentSimTime(), cmdString, new BalarCudaCallPacket_t(*packet_copy));

                                // Send the DMA request to the engine
                                // Since we used MMIO for DMA engine, one interface is enough
                                balar->mmio_iface->send(dma_req);
                            } else if (packet->cudaMemcpyAsync.kind == cudaMemcpyDeviceToHost) {
                                // Create a dst buffer to hold the dst data
                                packet_copy->cudaMemcpyAsync.dst_buf = (uint8_t *) calloc(packet->cudaMemcpyAsync.count, sizeof(uint8_t));

                                // Call memcpy
                                balar->cuda_ret.cuda_error = cudaMemcpyAsync(
                                        (void *) packet_copy->cudaMemcpyAsync.dst_buf,
                                        (const void*) packet_copy->cudaMemcpyAsync.src,
                                        packet_copy->cudaMemcpyAsync.count,
                                        packet_copy->cudaMemcpyAsync.kind,
                                        packet_copy->cudaMemcpyAsync.stream);
                            } else {
                                out->fatal(CALL_INFO, -1, "%s: unsupported memcpy kind! Get: %d\n", balar->getName().c_str(), packet->cudaMemcpyAsync.kind);
                            }
                        } else {
                            // No need to perform DMA
                            balar->has_blocked_response = false;
                            balar->cuda_ret.cuda_error = cudaMemcpyAsync(
                                    (void *) packet_copy->cudaMemcpyAsync.dst,
                                    (const void*) packet_copy->cudaMemcpyAsync.src,
                                    packet_copy->cudaMemcpyAsync.count,
                                    packet_copy->cudaMemcpyAsync.kind,
                                    packet_copy->cudaMemcpyAsync.stream);
                        }
                    }
                    break;
                case CUDA_GET_DEVICE_PROPERTIES: {
                        balar->cuda_ret.cuda_error = cudaGetDeviceProperties(
                            &(balar->cuda_ret.cudaGetDeviceProperties.prop),
                            packet->cudaGetDeviceProperties.device
                        );
                    }
                    break;
                case CUDA_SET_DEVICE_FLAGS: {
                        balar->cuda_ret.cuda_error = cudaSetDeviceFlagsSST(
                            packet->cudaSetDeviceFlags.flags
                        );
                    }
                    break;
                case CUDA_STREAM_CREATE: {
                        balar->cuda_ret.cuda_error = cudaStreamCreate(
                            &(balar->cuda_ret.cudaStreamCreate.stream)
                        );

                        // Create new stream queue for pending packets
                        balar->pending_packets_per_stream.insert({balar->cuda_ret.cudaStreamCreate.stream, std::queue<BalarCudaCallPacket_t *>()});
                    }
                    break;
                case CUDA_STREAM_SYNC: {
                        // Should always succeed
                        balar->cuda_ret.cuda_error = cudaSuccess;
                        cudaError_t result = cudaStreamSynchronizeSST(packet->cudaStreamSynchronize.stream);

                        // Check if need to make this a blocked response
                        // if so, mark this and defer the completion
                        // after GPGPU-Sim notifies us it is done with
                        // cudaStreamSynchronize()
                        if (result != cudaSuccess) {
                            // cudaStreamSync require callback response from GPGPUSim
                            // insert into default stream's pending packet queue
                            BalarCudaCallPacket_t * packet_copy = new BalarCudaCallPacket_t(*packet);
                            balar->pending_packets_per_stream.at(0).push(packet_copy);

                            // Now this sync op is blocking
                            balar->has_blocked_response = true;
                            // Mark this so that vanadis can poll for completion
                            balar->cuda_ret.is_cuda_call_done = false;
                        }
                    }
                    break;
                case CUDA_STREAM_DESTROY: {
                        // This cudaStreamDestroy should just remove stream
                        // as it assume stream is already synchronized
                        // Which means that before calling this,
                        // cudaStreamSynchronize() should be explicitly
                        // called, otherwise balar will deadlock

                        // Check if the stream queue is drained
                        if (!(balar->pending_packets_per_stream.at(packet->cudaStreamDestroy.stream).empty())) {
                            // Error, stream is not empty
                            out->fatal(CALL_INFO, -1, "Stream %p is not empty when destroyed!\n", packet->cudaStreamDestroy.stream);
                        }

                        balar->cuda_ret.cuda_error = cudaStreamDestroy(
                            packet->cudaStreamDestroy.stream
                        );

                        // Remove stream queue
                        balar->pending_packets_per_stream.erase(packet->cudaStreamDestroy.stream);
                    }
                    break;
                case CUDA_EVENT_CREATE: {
                        balar->cuda_ret.cuda_error = cudaEventCreate(
                            &(balar->cuda_ret.cudaEventCreate.event)
                        );
                    }
                    break;
                case CUDA_EVENT_CREATE_WITH_FLAGS: {
                        balar->cuda_ret.cuda_error = cudaEventCreateWithFlags(
                            &(balar->cuda_ret.cudaEventCreate.event),
                            packet->cudaEventCreateWithFlags.flags
                        );
                    }
                    break;
                case CUDA_EVENT_RECORD: {
                        // Block on issue
                        out->verbose(_INFO_, "Checking for stream blocking for %s on stream: %p\n", CudaAPIEnumToString(packet->cuda_call_id), packet->cudaEventRecord.stream);
                        if (balar->isStreamBlocking(packet->cudaEventRecord.stream)) {
                            balar->has_blocked_issue = true;
                            out->verbose(_INFO_, "Check fails for %s on stream: %p\n", CudaAPIEnumToString(packet->cuda_call_id), packet->cudaEventRecord.stream);
                            break;
                        }
                        out->verbose(_INFO_, "Check passes for %s on stream: %p\n", CudaAPIEnumToString(packet->cuda_call_id), packet->cudaEventRecord.stream);

                        balar->cuda_ret.cuda_error = cudaEventRecord(
                            packet->cudaEventRecord.event,
                            packet->cudaEventRecord.stream
                        );
                    }
                    break;
                case CUDA_EVENT_SYNCHRONIZE: {
                        balar->cuda_ret.cuda_error = cudaEventSynchronizeSST(
                            packet->cudaEventSynchronize.event
                        );

                        if (balar->cuda_ret.cuda_error != cudaSuccess) {
                            // cudaEventSynchronize require callback response from GPGPUSim
                            // insert into default stream's pending packet queue
                            BalarCudaCallPacket_t * packet_copy = new BalarCudaCallPacket_t(*packet);
                            balar->pending_packets_per_stream.at(0).push(packet_copy);

                            // Now this call is blocking as we have to wait for sync to complete
                            balar->has_blocked_response = true;
                            // Mark this so that vanadis can poll for completion
                            balar->cuda_ret.is_cuda_call_done = false;
                        }
                    }
                    break;
                case CUDA_EVENT_ELAPSED_TIME: {
                        balar->cuda_ret.cuda_error = cudaEventElapsedTime(
                            &(balar->cuda_ret.cudaEventElapsedTime.ms),
                            packet->cudaEventElapsedTime.start,
                            packet->cudaEventElapsedTime.end
                        );
                    }
                    break;
                case CUDA_EVENT_DESTROY: {
                        balar->cuda_ret.cuda_error = cudaEventDestroy(
                            packet->cudaEventDestroy.event
                        );
                    }
                    break;
                case CUDA_DEVICE_GET_ATTRIBUTE: {
                        balar->cuda_ret.cuda_error = cudaDeviceGetAttribute(
                            &(balar->cuda_ret.cudaDeviceGetAttribute.value),
                            packet->cudaDeviceGetAttribute.attr,
                            packet->cudaDeviceGetAttribute.device
                        );
                    }
                    break;
                default:
                    out->fatal(CALL_INFO, -1, "Received unknown GPU enum API: %d\n", packet->cuda_call_id);
                    break;
            }

            // Send response (ack) to the CUDA API Cuda request if needed
            if (!(write->posted)) {
                // At most one of these two can be true
                assert(!(balar->has_blocked_issue && balar->has_blocked_response));

                if (balar->has_blocked_response) {
                    // Save blocked req's response and send later
                    balar->blocked_response = write->makeResponse();
                    out->verbose(CALL_INFO, 1, 0, "Handling a request with blocking completion: %s\n", CudaAPIEnumToString(packet->cuda_call_id));

                } else if (balar->has_blocked_issue) {
                    // This request is blocking on issue, so we will
                    // just let caller try again via a cudaErrorNotReady
                    balar->cuda_ret.cuda_error = cudaErrorNotReady;

                    balar->mmio_iface->send(write->makeResponse());
                    out->verbose(CALL_INFO, 1, 0, "Handling a request with blocking issue: %s with error: %d\n", CudaAPIEnumToString(packet->cuda_call_id), balar->cuda_ret.cuda_error);
                } else {
                    balar->mmio_iface->send(write->makeResponse());
                    out->verbose(CALL_INFO, 1, 0, "Handling a non-blocking request: %s with error: %d\n", CudaAPIEnumToString(packet->cuda_call_id), balar->cuda_ret.cuda_error);
                }
            }
            balar->pending_write = nullptr;
            delete write;
            delete resp;
        } else if (request_type.compare("Write_cuda_ret") == 0) {
            // To a write we made to write the CUDA return packet
            // Make a response. Must fill in payload.
            StandardMem::Read* read = balar->pending_read;
            StandardMem::ReadResp* read_resp = static_cast<StandardMem::ReadResp*>(read->makeResponse());

            // Return the scratch memory address as the read result
            out->verbose(_INFO_, "%s: handling previous read request (%ld) for CUDA return packet to vaddr: %lx and paddr: %lx with size %ld at inst: %lx, returning the address of the packet: %lx\n", balar->getName().c_str(), read->getID(), read->vAddr, read->pAddr, read->size, read->iPtr, balar->packet_scratch_mem_addr);

            vector<uint8_t> payload;
            UInt64ToData(balar->packet_scratch_mem_addr, &payload);
            payload.resize(read->size, 0);
            read_resp->data = payload;
            balar->mmio_iface->send(read_resp);


            // Clean pending read
            balar->pending_read = nullptr;
            delete read;
            delete resp;
        } else if (request_type.compare("Issue_DMA_memcpy_D2H") == 0) {
            // To a DMA request we made to store the dst data into SST memory space
            // Send the blocked response aved in the previous
            // request handler for memcpyD2H to notify CPU we are done with memcpyD2H
            // Since we get this request only after all data have been copied into host memory

            // Free temp buffer to hold memcpyD2H data
            out->verbose(_INFO_, "%s: done with a memcpyD2H\n", balar->getName().c_str());

            free(request_associated_packet->cuda_memcpy.dst_buf);
            balar->mmio_iface->send(balar->blocked_response);
            balar->has_blocked_response = false;

            // Assert that this memcpy is done so that the vanadis CPU will
            // able to sync properly
            balar->cuda_ret.is_cuda_call_done = true;

            // destory the copy of cuda call packet
            delete request_associated_packet;
            delete resp;
        } else if (request_type.compare("Issue_DMA_memcpy_D2H_async") == 0) {
            // Free temp buffer to hold memcpyD2HAsync data
            out->verbose(_INFO_, "%s: done with a memcpyD2HAsync\n", balar->getName().c_str());
            free(request_associated_packet->cuda_memcpy.dst_buf);

            // Dont need to send any pending response back to vanadis as
            // this is an async memcpy

            delete request_associated_packet;
            delete resp;
        } else if (request_type.compare("Issue_DMA_memcpy_H2D") == 0) {
            // To a DMA request we made to store the src data from SST memory space to a buffer
            // Now we need to actually perform the memcpy using GPGPUSim method
            StandardMem::Write* write = balar->pending_write;

            // Call with the source data in simulator space
            // balar->memcpyH2D_dst is the buffer pointer where the DMA copied into
            balar->cuda_ret.cuda_error = cudaMemcpy(
                    (void *) request_associated_packet->cuda_memcpy.dst,
                    (const void*) request_associated_packet->cuda_memcpy.src_buf,
                    request_associated_packet->cuda_memcpy.count,
                    request_associated_packet->cuda_memcpy.kind);

            // Payload contains the pointer actual data from hw trace run
            // dst is the pointer to the simulation cpy
            balar->cuda_ret.cudamemcpy.sim_data = (volatile uint8_t*) request_associated_packet->cuda_memcpy.dst;
            balar->cuda_ret.cudamemcpy.real_data = (volatile uint8_t*) request_associated_packet->cuda_memcpy.payload;
            balar->cuda_ret.cudamemcpy.size = request_associated_packet->cuda_memcpy.count;
            balar->cuda_ret.cudamemcpy.kind = request_associated_packet->cuda_memcpy.kind;

            // Save blocked req's response and send later (in SST_callback_memcpy_H2D_done()), since it is a memcpy
            balar->blocked_response = write->makeResponse();
            out->verbose(CALL_INFO, 1, 0, "Handling a blocking request: %s\n", CudaAPIEnumToString(request_associated_packet->cuda_call_id));

            // Delete requests as we don't need them anymore
            balar->pending_write = nullptr;

            // Can safely delete this packet as it is just a copy of the original packet
            // that is in the pending_packets_per_stream
            // plus no indirection in its cuda_memcpy field
            // Also its allocated buffer will be freed once GPGPU-Sim finish memcpy
            delete request_associated_packet;
            delete write;
            delete resp;
        } else if (request_type.compare("Issue_DMA_memcpy_H2D_async") == 0) {
            // Perform the cudaMemcpyAsync
            balar->cuda_ret.cuda_error = cudaMemcpyAsync(
                    (void *) request_associated_packet->cudaMemcpyAsync.dst,
                    (const void*) request_associated_packet->cudaMemcpyAsync.src_buf,
                    request_associated_packet->cudaMemcpyAsync.count,
                    request_associated_packet->cudaMemcpyAsync.kind,
                    request_associated_packet->cudaMemcpyAsync.stream);

            delete request_associated_packet;
            delete resp;
        } else if (request_type.compare("Issue_DMA_memcpy_to_symbol") == 0) {
            // Similar to memcpyH2D
            StandardMem::Write* write = balar->pending_write;

            // Call GPGPU-Sim with copied data
            balar->cuda_ret.cuda_error = cudaMemcpyToSymbol(
                    (const char *) request_associated_packet->cuda_memcpy_to_symbol.symbol,
                    (const void*) request_associated_packet->cuda_memcpy_to_symbol.src_buf,
                    request_associated_packet->cuda_memcpy_to_symbol.count,
                    request_associated_packet->cuda_memcpy_to_symbol.offset,
                    request_associated_packet->cuda_memcpy_to_symbol.kind);

            // Save blocked req's response and send later (in SST_callback_memcpy_to_symbol_done()), since it is a memcpy
            balar->blocked_response = write->makeResponse();
            out->verbose(CALL_INFO, 1, 0, "Handling a blocking request: %s\n", CudaAPIEnumToString(request_associated_packet->cuda_call_id));

            // Delete requests as we don't need them anymore
            balar->pending_write = nullptr;

            delete request_associated_packet;
            delete write;
            delete resp;
        } else if (request_type.compare("Issue_DMA_memcpy_from_symbol") == 0) {
            // Free temp buffer to hold memcpyFromSymbol data
            // Pretty much similar to memcpyD2H
            out->verbose(_INFO_, "%s: done with a memcpyFromSymbol\n", balar->getName().c_str());

            free(request_associated_packet->cuda_memcpy_from_symbol.dst_buf);
            balar->mmio_iface->send(balar->blocked_response);
            balar->has_blocked_response = false;

            // Assert that this memcpy is done so that the vanadis CPU will
            // able to sync properly
            balar->cuda_ret.is_cuda_call_done = true;

            delete request_associated_packet;
            delete resp;
        } else {
            // Unknown cmdstring, abort
            out->fatal(CALL_INFO, -1, "%s: Unknown write request (%ld): %s!\n", balar->getName().c_str(), resp->getID(), request_type.c_str());
        }
        // Delete the request
        balar->requests.erase(i);
    }
}

void BalarMMIO::BalarHandlers::intToData(int32_t num, vector<uint8_t>* data) {
    data->clear();
    for (size_t i = 0; i < sizeof(int); i++) {
        data->push_back(num & 0xFF);
        num >>=8;
    }
}

int32_t BalarMMIO::BalarHandlers::dataToInt(vector<uint8_t>* data) {
    int32_t retval = 0;
    for (int i = data->size(); i > 0; i--) {
        retval <<= 8;
        retval |= (*data)[i-1];
    }
    return retval;
}

void BalarMMIO::BalarHandlers::UInt64ToData(uint64_t num, vector<uint8_t>* data) {
    data->clear();
    for (size_t i = 0; i < sizeof(uint64_t); i++) {
        data->push_back(num & 0xFF);
        num >>=8;
    }
}

uint64_t BalarMMIO::BalarHandlers::dataToUInt64(vector<uint8_t>* data) {
    uint64_t retval = 0;
    for (int i = data->size(); i > 0; i--) {
        retval <<= 8;
        retval |= (*data)[i-1];
    }
    return retval;
}

void BalarMMIO::printStatus(Output &statusOut) {
    statusOut.output("Balar::BalarMMIO %s\n", getName().c_str());
    statusOut.output("End Balar::BalarMMIO\n\n");
}

bool BalarMMIO::isStreamBlocking(cudaStream_t stream) {
    // Stream blocking happens when:
    //   1. We are pushing to default stream (blocking by default)
    //   2. We enable cuda launch blocking in GPGPU-Sim (for other concurrent streams)
    // And there are still ongoing stream operations in all streams
    bool needBlocking = !stream || isLaunchBlocking;
    if (needBlocking) {
        for (auto& [stream, queue] : pending_packets_per_stream) {
            if (!queue.empty()) {
                return true;
            }
        }
    }
    return false;
}

extern bool is_SST_buffer_full(unsigned core_id) {
    assert(g_balarmmio_component);
    return g_balarmmio_component->is_SST_buffer_full(core_id);
    return false;
}

extern void send_read_request_SST(unsigned core_id, uint64_t address, uint64_t size, void* mem_req) {
    assert(g_balarmmio_component);
    g_balarmmio_component->send_read_request_SST(core_id, address, size, mem_req);
}

extern void send_write_request_SST(unsigned core_id, uint64_t address, uint64_t size, void* mem_req) {
    assert(g_balarmmio_component);
    g_balarmmio_component->send_write_request_SST(core_id, address, size, mem_req);
}

/**
 * @brief Signal that the cudaMemcpy with H2D is done, could be from async memcpy
 *
 * @param dst
 * @param src
 * @param count
 */
extern void SST_callback_memcpy_H2D_done(uint64_t dst, uint64_t src, size_t count, cudaStream_t stream) {
    assert(g_balarmmio_component);
    BalarCudaCallPacket_t *callback_packet = new BalarCudaCallPacket_t();

    // default stream memcpy is blocking
    if (stream == 0) {
        callback_packet->cuda_call_id = CUDA_MEMCPY;
        callback_packet->cuda_memcpy.dst = dst;
        callback_packet->cuda_memcpy.src = src;
        callback_packet->cuda_memcpy.count = count;
        callback_packet->cuda_memcpy.kind = cudaMemcpyHostToDevice;
    } else {
        callback_packet->cuda_call_id = CUDA_MEMCPY_ASYNC;
        callback_packet->cudaMemcpyAsync.dst = dst;
        callback_packet->cudaMemcpyAsync.src = src;
        callback_packet->cudaMemcpyAsync.count = count;
        callback_packet->cudaMemcpyAsync.kind = cudaMemcpyHostToDevice;
    }

    g_balarmmio_component->SST_callback_event_done("memcpy_H2D_done", stream, (uint8_t *)callback_packet, sizeof(BalarCudaCallPacket_t));
}

/**
 * @brief Signal that the cudaMemcpy with D2H is done, could be from async memcpy
 *
 * @param dst
 * @param src
 * @param count
 */
extern void SST_callback_memcpy_D2H_done(uint64_t dst, uint64_t src, size_t count, cudaStream_t stream) {
    assert(g_balarmmio_component);
    BalarCudaCallPacket_t *callback_packet = new BalarCudaCallPacket_t();

    // default stream memcpy is blocking
    if (stream == 0) {
        callback_packet->cuda_call_id = CUDA_MEMCPY;
        callback_packet->cuda_memcpy.dst = dst;
        callback_packet->cuda_memcpy.src = src;
        callback_packet->cuda_memcpy.count = count;
        callback_packet->cuda_memcpy.kind = cudaMemcpyDeviceToHost;
    } else {
        callback_packet->cuda_call_id = CUDA_MEMCPY_ASYNC;
        callback_packet->cudaMemcpyAsync.dst = dst;
        callback_packet->cudaMemcpyAsync.src = src;
        callback_packet->cudaMemcpyAsync.count = count;
        callback_packet->cudaMemcpyAsync.kind = cudaMemcpyDeviceToHost;
    }

    g_balarmmio_component->SST_callback_event_done("memcpy_D2H_done", stream, (uint8_t *)callback_packet, sizeof(BalarCudaCallPacket_t));
}

/**
 * @brief Signal that the cudaMemcpyToSymbol is done
 *
 */
extern void SST_callback_memcpy_to_symbol_done() {
    assert(g_balarmmio_component);
    g_balarmmio_component->SST_callback_event_done("memcpy_to_symbol_done", 0, NULL, 0);
}

/**
 * @brief Signal that the cudaMemcpyFromSymbol is done
 *
 */
extern void SST_callback_memcpy_from_symbol_done() {
    assert(g_balarmmio_component);
    g_balarmmio_component->SST_callback_event_done("memcpy_from_symbol_done", 0, NULL, 0);
}

/**
 * @brief Signal that the cudaThreadSynchronize is done
 *
 */
extern void SST_callback_cudaThreadSynchronize_done() {
    assert(g_balarmmio_component);
    g_balarmmio_component->SST_callback_event_done("cudaThreadSynchronize_done", 0, NULL, 0);
}

/**
 * @brief Signal that cudaStreamSynchronize is done for stream
 *
 * @param stream
 */
extern void SST_callback_cudaStreamSynchronize_done(cudaStream_t stream) {
    assert(g_balarmmio_component);
    g_balarmmio_component->SST_callback_event_done("cudaStreamSynchronize_done", stream, NULL, 0);
}


/**
 * @brief Signal that the cudaEventSynchronize is done
 */
extern void SST_callback_cudaEventSynchronize_done(cudaEvent_t event) {
    assert(g_balarmmio_component);
    BalarCudaCallPacket_t *callback_packet = new BalarCudaCallPacket_t();
    callback_packet->cuda_call_id = CUDA_EVENT_SYNCHRONIZE;
    callback_packet->cudaEventSynchronize.event = event;

    g_balarmmio_component->SST_callback_event_done("cudaEventSynchronize_done", 0, (uint8_t *)callback_packet, sizeof(BalarCudaCallPacket_t));
}

/**
 * @brief Signal that a kernel has finished
 *
 * @param stream : stream of the finished kernel
 */
extern void SST_callback_kernel_done(cudaStream_t stream) {
    assert(g_balarmmio_component);
    g_balarmmio_component->SST_callback_event_done("Kernel_done", stream, NULL, 0);
}