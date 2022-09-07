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

#include <sst_config.h>
#include "balarMMIO.h"
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
    cpu_core_count = (uint32_t) params.find<uint32_t>("cpu_cores", 1);
    gpu_core_count = (uint32_t) params.find<uint32_t>("gpu_cores", 1);
    latency = (uint32_t) params.find<uint32_t>("latency", 1);
    maxPendingTransCore = (uint32_t) params.find<uint32_t>("maxtranscore", 16);
    maxPendingCacheTrans = (uint32_t) params.find<uint32_t>("maxcachetrans", 512);
    mmio_size = (uint32_t) params.find<uint32_t>("mmio_size", 512);

    // Ensure that GPGP-sim has the same as SST gpu cores
    SST_gpgpusim_numcores_equal_check(gpu_core_count);
    pending_transactions_count = 0;
    remainingTransfer = 0;
    totalTransfer = 0;
    ackTransfer = 0;
    transferNumber = 0;

    bool found = false;

    // Initialize registers
    last_packet = nullptr;

    // Memory address
    mmio_addr = params.find<uint64_t>("base_addr", 0);

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
    pendingTransactions = new std::unordered_map<StandardMem::Request::id_t, StandardMem::Request*>();
    gpuCachePendingTransactions = new std::unordered_map<StandardMem::Request::id_t, cache_req_params>();

    // Interface to CPU
    iface = loadUserSubComponent<SST::Interfaces::StandardMem>("iface", ComponentInfo::SHARE_NONE, tc, 
            new StandardMem::Handler<BalarMMIO>(this, &BalarMMIO::handleEvent));
    
    if (!iface) {
        out.fatal(CALL_INFO, -1, "%s, Error: No interface found loaded into 'iface' subcomponent slot. Please check input file\n", getName().c_str());
    }

    iface->setMemoryMappedAddressRegion(mmio_addr, mmio_size);

    // Handlers for cpu interactions
    handlers = new mmioHandlers(this, &out);
    
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

            // Debug
            param.insert("debug", "1");
            param.insert("debug_level", "10");
            // Debug end

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
    iface->init(phase);
    for (uint32_t i = 0; i < gpu_core_count; i++)
        gpu_to_cache_links[i]->init(phase);
}

void BalarMMIO::setup() {
    iface->setup();
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

    out.verbose(CALL_INFO, 1, 0, "Sent a read request with id (%d)\n", req->getID());
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
    out.verbose(CALL_INFO, 1, 0, "Sent a write request with id (%d)\n", req->getID());
}

void BalarMMIO::SST_callback_memcpy_H2D_done() {
    // Send blocked response
    iface->send(blocked_response);
}

void BalarMMIO::SST_callback_memcpy_D2H_done() {
    // Send blocked response
    iface->send(blocked_response);
}

bool BalarMMIO::clockTic(Cycle_t cycle) {
    bool done = SST_gpu_core_cycle();
    return done;
}

void BalarMMIO::handleEvent(StandardMem::Request* req) {
    // incoming CPU request, handle using mmioHandlers
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
        assert("\n Cannot find the request\n" &&  0);
    }
    // req->handle(gpuCacheHandlers);
}

/**
 * @brief Handler for incoming Write requests, passing
 *        the cuda function call arguments.
 *        GPGPUSIM will be called and the corresponding
 *        return value will be stored.
 * 
 * @param write 
 */
void BalarMMIO::mmioHandlers::handle(SST::Interfaces::StandardMem::Write* write) {
    // TODO: Treat the data as the address to the passed in packet
    // Convert 8 bytes of the payload into an int
    // std::vector<uint8_t> buff = write->data;

    // Save this write instance as we will need it to make response
    // when finish calling GPGPUSim after getting readresp
    mmio->pending_write = write;

    // The write data is a scratch memory address to the real cuda call packet
    // Convert this into an address, assume little endian
    mmio->packet_scratch_mem_addr = dataToUInt64(&(write->data));

    // Create a memory request to read the cuda call packet
    StandardMem::Request* cuda_req = new StandardMem::Read(mmio->packet_scratch_mem_addr, sizeof(BalarCudaCallPacket_t));

    // Now send the memory request
    // inside the SST memory space
    mmio->iface->send(cuda_req);
}

