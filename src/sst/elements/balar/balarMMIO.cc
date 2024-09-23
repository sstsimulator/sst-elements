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

#include <sst_config.h>
#include "balarMMIO.h"
#include "dmaEngine.h"
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
    registerClock(tc, new Clock::Handler<BalarMMIO>(this, &BalarMMIO::clockTic));

    // Link names
    char* link_buffer = (char*) malloc(sizeof(char) * 256);
    char* link_buffer_mem = (char*) malloc(sizeof(char) * 256);
    char* link_cache_buffer = (char*) malloc(sizeof(char) * 256);

    // Buffer to keep track of pending transactions
    gpuCachePendingTransactions = new std::unordered_map<StandardMem::Request::id_t, cache_req_params>();

    // Interface to CPU via MMIO
    mmio_iface = loadUserSubComponent<SST::Interfaces::StandardMem>("mmio_iface", ComponentInfo::SHARE_NONE, tc, 
            new StandardMem::Handler<BalarMMIO>(this, &BalarMMIO::handleEvent));
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
            gpu_to_cache_links[i] = gpu_to_cache->create<Interfaces::StandardMem>(i, ComponentInfo::INSERT_STATS, tc, new StandardMem::Handler<BalarMMIO>(this, &BalarMMIO::handleGPUCache));
        } else {
    	    // Create a unique name for all links, the configure file links need to match this
    	    sprintf(link_cache_buffer, "requestGPUCacheLink%" PRIu32, i);
            out.verbose(CALL_INFO, 1, 0, "Starts to configure cache link (%s) for core %d\n", link_cache_buffer, i);
            Params param;
            param.insert("port", link_cache_buffer);

            gpu_to_cache_links[i] = loadAnonymousSubComponent<SST::Interfaces::StandardMem>("memHierarchy.standardInterface", 
                    "gpu_cache", i, ComponentInfo::SHARE_PORTS | ComponentInfo::INSERT_STATS, 
                    param, tc, new StandardMem::Handler<BalarMMIO>(this, &BalarMMIO::handleGPUCache));
            out.verbose(CALL_INFO, 1, 0, "Finish configure cache link for core %d\n", i);
        }

        numPendingCacheTransPerCore[i] = 0;
    }

    g_balarmmio_component = this;

    // Initial values
    has_blocked_response = false;
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

void BalarMMIO::SST_callback_memcpy_H2D_done() {
    out.verbose(CALL_INFO, 1, 0, "Prepare memcpyD2H writes\n");
    if (last_packet.isSSTmem) {
        // Safely free the source data buffer
        free(memcpyH2D_dst);
        memcpyH2D_dst = NULL;

        // Mark the CUDA call as done
        cuda_ret.is_cuda_call_done = true;
    }
    
    // Send blocked response
    mmio_iface->send(blocked_response);
    has_blocked_response = false;
}

