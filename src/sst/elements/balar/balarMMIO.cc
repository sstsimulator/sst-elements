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

    iface->setMemoryMappedAddressRegion(mmio_addr, 1);

    // TODO GPU Cache interface?

    handlers = new mmioHandlers(this, &out);
    g_balarmmio_component = this;
}

void BalarMMIO::init(unsigned int phase) {
    iface->init(phase);
}

void BalarMMIO::setup() {
    iface->setup();
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
 * @brief Handler for incoming Write requests, passing
 *        the cuda function call arguments.
 *        GPGPUSIM will be called and the corresponding
 *        return value will be stored.
 * 
 * @param write 
 */
void BalarMMIO::mmioHandlers::handle(SST::Interfaces::StandardMem::Write* write) {
    // Whether the request is blocking or not
    bool is_blocked = false;
    // Convert 8 bytes of the payload into an int
    std::vector<uint8_t> buff = write->data;

    // Get cuda call arguments
    mmio->last_packet = decode_balar_packet<BalarCudaCallPacket_t>(&buff);
    BalarCudaCallPacket_t * packet = mmio->last_packet;

    out->verbose(_INFO_, "Handling Write. Enum is %s\n", gpu_api_to_string(packet->cuda_call_id)->c_str());

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
            // TODO: Arguments are setup individually?
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

    /* Send response (ack) if needed */
    if (!(write->posted)) {
        if (is_blocked) {
            // Save blocked req's response and send later
            mmio->blocked_response = write->makeResponse();
        } else {
            mmio->iface->send(write->makeResponse());
        }
    }
    delete write;
}

/**
 * @brief Handler for return value query
 * 
 * @param read 
 */
void BalarMMIO::mmioHandlers::handle(SST::Interfaces::StandardMem::Read* read) {
    out->verbose(_INFO_, "Handling Read for return value for a %s request\n", gpu_api_to_string(mmio->cuda_ret.cuda_call_id)->c_str());

    vector<uint8_t> *payload = encode_balar_packet<BalarCudaCallReturnPacket_t>(&mmio->cuda_ret);

    payload->resize(read->size, 0); // A bit silly if size != sizeof(int) but that's the CPU's problem

    // Make a response. Must fill in payload.
    StandardMem::ReadResp* resp = static_cast<StandardMem::ReadResp*>(read->makeResponse());
    resp->data = *payload;
    mmio->iface->send(resp);
    delete read;
}

/* Handler for incoming Read responses - should be a response to a Read we issued */
void BalarMMIO::mmioHandlers::handle(SST::Interfaces::StandardMem::ReadResp* resp) {
    // TODO Do nothing rn since we do not issue read
    // TODO But might need this in future to handle read from gpu cache?
    // std::map<uint64_t, std::pair<SimTime_t,std::string>>::iterator i = mmio->requests.find(resp->getID());
    // if ( mmio->requests.end() == i ) {
    //     out->fatal(CALL_INFO, -1, "Event (%" PRIx64 ") not found!\n", resp->getID());
    // } else {
    //     SimTime_t et = mmio->getCurrentSimTime() - i->second.first;
    //     mmio->statReadLatency->addData(et);
    //     mmio->requests.erase(i);
    // }
    // delete resp;
    // if (mmio->mem_access == 0 && mmio->requests.empty())
    //     mmio->primaryComponentOKToEndSim();
}

/* Handler for incoming Write responses - should be a response to a Write we issued */
void BalarMMIO::mmioHandlers::handle(SST::Interfaces::StandardMem::WriteResp* resp) {
    // TODO Do nothing rn since we do not issue write
    // TODO But might need this in future to handle write to gpu cache?
    // std::map<uint64_t, std::pair<SimTime_t,std::string>>::iterator i = mmio->requests.find(resp->getID());
    // if (mmio->requests.end() == i) {
    //     out->fatal(CALL_INFO, -1, "Event (%" PRIx64 ") not found!\n", resp->getID());
    // } else {
    //     SimTime_t et = mmio->getCurrentSimTime() - i->second.first;
    //     mmio->statWriteLatency->addData(et);
    //     mmio->requests.erase(i);
    // }
    // delete resp;
    // if (mmio->mem_access == 0 && mmio->requests.empty())
    //     mmio->primaryComponentOKToEndSim();
}

void BalarMMIO::mmioHandlers::intToData(int32_t num, vector<uint8_t>* data) {
    data->clear();
    for (int i = 0; i < sizeof(int); i++) {
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
extern bool is_SST_buffer_full(unsigned core_id)
{
//    assert(my_gpu_component);
//    return my_gpu_component->is_SST_buffer_full(core_id);
    return false;
}

extern void send_read_request_SST(unsigned core_id, uint64_t address, uint64_t size, void* mem_req)
{
//    assert(my_gpu_component);
//    my_gpu_component->send_read_request_SST(core_id, address, size, mem_req);

}

extern void send_write_request_SST(unsigned core_id, uint64_t address, uint64_t size, void* mem_req)
{
//    assert(my_gpu_component);
//    my_gpu_component->send_write_request_SST(core_id, address, size, mem_req);
}

extern void SST_callback_memcpy_H2D_done()
{
   assert(g_balarmmio_component);
   g_balarmmio_component->SST_callback_memcpy_H2D_done();
}

extern void SST_callback_memcpy_D2H_done()
{
   assert(g_balarmmio_component);
   g_balarmmio_component->SST_callback_memcpy_D2H_done();
}