/**
 * @brief Handler for return value query
 * 
 * @param read 
 */
void BalarMMIO::mmioHandlers::handle(SST::Interfaces::StandardMem::Read* read) {
    out->verbose(_INFO_, "Handling Read for return value for a %s request\n", gpu_api_to_string(mmio->cuda_ret.cuda_call_id)->c_str());

    // Save this write instance as we will need it to make response
    // when finish writing return packet to memory
    mmio->pending_read = read;

    vector<uint8_t> *payload = encode_balar_packet<BalarCudaCallReturnPacket_t>(&mmio->cuda_ret);

    // Write to the scratch memory region first
    StandardMem::Request* write_cuda_ret = new StandardMem::Write(mmio->packet_scratch_mem_addr, sizeof(BalarCudaCallReturnPacket_t), *payload);
    
    // Now send the memory request to write the cuda return packet
    // inside the SST memory space
    mmio->iface->send(write_cuda_ret);
}

/* Handler for incoming Read responses - getting response from reading
 * CUDA packet data
 */
void BalarMMIO::mmioHandlers::handle(SST::Interfaces::StandardMem::ReadResp* resp) {
    // Whether the request is blocking or not
    bool is_blocked = false;

    // Our write instance from CUDA API Call
    StandardMem::Write* write = mmio->pending_write;

    // Get cuda call arguments from read response
    mmio->last_packet = decode_balar_packet<BalarCudaCallPacket_t>(&(resp->data));
    BalarCudaCallPacket_t * packet = mmio->last_packet;

    out->verbose(_INFO_, "Handling CUDA API Call. Enum is %s\n", gpu_api_to_string(packet->cuda_call_id)->c_str());

    // mmio->cuda_ret = (cudaError_t) pack_ptr->cuda_call_id; // For testing purpose

    // Save the call type for return packet
    mmio->cuda_ret.cuda_call_id = packet->cuda_call_id;
    switch (packet->cuda_call_id) {
        case GPU_REG_FAT_BINARY: 
            mmio->cuda_ret.cuda_error = cudaSuccess;
            mmio->cuda_ret.fat_cubin_handle = (uint64_t) __cudaRegisterFatBinarySST(packet->register_fatbin.file_name);
            break;
        case GPU_REG_FUNCTION: 
            // hostFun should be a fixed pointer value to each kernel function 
            __cudaRegisterFunctionSST(packet->register_function.fatCubinHandle, 
                                      packet->register_function.hostFun,
                                      packet->register_function.deviceFun);
            break;
        case GPU_MEMCPY: 
            is_blocked = true;
            // TODO Might want to optimize here with the payload and src
            // TODO that are essentially the same thing?
            mmio->cuda_ret.cuda_error = cudaMemcpy(
                    (void *) packet->cuda_memcpy.dst,
                    (const void*) packet->cuda_memcpy.src,
                    packet->cuda_memcpy.count,
                    packet->cuda_memcpy.kind);
            
            // Payload contains the pointer actual data from hw trace run
            // dst is the pointer to the simulation cpy
            mmio->cuda_ret.cudamemcpy.sim_data = (volatile uint8_t*) packet->cuda_memcpy.dst;
            mmio->cuda_ret.cudamemcpy.real_data = (volatile uint8_t*) packet->cuda_memcpy.payload;
            mmio->cuda_ret.cudamemcpy.size = packet->cuda_memcpy.count;
            mmio->cuda_ret.cudamemcpy.kind = packet->cuda_memcpy.kind;
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
                mmio->cuda_ret.cuda_error = cudaConfigureCallSST(
                    gridDim, 
                    blockDim, 
                    packet->configure_call.sharedMem, 
                    packet->configure_call.stream
                );
            }
            break;
        case GPU_SET_ARG: 
            mmio->cuda_ret.cuda_error = cudaSetupArgumentSST(
                packet->setup_argument.arg,
                packet->setup_argument.value,
                packet->setup_argument.size,
                packet->setup_argument.offset
            );
            break;
        case GPU_LAUNCH: 
            mmio->cuda_ret.cuda_error = cudaLaunchSST(packet->cuda_launch.func);
            break;
        case GPU_FREE: 
            mmio->cuda_ret.cuda_error = cudaFree(packet->cuda_free.devPtr);
            break;
        case GPU_GET_LAST_ERROR: 
            mmio->cuda_ret.cuda_error = cudaGetLastError();
            break;
        case GPU_MALLOC: 
            mmio->cuda_ret.cudamalloc.devptr_addr = (uint64_t) packet->cuda_malloc.devPtr;
            mmio->cuda_ret.cuda_error = cudaSuccess;
            mmio->cuda_ret.cudamalloc.malloc_addr = cudaMallocSST(
                packet->cuda_malloc.devPtr,
                packet->cuda_malloc.size
            );
            break;
        case GPU_REG_VAR: 
            mmio->cuda_ret.cuda_error = cudaSuccess;
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
            mmio->cuda_ret.cuda_error = cudaOccupancyMaxActiveBlocksPerMultiprocessorWithFlags(
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

    /* Send response (ack) to the CUDA API Cuda request if needed */
    if (!(write->posted)) {
        if (is_blocked) {
            // Save blocked req's response and send later
            mmio->blocked_response = write->makeResponse();
            out->verbose(CALL_INFO, 1, 0, "Handling a blocking request: %s", gpu_api_to_string(packet->cuda_call_id)->c_str());

        } else {
            mmio->iface->send(write->makeResponse());
            out->verbose(CALL_INFO, 1, 0, "Handling a non-blocking request: %s", gpu_api_to_string(packet->cuda_call_id)->c_str());
        }
    }
    delete write;
    delete resp;
}

/* Handler for incoming Write responses - should be a response to a Write we issued */
void BalarMMIO::mmioHandlers::handle(SST::Interfaces::StandardMem::WriteResp* resp) {
    // Make a response. Must fill in payload.
    StandardMem::Read* read = mmio->pending_read;
    StandardMem::ReadResp* read_resp = static_cast<StandardMem::ReadResp*>(read->makeResponse());

    // Return the scratch memory address as the read result
    vector<uint8_t> payload;
    UInt64ToData(mmio->packet_scratch_mem_addr, &payload);
    payload.resize(read->size, 0);
    read_resp->data = payload;
    mmio->iface->send(read_resp);
    delete read;
    delete resp;
}

void BalarMMIO::mmioHandlers::intToData(int32_t num, vector<uint8_t>* data) {
    data->clear();
    for (size_t i = 0; i < sizeof(int); i++) {
        data->push_back(num & 0xFF);
        num >>=8;
    }
}

int32_t BalarMMIO::mmioHandlers::dataToInt(vector<uint8_t>* data) {
    int32_t retval = 0;
    for (int i = data->size(); i > 0; i--) {
        retval <<= 8;
        retval |= (*data)[i-1];
    }
    return retval;
}

void BalarMMIO::mmioHandlers::UInt64ToData(uint64_t num, vector<uint8_t>* data) {
    data->clear();
    for (size_t i = 0; i < sizeof(uint64_t); i++) {
        data->push_back(num & 0xFF);
        num >>=8;
    }
}

uint64_t BalarMMIO::mmioHandlers::dataToUInt64(vector<uint8_t>* data) {
    uint64_t retval = 0;
    for (int i = data->size(); i > 0; i--) {
        retval <<= 8;
        retval |= (*data)[i-1];
    }
    return retval;
}

void BalarMMIO::printStatus(Output &statusOut) {
    statusOut.output("Balar::BalarMMIO %s\n", getName().c_str());
    // statusOut.output("    Register: %d\n", squared);
    // iface->printStatus(statusOut);
    statusOut.output("End Balar::BalarMMIO\n\n");
}

// TODO Global Wrappers
// TODO Finish for mmiohere
// TODO Handle cache timing simulation?
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