void BalarMMIO::SST_callback_memcpy_D2H_done() {
    // Use DMA engine to send the copy from Simulator to SST memspace
    // Should wait for the memcpy to complete
    // as memcpy is blocking
    // Prepare a request to write the dst data to CPU
    out.verbose(CALL_INFO, 1, 0, "Prepare memcpyD2H writes\n");
    BalarCudaCallPacket_t * packet = &last_packet;
    Addr dstAddr = packet->cuda_memcpy.dst;

    if (packet->isSSTmem) {
        std::string cmdString = "Issue_DMA_memcpy_D2H";
        // Prepare DMA config

        DMAEngine::DMAEngineControlRegisters dma_registers;
        dma_registers.sst_mem_addr = dstAddr;
        dma_registers.simulator_mem_addr = memcpyD2H_dst;
        dma_registers.data_size = packet->cuda_memcpy.count;
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
        requests[dma_req->getID()] = std::make_pair(getCurrentSimTime(), cmdString);

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

void BalarMMIO::SST_callback_cudaThreadSynchronize_done() {
    if (has_blocked_response) {
        mmio_iface->send(blocked_response);
        has_blocked_response = false;

        // Mark the CUDA call as done
        cuda_ret.is_cuda_call_done = true;
    }
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
    out->verbose(_INFO_, "%s: receiving incoming write (%ld) to vaddr: %llx and paddr: %llx with size %lld\n", balar->getName().c_str(), write->getID(), write->vAddr, write->pAddr, write->size);

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

    // Record the read request we made
    balar->requests[dma_req->getID()] = std::make_pair(balar->getCurrentSimTime(), cmdString);

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
    out->verbose(_INFO_, "%s: receiving incoming read (%ld) to vaddr: %llx and paddr: %llx with size %lld\n", balar->getName().c_str(), read->getID(), read->vAddr, read->pAddr, read->size);

    out->verbose(_INFO_, "Handling Read for return value for a %s request\n", gpu_api_to_string(balar->cuda_ret.cuda_call_id)->c_str());

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
    balar->requests[dma_req->getID()] = std::make_pair(balar->getCurrentSimTime(), cmdString);

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
    out->verbose(_INFO_, "%s: receiving incoming readresp (%ld) to vaddr: %llx and paddr: %llx with size %lld\n", balar->getName().c_str(), resp->getID(), resp->vAddr, resp->pAddr, resp->size);
    // Classify the read request type
    std::map<uint64_t, std::pair<SimTime_t,std::string>>::iterator i = balar->requests.find(resp->getID());
    if (balar->requests.end() == i) {
        out->fatal(_INFO_, "Event (%ld) not found!\n", resp->getID());
    } else {
        std::string request_type = (i)->second.second;
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
    out->verbose(_INFO_, "%s: receiving incoming writeresp (%ld) to vaddr: %llx and paddr: %llx with size %lld\n", balar->getName().c_str(), resp->getID(), resp->vAddr, resp->pAddr, resp->size);

    std::map<uint64_t, std::pair<SimTime_t,std::string>>::iterator i = balar->requests.find(resp->getID());
    if (balar->requests.end() == i ) {
        out->fatal(_INFO_, "Event (%ld) not found!\n", resp->getID());
    } else {
        std::string request_type = (i)->second.second;
        out->verbose(_INFO_, "%s: get response from write request (%ld) with type: %s\n", balar->getName().c_str(), resp->getID(), request_type.c_str());

        // Dispatch based on request type
        if (request_type.compare("Read_cuda_packet") == 0) {
            // This is a response to a request we send to cache
            // to read the cuda packet
            // Whether the request is blocking or not
            balar->has_blocked_response = false;

            // Our write instance from CUDA API Call
            StandardMem::Write* write = balar->pending_write;

            // Get cuda call arguments from the last_packet, which is the
            // simulator space pointer we passed to DMA engine
            BalarCudaCallPacket_t * packet = &(balar->last_packet);

            out->verbose(_INFO_, "Handling CUDA API Call (%d). Enum is %s\n", packet->cuda_call_id, gpu_api_to_string(packet->cuda_call_id)->c_str());

            // Save the call type for return packet
            balar->cuda_ret.cuda_call_id = packet->cuda_call_id;

            // Most of the CUDA api will return immediately
            // except for memcpy and threadsync
            balar->cuda_ret.is_cuda_call_done = true;

            // The following are for non-SST memspace
            switch (packet->cuda_call_id) {
                case GPU_REG_FAT_BINARY: 
                    balar->cuda_ret.cuda_error = cudaSuccess;
                    // Overwrite filename if given CUDA program path from config script
                    if (balar->cudaExecutable.empty())
                        balar->cuda_ret.fat_cubin_handle = (uint64_t) __cudaRegisterFatBinarySST(packet->register_fatbin.file_name);
                    else
                        balar->cuda_ret.fat_cubin_handle = (uint64_t) __cudaRegisterFatBinarySST((char *)balar->cudaExecutable.c_str());
                    break;
                case GPU_REG_FUNCTION: 
                    // hostFun should be a fixed pointer value to each kernel function 
                    __cudaRegisterFunctionSST(packet->register_function.fatCubinHandle, 
                                            packet->register_function.hostFun,
                                            packet->register_function.deviceFun);
                    break;
                case GPU_MEMCPY: 
                    // Handle a special case where we have a memcpy
                    // within SST memory space
                    balar->has_blocked_response = true;
                    if (packet->isSSTmem) {
                        // With SST memory/vanadis, we should sync for it to complete
                        balar->cuda_ret.is_cuda_call_done = false;
                        if (packet->cuda_memcpy.kind == cudaMemcpyHostToDevice) {
                            // We will end the handler early as we
                            // need to read the src data in host mem first
                            // and then proceed

                            // Assign device buffer
                            size_t data_size = packet->cuda_memcpy.count;
                            balar->memcpyH2D_dst = (uint8_t *) calloc(data_size, sizeof(uint8_t));

                            DMAEngine::DMAEngineControlRegisters dma_registers;
                            dma_registers.sst_mem_addr = packet->cuda_memcpy.src;
                            dma_registers.simulator_mem_addr = balar->memcpyH2D_dst;
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
                            balar->requests[dma_req->getID()] = std::make_pair(balar->getCurrentSimTime(), cmdString);

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
                            balar->memcpyD2H_dst = (uint8_t *) calloc(packet->cuda_memcpy.count, sizeof(uint8_t));

                            // Call memcpy
                            balar->cuda_ret.cuda_error = cudaMemcpy(
                                    (void *)  balar->memcpyD2H_dst,
                                    (const void*) packet->cuda_memcpy.src,
                                    packet->cuda_memcpy.count,
                                    packet->cuda_memcpy.kind);

                            // These two fields are for non SST mem use only
                            // for comparing the results
                            balar->cuda_ret.cudamemcpy.sim_data = NULL;
                            balar->cuda_ret.cudamemcpy.real_data = NULL;

                            balar->cuda_ret.cudamemcpy.size = packet->cuda_memcpy.count;
                            balar->cuda_ret.cudamemcpy.kind = packet->cuda_memcpy.kind;
                        } else {
                            out->fatal(CALL_INFO, -1, "%s: unsupported memcpy kind! Get: %d\n", balar->getName().c_str(), packet->cuda_memcpy.kind);
                        }
                    } else {
                        balar->cuda_ret.cuda_error = cudaMemcpy(
                                (void *) packet->cuda_memcpy.dst,
                                (const void*) packet->cuda_memcpy.src,
                                packet->cuda_memcpy.count,
                                packet->cuda_memcpy.kind);
                        
                        // Payload contains the pointer actual data from hw trace run
                        // dst is the pointer to the simulation cpy
                        balar->cuda_ret.cudamemcpy.sim_data = (volatile uint8_t*) packet->cuda_memcpy.dst;
                        balar->cuda_ret.cudamemcpy.real_data = (volatile uint8_t*) packet->cuda_memcpy.payload;
                        balar->cuda_ret.cudamemcpy.size = packet->cuda_memcpy.count;
                        balar->cuda_ret.cudamemcpy.kind = packet->cuda_memcpy.kind;
                    }
                    break;
                case GPU_CONFIG_CALL: 
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
                    }
                    break;
                case GPU_SET_ARG: 
                    balar->cuda_ret.cuda_error = cudaSetupArgumentSST(
                        packet->setup_argument.arg,
                        packet->setup_argument.value,
                        packet->setup_argument.size,
                        packet->setup_argument.offset
                    );
                    break;
                case GPU_LAUNCH: 
                    balar->cuda_ret.cuda_error = cudaLaunchSST(packet->cuda_launch.func);
                    break;
                case GPU_FREE: 
                    balar->cuda_ret.cuda_error = cudaFree(packet->cuda_free.devPtr);
                    break;
                case GPU_GET_LAST_ERROR: 
                    balar->cuda_ret.cuda_error = cudaGetLastError();
                    break;
                case GPU_MALLOC: 
                    balar->cuda_ret.cudamalloc.devptr_addr = (uint64_t) packet->cuda_malloc.devPtr;
                    balar->cuda_ret.cuda_error = cudaSuccess;
                    balar->cuda_ret.cudamalloc.malloc_addr = cudaMallocSST(
                        packet->cuda_malloc.devPtr,
                        packet->cuda_malloc.size
                    );
                    break;
                case GPU_REG_VAR: 
                    balar->cuda_ret.cuda_error = cudaSuccess;
                    __cudaRegisterVar(
                        packet->register_var.fatCubinHandle,
                        packet->register_var.hostVar,
                        packet->register_var.deviceAddress,
                        packet->register_var.deviceName,
                        packet->register_var.ext,
                        packet->register_var.size,
                        packet->register_var.constant,
                        packet->register_var.global
                    );
                    break;
                case GPU_MAX_BLOCK: 
                    balar->cuda_ret.cuda_error = cudaOccupancyMaxActiveBlocksPerMultiprocessorWithFlags(
                        (int *) (packet->max_active_block.numBlock),
                        (const char *) (packet->max_active_block.hostFunc),
                        packet->max_active_block.blockSize,
                        packet->max_active_block.dynamicSMemSize,
                        packet->max_active_block.flags
                    );
                    break;
                case GPU_PARAM_CONFIG: {
                        std::tuple<cudaError_t, size_t, unsigned> res = SST_cudaGetParamConfig(packet->cudaparamconfig.hostFun, packet->cudaparamconfig.index);
                        balar->cuda_ret.cuda_error = std::get<0>(res);
                        balar->cuda_ret.cudaparamconfig.size = std::get<1>(res);
                        balar->cuda_ret.cudaparamconfig.alignment = std::get<2>(res);
                    }
                    break;
                case GPU_THREAD_SYNC: {
                        // Should always succeed
                        balar->cuda_ret.cuda_error = cudaSuccess;
                        cudaError_t result = cudaThreadSynchronizeSST();
                        
                        // Check if need to make this a blocked response
                        // if so, mark this and defer the completion
                        // after GPGPU-Sim notifies us it is done with
                        // cudaThreadSynchronize()
                        if (result != cudaSuccess) {
                            balar->has_blocked_response = true;
                            // Mark this so that vanadis can poll for completion
                            balar->cuda_ret.is_cuda_call_done = false;
                        }
                    }
                    break;
                case GPU_MEMSET: {
                        balar->cuda_ret.cuda_error = cudaMemset(
                            packet->cudamemset.mem,
                            packet->cudamemset.c,
                            packet->cudamemset.count
                        );
                    }
                    break;
                case GPU_GET_DEVICE_COUNT: {
                        balar->cuda_ret.cuda_error = cudaGetDeviceCount(
                            &(balar->cuda_ret.cudagetdevicecount.count)
                        );
                    }
                    break;
                case GPU_SET_DEVICE: {
                        balar->cuda_ret.cuda_error = cudaSetDevice(
                            packet->cudasetdevice.device
                        );
                    }
                    break;
                case GPU_REG_TEXTURE: {
                        balar->cuda_ret.cuda_error = cudaSuccess;
                        __cudaRegisterTexture(
                            packet->cudaregtexture.fatCubinHandle,
                            packet->cudaregtexture.hostVar,
                            packet->cudaregtexture.deviceAddress,
                            packet->cudaregtexture.deviceName,
                            packet->cudaregtexture.dim,
                            packet->cudaregtexture.norm,
                            packet->cudaregtexture.ext
                        );
                    }
                    break;
                case GPU_BIND_TEXTURE: {
                        balar->cuda_ret.cuda_error = cudaBindTexture(
                            packet->cudabindtexture.offset,
                            packet->cudabindtexture.texref,
                            packet->cudabindtexture.devPtr,
                            packet->cudabindtexture.desc,
                            packet->cudabindtexture.size
                        );
                    }
                    break;
                case GPU_MALLOC_HOST: {
                        balar->cuda_ret.cuda_error = cudaMallocHostSST(
                            packet->cudamallochost.addr,
                            packet->cudamallochost.size
                        );
                    }
                    break;
                default:
                    out->fatal(CALL_INFO, -1, "Received unknown GPU enum API: %d\n", packet->cuda_call_id);
                    break;
            }

            // Send response (ack) to the CUDA API Cuda request if needed
            if (!(write->posted)) {
                if (balar->has_blocked_response) {
                    // Save blocked req's response and send later
                    balar->blocked_response = write->makeResponse();
                    out->verbose(CALL_INFO, 1, 0, "Handling a blocking request: %s\n", gpu_api_to_string(packet->cuda_call_id)->c_str());

                } else {
                    balar->mmio_iface->send(write->makeResponse());
                    out->verbose(CALL_INFO, 1, 0, "Handling a non-blocking request: %s\n", gpu_api_to_string(packet->cuda_call_id)->c_str());
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
            out->verbose(_INFO_, "%s: handling previous read request (%ld) for CUDA return packet to vaddr: %llx and paddr: %llx with size %lld at inst: %llx, returning the address of the packet: %lx\n", balar->getName().c_str(), read->getID(), read->vAddr, read->pAddr, read->size, read->iPtr, balar->packet_scratch_mem_addr);

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

            free(balar->memcpyD2H_dst);
            balar->memcpyD2H_dst = NULL;
            balar->mmio_iface->send(balar->blocked_response);
            balar->has_blocked_response = false;
            
            // Assert that this memcpy is done so that the vanadis CPU will
            // able to sync properly
            balar->cuda_ret.is_cuda_call_done = true;

            delete resp;
        } else if (request_type.compare("Issue_DMA_memcpy_H2D") == 0) {
            // To a DMA request we made to store the src data from SST memory space to a buffer
            // Now we need to actually perform the memcpy using GPGPUSim method
            BalarCudaCallPacket_t * packet = &(balar->last_packet);
            StandardMem::Write* write = balar->pending_write;

            // Call with the source data in simulator space
            // balar->memcpyH2D_dst is the buffer pointer where the DMA copied into
            balar->cuda_ret.cuda_error = cudaMemcpy(
                    (void *) packet->cuda_memcpy.dst,
                    (const void*) balar->memcpyH2D_dst,
                    packet->cuda_memcpy.count,
                    packet->cuda_memcpy.kind);
            
            // Payload contains the pointer actual data from hw trace run
            // dst is the pointer to the simulation cpy
            balar->cuda_ret.cudamemcpy.sim_data = (volatile uint8_t*) packet->cuda_memcpy.dst;
            balar->cuda_ret.cudamemcpy.real_data = (volatile uint8_t*) packet->cuda_memcpy.payload;
            balar->cuda_ret.cudamemcpy.size = packet->cuda_memcpy.count;
            balar->cuda_ret.cudamemcpy.kind = packet->cuda_memcpy.kind;

            // Save blocked req's response and send later (in SST_callback_memcpy_H2D_done()), since it is a memcpy
            balar->blocked_response = write->makeResponse();
            out->verbose(CALL_INFO, 1, 0, "Handling a blocking request: %s\n", gpu_api_to_string(packet->cuda_call_id)->c_str());

            // Delete requests as we don't need them anymore
            balar->pending_write = nullptr;
            delete write;
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

extern void SST_callback_memcpy_H2D_done() {
    assert(g_balarmmio_component);
    g_balarmmio_component->SST_callback_memcpy_H2D_done();
}

extern void SST_callback_memcpy_D2H_done() {
    assert(g_balarmmio_component);
    g_balarmmio_component->SST_callback_memcpy_D2H_done();
}

extern void SST_callback_cudaThreadSynchronize_done() {
    assert(g_balarmmio_component);
    g_balarmmio_component->SST_callback_cudaThreadSynchronize_done();
}