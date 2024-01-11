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

    // Initialize registers
    last_packet = nullptr;

    // Memory address
    mmio_addr = params.find<uint64_t>("base_addr", 0);
    dma_addr = params.find<uint64_t>("dma_addr", 0);
    separate_mem_iface = params.find<bool>("separate_mem_iface", true);

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

    // Interface to memory
    if (!separate_mem_iface) {
        // If we dont need to have a separate memory interface, can just use mmio_iface for memory requests as well
        mem_iface = mmio_iface;
    } else {
        mem_iface = loadUserSubComponent<SST::Interfaces::StandardMem>("mem_iface", ComponentInfo::SHARE_NONE, tc, 
                new StandardMem::Handler<BalarMMIO>(this, &BalarMMIO::handleEvent));
        
        if (!mem_iface) {
            out.fatal(CALL_INFO, -1, "%s, Error: No interface found loaded into 'mem_iface' subcomponent slot. Please check input file\n", getName().c_str());
        }
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
}

void BalarMMIO::init(unsigned int phase) {
    mmio_iface->init(phase);
    if (separate_mem_iface)
        mem_iface->init(phase);
    for (uint32_t i = 0; i < gpu_core_count; i++)
        gpu_to_cache_links[i]->init(phase);
}

void BalarMMIO::setup() {
    mmio_iface->setup();
    if (separate_mem_iface)
        mem_iface->setup();
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

    out.verbose(CALL_INFO, 1, 0, "Sent a read request with id (%d) to addr %llx\n", req->getID(), req->pAddr);
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
    out.verbose(CALL_INFO, 1, 0, "Sent a write request with id (%d) to addr: %llx\n", req->getID(), req->pAddr);
}

void BalarMMIO::SST_callback_memcpy_H2D_done() {
    if (last_packet->isSSTmem) {
        // Safely free the source data buffer
        free(memcpyH2D_dst);
        memcpyH2D_dst = NULL;

        // Mark the CUDA call as done
        cuda_ret.is_cuda_call_done = true;
    }
    
    // Send blocked response
    mmio_iface->send(blocked_response);
}

void BalarMMIO::SST_callback_memcpy_D2H_done() {
    // Use DMA engine to send the copy from Simulator to SST memspace
    // Should wait for the memcpy to complete
    // as memcpy is blocking
    // Prepare a request to write the dst data to CPU
    out.verbose(CALL_INFO, 1, 0, "Prepare memcpyD2H writes\n");
    BalarCudaCallPacket_t * packet = last_packet;
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
        out.fatal(CALL_INFO, -1, "GPU Cache Request (%d) not found!\n", req_id);
    }
}

/**
 * @brief Handler for incoming Write requests via `mmio_iface`
 *        the payload will be an address to the actual cuda call packet
 *        the function will save the Write object and issue a read
 *        to memory to fetch the cuda call packet (via `mem_iface`).
 *        The actual calling to GPGPU-Sim will be done in ReadResp handler
 * 
 * @param write 
 */
void BalarMMIO::BalarHandlers::handle(SST::Interfaces::StandardMem::Write* write) {
    // Save this write instance as we will need it to make response
    // when finish calling GPGPUSim after getting readresp
    balar->pending_write = write;

    // The write data is a scratch memory address to the real cuda call packet
    // Convert this into an address, assume little endian
    balar->packet_scratch_mem_addr = dataToUInt64(&(write->data));

    // On calling a new cuda call, we set the cuda return packet status
    // to be not done so that our CUDA runtime lib will sync the cudaMemcpy
    balar->cuda_ret.is_cuda_call_done = false;

    // Create a memory request to read the cuda call packet
    StandardMem::Request* cuda_req = new StandardMem::Read(balar->packet_scratch_mem_addr, sizeof(BalarCudaCallPacket_t));

    // Record the read request we made
    balar->requests[cuda_req->getID()] = std::make_pair(balar->getCurrentSimTime(), "Read_cuda_packet");

    // Log this write
    out->verbose(_INFO_, "%s: receiving incoming write with scratch mem address: 0x%llx, issuing read to this address\n", balar->getName().c_str(), balar->packet_scratch_mem_addr);

    // Now send the memory request
    // inside the SST memory space to read the cuda packet
    balar->mem_iface->send(cuda_req);
}

/**
 * @brief Handler for return value query via `mmio_iface`.
 *        Will save the read as pending read. Then it will issue a write
 *        request to save the return packet to the previously passed in
 *        memory address in Write request (via `mem_iface`).
 *        Response to CPU will be done in handler for WriteResp
 * 
 * @param read 
 */
void BalarMMIO::BalarHandlers::handle(SST::Interfaces::StandardMem::Read* read) {
    out->verbose(_INFO_, "Handling Read for return value for a %s request\n", gpu_api_to_string(balar->cuda_ret.cuda_call_id)->c_str());

    // Save this write instance as we will need it to make response
    // when finish writing return packet to memory
    balar->pending_read = read;

    vector<uint8_t> *payload = encode_balar_packet<BalarCudaCallReturnPacket_t>(&balar->cuda_ret);

    // Write to the scratch memory region first
    StandardMem::Request* write_cuda_ret = new StandardMem::Write(balar->packet_scratch_mem_addr, sizeof(BalarCudaCallReturnPacket_t), *payload);

    // Record this request
    balar->requests[write_cuda_ret->getID()] = std::make_pair(balar->getCurrentSimTime(), "Write_cuda_ret");

    // Log this read
    out->verbose(_INFO_, "%s: receiving incoming read, writing cuda return packet to scratch mem address: 0x%llx\n", balar->getName().c_str(), balar->packet_scratch_mem_addr);

    // Now send the memory request to write the cuda return packet
    // inside the SST memory space
    balar->mem_iface->send(write_cuda_ret);
}

/**
 * @brief Handler for the previous read request that get the cuda call packet (from `mem_iface`)
 *        It will decode the packet and make call to GPGPU-Sim.
 *        Then will notify CPU if the call is non-blocking (from `mmio_iface`).
 * 
 * @param resp 
 */
void BalarMMIO::BalarHandlers::handle(SST::Interfaces::StandardMem::ReadResp* resp) {
    // Based on the cuda api call type
    // issue reads to read the pointer data until all one level pointer value is read in?
    // Then proceed to the actual calling part
    // Might need to make a mask specifying which pointer has
    // been read in?
    // Also need a place at the end to write for the cuda memcpy?

    // Classify the read request type
    std::map<uint64_t, std::pair<SimTime_t,std::string>>::iterator i = balar->requests.find(resp->getID());
    if (balar->requests.end() == i ) {
        out->fatal(_INFO_, "Event (%" PRIx64 ") not found!\n", resp->getID());
    } else {
        std::string request_type = (i)->second.second;
        out->verbose(_INFO_, "%s: get response from read request (%d) with type: %s\n", balar->getName().c_str(), resp->getID(), request_type.c_str());

        // Now dispatch based on read request type
        // Either: Read CUDA packet
        //         Finish Query memcpy src

        if (request_type.compare("Read_cuda_packet") == 0) {
            // This is a response to a request we send to cache
            // to read the cuda packet
            // Whether the request is blocking or not
            bool is_blocked = false;

            // Our write instance from CUDA API Call
            StandardMem::Write* write = balar->pending_write;

            // Get cuda call arguments from read response
            balar->last_packet = decode_balar_packet<BalarCudaCallPacket_t>(&(resp->data));
            BalarCudaCallPacket_t * packet = balar->last_packet;

            out->verbose(_INFO_, "Handling CUDA API Call. Enum is %s\n", gpu_api_to_string(packet->cuda_call_id)->c_str());

            // Save the call type for return packet
            balar->cuda_ret.cuda_call_id = packet->cuda_call_id;

            // Most of the CUDA api will return immediately
            // except for memcpy and kernel launch
            balar->cuda_ret.is_cuda_call_done = true;

            // The following are for non-SST memspace
            switch (packet->cuda_call_id) {
                case GPU_REG_FAT_BINARY: 
                    balar->cuda_ret.cuda_error = cudaSuccess;
                    balar->cuda_ret.fat_cubin_handle = (uint64_t) __cudaRegisterFatBinarySST(packet->register_fatbin.file_name);
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
                    is_blocked = true;
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
                            out->fatal(CALL_INFO, -1, "%s: unsupported memcpy kind!\n", balar->getName().c_str());
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
                            packet->configure_call.stream
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
                        packet->max_active_block.numBlock,
                        packet->max_active_block.hostFunc,
                        packet->max_active_block.blockSize,
                        packet->max_active_block.dynamicSMemSize,
                        packet->max_active_block.flags
                    );
                    break;
                default:
                    out->fatal(CALL_INFO, -1, "Received unknown GPU enum API: %d\n", packet->cuda_call_id);
                    break;
            }

            // Send response (ack) to the CUDA API Cuda request if needed
            if (!(write->posted)) {
                if (is_blocked) {
                    // Save blocked req's response and send later
                    balar->blocked_response = write->makeResponse();
                    out->verbose(CALL_INFO, 1, 0, "Handling a blocking request: %s\n", gpu_api_to_string(packet->cuda_call_id)->c_str());

                } else {
                    balar->mmio_iface->send(write->makeResponse());
                    out->verbose(CALL_INFO, 1, 0, "Handling a non-blocking request: %s\n", gpu_api_to_string(packet->cuda_call_id)->c_str());
                }
            }
            delete write;
            delete resp;
        } else {
            // Unknown cmdstring, abort
            out->fatal(CALL_INFO, -1, "%s: Unknown read request (%" PRIx64 "): %s!\n", balar->getName().c_str(), resp->getID(), request_type.c_str());
        }

        // Delete the request 
        balar->requests.erase(i);
    }
}


/**
 * @brief Handler for a write we made (via `mem_iface`), which is a write that store
 *        cuda return packet in memory. It will then use the pending
 *        read request to make request telling CPU the return value is ready (via `mmio_iface`).
 * 
 * @param resp 
 */
void BalarMMIO::BalarHandlers::handle(SST::Interfaces::StandardMem::WriteResp* resp) {
    std::map<uint64_t, std::pair<SimTime_t,std::string>>::iterator i = balar->requests.find(resp->getID());
    if (balar->requests.end() == i ) {
        out->fatal(_INFO_, "Event (%" PRIx64 ") not found!\n", resp->getID());
    } else {
        std::string request_type = (i)->second.second;
        out->verbose(_INFO_, "%s: get response from write request (%d) with type: %s\n", balar->getName().c_str(), resp->getID(), request_type.c_str());

        // Dispatch based on request type
        if (request_type.compare("Write_cuda_ret") == 0) {
            // To a write we made to write the CUDA return packet
            // Make a response. Must fill in payload.
            StandardMem::Read* read = balar->pending_read;
            StandardMem::ReadResp* read_resp = static_cast<StandardMem::ReadResp*>(read->makeResponse());

            // Return the scratch memory address as the read result
            vector<uint8_t> payload;
            UInt64ToData(balar->packet_scratch_mem_addr, &payload);
            payload.resize(read->size, 0);
            read_resp->data = payload;
            balar->mmio_iface->send(read_resp);
            delete read;
            delete resp;
        } else if (request_type.compare("Issue_DMA_memcpy_D2H") == 0) {
            // To a DMA request we made to store the dst data into SST memory space
            // Send the blocked response aved in the previous 
            // request handler for memcpyD2H to notify CPU we are done with memcpyD2H
            // Since we get this request only after all data have been copied into host memory

            // Free temp buffer to hold memcpyD2H data
            free(balar->memcpyD2H_dst);
            balar->memcpyD2H_dst = NULL;
            balar->mmio_iface->send(balar->blocked_response);
            
            // Assert that this memcpy is done so that the vanadis CPU will
            // able to sync properly
            balar->cuda_ret.is_cuda_call_done = true;

            delete resp;
        } else if (request_type.compare("Issue_DMA_memcpy_H2D") == 0) {
            // To a DMA request we made to store the src data from SST memory space to a buffer
            // Now we need to actually perform the memcpy using GPGPUSim method
            BalarCudaCallPacket_t * packet = balar->last_packet;
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
            delete write;
            delete resp;
        } else {
            // Unknown cmdstring, abort
            out->fatal(CALL_INFO, -1, "%s: Unknown write request (%" PRIx64 "): %s!\n", balar->getName().c_str(), resp->getID(), request_type.c_str());
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
