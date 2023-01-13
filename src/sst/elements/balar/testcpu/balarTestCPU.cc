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

#include <sst/core/params.h>
#include <sst/core/simulation.h>
#include <sst/core/interfaces/stringEvent.h>

#include <sst/elements/memHierarchy/util.h>

#include "testcpu/balarTestCPU.h"
#include <string>
#include <iostream>
#include <map>
#include "util.h"

using namespace SST;
using namespace SST::Interfaces;
using namespace SST::MemHierarchy;
using namespace SST::BalarComponent;
using namespace SST::Statistics;

/* Constructor */
BalarTestCPU::BalarTestCPU(ComponentId_t id, Params& params) :
    Component(id) {

    out.init("BalarTestCPU[@f:@l:@p] ", params.find<unsigned int>("verbose", 1), 0, Output::STDOUT);
    
    bool found;

    gpuAddr = params.find<uint64_t>("gpu_addr", "0", found);  // range for gpu address space

    scratchMemAddr = params.find<uint64_t>("scratch_mem_addr", "0", found);  // range for gpu address space
    if (found) {
        sst_assert(scratchMemAddr != gpuAddr, CALL_INFO, -1, "incompatible parameters: gpu_addr must be >= mmio_addr (gpu_addr above mmio).\n");
    }

    // Tell the simulator not to end until we OK it
    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

    //set our clock
    std::string clockFreq = params.find<std::string>("clock", "1GHz");
    clockHandler = new Clock::Handler<BalarTestCPU>(this, &BalarTestCPU::clockTic);
    clockTC = registerClock( clockFreq, clockHandler );

    /* Find the interface the user provided in the Python and load it*/
    memory = loadUserSubComponent<StandardMem>("memory", ComponentInfo::SHARE_NONE, clockTC, new StandardMem::Handler<BalarTestCPU>(this, &BalarTestCPU::handleEvent));

    if (!memory) {
        out.fatal(CALL_INFO, -1, "Unable to load memHierarchy.standardInterface subcomponent; check that 'memory' slot is filled in input.\n");
    }

    clock_ticks = 0;
    total_memD2H_bytes = registerStatistic<uint64_t>("total_memD2H_bytes");
    correct_memD2H_bytes = registerStatistic<uint64_t>("correct_memD2H_bytes");
    correct_memD2H_ratio = registerStatistic<double>("correct_memD2H_ratio");

    // Trace parser initialization
    std::string traceFile = params.find<std::string>("trace_file", "cuda_calls.trace");
    std::string cudaExecutable = params.find<std::string>("cuda_executable", found);
    sst_assert(found, CALL_INFO, -1, "%s, Error: parameter 'cuda_executable' was not provided\n", getName().c_str());
    out.verbose(CALL_INFO, 2, 0, "Trace file: %s, cuda_executable: %s\n", traceFile.c_str(), cudaExecutable.c_str());

    // Whether to dump memcpy files
    enable_memcpy_dump = params.find<bool>("enable_memcpy_dump", false);

    // Bind response handler to cpu
    gpuHandler = new mmioHandlers(this, &out);

    // Bind trace parser
    trace_parser = new CudaAPITraceParser(this, &out, traceFile, cudaExecutable);
}

void BalarTestCPU::init(unsigned int phase)
{
    memory->init(phase);
}

void BalarTestCPU::setup() {
    memory->setup();
}

void BalarTestCPU::finish() { }

// incoming events are handled
void BalarTestCPU::handleEvent(StandardMem::Request *req) {
    req->handle(gpuHandler);
}

bool BalarTestCPU::clockTic( Cycle_t ) {
    ++clock_ticks;

    // Wait for previous call to finish
    // Issue a new API call if none is pending
    if (requests.empty()) {
        // Prepare cuda packet here and record the 
        // request we sent to gpu to start a cuda call
        Interfaces::StandardMem::Request* req = trace_parser->getNextCall();
        if (!(req == nullptr)) {
            // Add cuda call requests to pending map
            std::string cmdString = "Prepare_CUDA_packet";
            requests[req->getID()] = std::make_pair(getCurrentSimTime(), cmdString);

            // Write the cuda packet to memory so that balar can access it later
            out.verbose(_INFO_, "Sending cuda api call packet to memory\n");

            memory->send(req);

            return false;
        } else {
            // No more request
            out.verbose(CALL_INFO, 1, 0, "BalarTestCPU: Test Completed Successfuly\n");
            primaryComponentOKToEndSim();
            return true;    // Turn our clock off while we wait for any other CPUs to end
        }
    } else {
        // Wait for pending request
        return false;
    }
}

void BalarTestCPU::emergencyShutdown() {
    if (out.getVerboseLevel() > 1) {
        if (out.getOutputLocation() == Output::STDOUT)
            out.setOutputLocation(Output::STDERR);
        
        out.output("MemHierarchy::BalarTestCPU %s\n", getName().c_str());
        out.output("  Outstanding events: %zu\n", requests.size());
        out.output("End MemHierarchy::BalarTestCPU %s\n", getName().c_str());
    }
}

/**
 * @brief Helper function to create a write request to scratch memory
 *        address with the passed in packet as payload
 * 
 * @param pack : CUDA call config packet
 * @return Interfaces::StandardMem::Request* 
 */
Interfaces::StandardMem::Request* 
    BalarTestCPU::createGPUReqFromPacket(BalarCudaCallPacket_t pack) {
    vector<uint8_t> *buffer = encode_balar_packet<BalarCudaCallPacket_t>(&pack);

    // Instead of writing to GPU MMIO address
    // We write to the scratch memory address first
    StandardMem::Request* req = new Interfaces::StandardMem::Write(scratchMemAddr, buffer->size(), *buffer, false);
    // num_gpu_issued->addData(1);

    out.verbose(_INFO_, "creating GPU request %s, CUDA Function enum %s; packet address: %x\n", getName().c_str(), gpu_api_to_string(pack.cuda_call_id)->c_str(), scratchMemAddr);
    return req;
}

/**
 * @brief mmio event handler for a read we issued
 *        Check the pending map for the request corresponding to the response
 *        If the resp does not have a corresponding pending request, we abort
 *        If the req exist, we identify the type of the request.
 *          "Start_CUDA_ret":
 *              This request is previously sent to ask balar to prepare the
 *              return packet.
 * 
 *              Since it has received a resp, it means now we are ready
 *              to read the packet in the address returned in this read.
 *              
 *              Thus we create a read request with the returned
 *              value as address. Then we send it to memory and 
 *              mark this request as "Read_CUDA_ret_packet"
 *          "Read_CUDA_ret_packet":
 *              This request is sent to read CUDA return packet.
 *              Since we have received response, we have gotten the
 *              return value of last cuda call. 
 * 
 *              We will output the return value if it exists. Also
 *              if the previous call is a memcpy, we compare the data
 *              with the actual data from real GPU trace and output byte
 *              correct ratio.
 *        Remove that request after finish handling it.
 * 
 * @param resp
 */
void BalarTestCPU::mmioHandlers::handle(Interfaces::StandardMem::ReadResp* resp) {
    // Find the request from pending requests map
    std::map<uint64_t, std::pair<SimTime_t,std::string>>::iterator i = cpu->requests.find(resp->getID());
    if ( cpu->requests.end() == i ) {
        out->fatal(_INFO_, "Event (%" PRIx64 ") not found!\n", resp->getID());
    } else {
        std::string request_type = (i)->second.second;
        out->verbose(_INFO_, "%s: get response from read request (%d) with type: %s\n", cpu->getName().c_str(), resp->getID(), request_type.c_str());

        // Need to identify whether this response is a 
        //  write to the MMIO address
        //  or to the scratch memory where we prepare the cuda packet
        // Dispatcher here
        if (request_type.compare("Start_CUDA_ret") == 0) {
            // Balar has prepared the return packet by writing it to memory
            // pointed by the read paylod
            // We now need to issue yet another read to actually read the balar
            // return packet

            // Prepare packet
            std::string cmdString = "Read_CUDA_ret_packet";
            uint64_t ret_pack_addr = dataToUInt64(&(resp->data));
            Interfaces::StandardMem::Request* req = new Interfaces::StandardMem::Read(ret_pack_addr, sizeof(BalarCudaCallReturnPacket_t));
            
            // Add request to pending requests
            cpu->requests[req->getID()] = std::make_pair(cpu->getCurrentSimTime(), cmdString);
            
            // Send the read request to memory
            cpu->memory->send(req);
        } else if (request_type.compare("Read_CUDA_ret_packet") == 0) {
            vector<uint8_t> *data_ptr = &(resp->data);
            BalarCudaCallReturnPacket_t *ret_pack_ptr = decode_balar_packet<BalarCudaCallReturnPacket_t>(data_ptr);
            enum GpuApi_t api_type = ret_pack_ptr->cuda_call_id;
            if (api_type == GPU_REG_FAT_BINARY) {
                out->verbose(_INFO_, "Fatbin handle: %d\n", ret_pack_ptr->fat_cubin_handle);
                cpu->trace_parser->fatCubinHandle = ret_pack_ptr->fat_cubin_handle;
            } else if (api_type == GPU_MALLOC) {
                out->verbose(_INFO_, "GPU Malloc addr: %p\n", ret_pack_ptr->cudamalloc.malloc_addr);

                // Set device pointer value
                *(CUdeviceptr *)(ret_pack_ptr->cudamalloc.devptr_addr) = ret_pack_ptr->cudamalloc.malloc_addr;
            } else if (api_type == GPU_MEMCPY && ret_pack_ptr->cudamemcpy.kind == cudaMemcpyDeviceToHost) {
                // Verify D2H result by counting bytes differences
                // Also dump the device result
                size_t tot = ret_pack_ptr->cudamemcpy.size;
                size_t correct = 0;
                volatile uint8_t *sim_ptr    = ret_pack_ptr->cudamemcpy.sim_data;
                volatile uint8_t *real_ptr   = ret_pack_ptr->cudamemcpy.real_data;
                for (size_t i = 0; i < tot; i++) {
                    if ((*real_ptr) == (*sim_ptr))
                        correct++;
                    sim_ptr++;
                    real_ptr++;
                }

                // Reset ptr for dumping
                sim_ptr    = ret_pack_ptr->cudamemcpy.sim_data;
                real_ptr     = ret_pack_ptr->cudamemcpy.real_data;

                // Print out stats
                double ratio = (double) correct / (double) tot;
                out->verbose(_INFO_, "GPU memcpyD2H correct bytes: %d total bytes: %d ratio: %f\n", correct, tot, ratio);

                // Record stats
                cpu->total_memD2H_bytes->addData(tot);
                cpu->correct_memD2H_bytes->addData(correct);
                cpu->correct_memD2H_ratio->addData(ratio);

                // Dump data to file
                if (cpu->enable_memcpy_dump) {
                    char buf[200];
                    sprintf(buf, "cudamemcpyD2H-sim-%p-%p-size-%d.data", sim_ptr, real_ptr, tot);
                    std::ofstream dumpStream;
                    dumpStream.open(buf, std::ios::out | std::ios::binary);
                    if (!dumpStream.is_open()) {
                        out->fatal(CALL_INFO, -1, "Cannot open '%s' for dumping D2H memcpy data\n", buf);
                    }
                    dumpStream.write((const char *) sim_ptr, tot);
                    dumpStream.close();

                    sprintf(buf, "cudamemcpyD2H-real-%p-%p-size-%d.data", sim_ptr, real_ptr, tot);
                    std::ofstream dumpRealStream;
                    dumpRealStream.open(buf, std::ios::out | std::ios::binary);
                    if (!dumpRealStream.is_open()) {
                        out->fatal(CALL_INFO, -1, "Cannot open '%s' for dumping D2H memcpy data\n", buf);
                    }
                    dumpRealStream.write((const char *) real_ptr, tot);
                    dumpRealStream.close();
                }
            }
        } else {
            // Unknown cmdstring, abort
            out->fatal(CALL_INFO, -1, "%s: Unknown readd request (%" PRIx64 "): %s!\n", cpu->getName().c_str(), resp->getID(), request_type.c_str());
        }

        cpu->requests.erase(i);
    }

    delete resp;
}

/**
 * @brief mmio event handler for a write we issued
 *        Check the pending map for the request corresponding to the response
 *        If the resp does not have a corresponding pending request, we abort
 *        If the req exist, we identify the type of the request.
 *          "Prepare_CUDA_packet":
 *              This request is previously sent to store the CUDA packet to 
 *              some place in memory so that BalarMMIO could access it
 *              later.
 * 
 *              Since it has received a resp, it means now we are ready
 *              to let BalarMMIO initiate a CUDA call. Thus we create
 *              a write request to BalarMMIO mmio address and pass the
 *              scratch memory address we placed the packet.
 *              
 *              Then we send it to balar and mark this request as
 *              "Start_CUDA_call"
 *          "Start_CUDA_call":
 *              This request is sent to initiate a CUDA call. Since it
 *              has received response, we should issue a read to 
 *              balar mmio address to let it prepare the return packet
 *              in memory
 *        Remove that request after finish handling it.
 * 
 * @param resp 
 */
void BalarTestCPU::mmioHandlers::handle(Interfaces::StandardMem::WriteResp* resp) {
    // Find the request from pending requests map
    std::map<uint64_t, std::pair<SimTime_t,std::string>>::iterator i = cpu->requests.find(resp->getID());
    if ( cpu->requests.end() == i ) {
        out->fatal(CALL_INFO, -1, "Event (%" PRIx64 ") not found!\n", resp->getID());
    } else {
        std::string request_type = (i)->second.second;
        out->verbose(_INFO_, "%s: get response from write request (%d) with type: %s\n", cpu->getName().c_str(), resp->getID(), request_type.c_str());

        // Need to identify whether this response is a 
        //  write to the MMIO address
        //  or to the scratch memory where we prepare the cuda packet
        // Dispatcher here
        if (request_type.compare("Prepare_CUDA_packet") == 0) {
            // A response to a write of scratch memory address
            // where we store the cuda packet
            // Now we are ready to issue a write to the MMIO
            // address to notify the balarMMIO packet is ready

            // Create the write request to balar address
            // Store the scratch memory address to it
            vector<uint8_t> payload;
            UInt64ToData(cpu->scratchMemAddr, &payload);
            StandardMem::Request* req = new Interfaces::StandardMem::Write(cpu->gpuAddr, payload.size(), payload, false);

            // Add request to initiate cuda call and store in requests queue
            std::string cmdString = "Start_CUDA_call";
            cpu->requests[req->getID()] = std::make_pair(cpu->getCurrentSimTime(), cmdString);

            // Send the address of packet to GPU
            cpu->memory->send(req);
        } else if (request_type.compare("Start_CUDA_call") == 0) {
            // A response to a write which initiates the balarMMIO
            // We should now first send a read request to GPU to let it prepare the
            // return packet.
            std::string cmdString = "Start_CUDA_ret";
            Interfaces::StandardMem::Request* req = new Interfaces::StandardMem::Read(cpu->gpuAddr, 8);
            cpu->requests[req->getID()] = std::make_pair(cpu->getCurrentSimTime(), cmdString);
            cpu->memory->send(req);
        } else {
            // Unknown cmdstring, abort
            out->fatal(CALL_INFO, -1, "%s: Unknown write request (%" PRIx64 "): %s!\n", cpu->getName().c_str(), resp->getID(), request_type.c_str());
        }

        cpu->requests.erase(i);
    }
    delete resp;
}

/**
 * @brief Construct a new BalarTestCPU::CudaAPITraceParser::CudaAPITraceParser object
 * 
 * @param cpu               : BalarTestCPU object that used this parser
 * @param out               : SST Output object to log info
 * @param traceFile         : Path to the CUDA API trace file
 * @param cudaExecutable    : Path to the CUDA executable, which will be used by GPGPU-Sim
 */
BalarTestCPU::CudaAPITraceParser::CudaAPITraceParser(BalarTestCPU* cpu, SST::Output* out, std::string& traceFile, std::string& cudaExecutable) {
    this->cpu = cpu;
    this->out = out;
    this->cudaExecutable = cudaExecutable;
    traceStream.open(traceFile, std::ifstream::in);
    if (!traceStream.is_open()) {
        out->fatal(CALL_INFO, -1,"Error: trace file: '%s' not exist\n", traceFile.c_str());
    }

    // Extract base path
    size_t sep = traceFile.rfind("/");
    if (sep == std::string::npos) {
        // Local
        traceFileBasePath = std::string("./");
    } else {
        traceFileBasePath = traceFile.substr(0, sep + 1);  // include the slash
    }

    // Init class data structure
    initReqs = new std::queue<Interfaces::StandardMem::Request*>();
    dptr_map = new std::map<std::string, CUdeviceptr*>();
    func_map = new std::map<std::string, uint64_t>();

    // Inserting initialization request like register fatbinary to initReqs
    Interfaces::StandardMem::Request* req;
    BalarCudaCallPacket_t fatbin_pack;
    fatbin_pack.cuda_call_id = GPU_REG_FAT_BINARY;
    strcpy(fatbin_pack.register_fatbin.file_name, cudaExecutable.c_str());
    req = cpu->createGPUReqFromPacket(fatbin_pack);
    initReqs->push(req);
}

/**
 * @brief Receive next CUDA api call from the trace file
 * 
 * @return Interfaces::StandardMem::Request*, 
 *          a request that write a BalarCudaCallPacket_t to scratch memory place
 */
Interfaces::StandardMem::Request* BalarTestCPU::CudaAPITraceParser::getNextCall() {
    Interfaces::StandardMem::Request* req;
    if (!initReqs->empty()) {   // Finish initialization first
        req = initReqs->front();
        // Pop call to destructor to pointer, which is empty
        initReqs->pop();
        return req;
    } else {
        // Iterate through lines of trace file
        req = nullptr;
        if (!traceStream.eof()) {
            BalarCudaCallPacket_t pack;
            pack.isSSTmem = false;
            std::string line;
            std::getline(traceStream, line);
            out->verbose(CALL_INFO, 2, 0, "Trace info: %s\n", line.c_str());

            // TODO Parse the trace
            // Before first colon: api type
            // Search every colon for individual argument
            // TODO Need to maintain a hashmap for device pointer
            // TODO Need to load host data for cpy
            size_t firstColIdx = line.find(":");
            std::string cudaCallType = line.substr(0, firstColIdx);
            line = line.substr(firstColIdx + 1);
            line = trim(line);

            // Extract parameters as a map
            std::vector<std::string> params = split(line, std::string(","));

            // Trim whitespaces
            for (auto it = params.begin(); it < params.end(); it++)
                *it = trim(*it);

            // Params map
            std::map<std::string, std::string> params_map = map_from_vec(params, std::string(":"));

            // Branch to different api calls
            if (cudaCallType.find("memalloc") != std::string::npos) {
                pack.cuda_call_id = GPU_MALLOC;
                
                // Params
                auto tmp = params_map.find(std::string("dptr"));
                std::string dptr_name = trim(tmp->second);
                std::string dptr_size = trim(params_map.find(std::string("size"))->second);

                // Extract size
                size_t size;
                std::stringstream sstream(dptr_size);
                sstream >> size;

                // look up name first and use the ptr
                auto res = dptr_map->find(dptr_name);
                if (res == dptr_map->end()) {
                    // New pointer
                    CUdeviceptr* dptr = (CUdeviceptr*) malloc(sizeof(CUdeviceptr));
                    
                    // Save ptr val in hashmap 
                    dptr_map->insert({dptr_name, dptr});

                    // Prepare call pack
                    pack.cuda_malloc.devPtr = (void**) dptr;
                    pack.cuda_malloc.size = size;
                } else {
                    // Old pointer
                    // Impossible as current trace implementation
                    // Every malloc will get a new pointer
                    CUdeviceptr* dptr = res->second;

                    // Prepare call pack
                    pack.cuda_malloc.devPtr = (void**) dptr;
                    pack.cuda_malloc.size = size;
                }

                out->verbose(CALL_INFO, 2, 0, "Malloc Device pointer (%s) addr: %p val: %p\n", dptr_name.c_str(), pack.cuda_malloc.devPtr, *(pack.cuda_malloc.devPtr));


                // Create request
                req = cpu->createGPUReqFromPacket(pack);                
            } else if (cudaCallType.find("memcpyH2D") != std::string::npos || 
                       cudaCallType.find("memcpyD2H") != std::string::npos) {
                pack.cuda_call_id = GPU_MEMCPY;

                // Params
                std::string dptr_name = trim(params_map.find(std::string("device_ptr"))->second);
                size_t size;
                std::stringstream tmp(params_map.find(std::string("size"))->second);
                tmp >> size;
                std::string data_file = trim(params_map.find(std::string("data_file"))->second);
                std::string data_file_path = traceFileBasePath + data_file;

                // Open data file and prepare data to be copy
                std::ifstream dataStream(data_file_path, std::fstream::in | std::fstream::binary);
                if (!dataStream.is_open()) {
                    out->fatal(CALL_INFO, -1,"Error: data file: '%s' not exist\n", data_file_path.c_str());
                }
                out->verbose(CALL_INFO, 2, 0, "Reading data file: '%s'\n", data_file_path.c_str());
                uint8_t *real_data = (uint8_t *) malloc(sizeof(uint8_t) * size);
                dataStream.read((char *)real_data, size);

                // Get device pointer value
                CUdeviceptr* dptr = nullptr;
                auto pair = dptr_map->find(dptr_name);
                if (pair == dptr_map->end()) {
                    out->fatal(CALL_INFO, -1,"Error: device pointer: '%s' not exist in hashmap\n", dptr_name.c_str());
                } else {
                    dptr = pair->second;
                }

                if (cudaCallType.find("memcpyH2D") != std::string::npos) {
                    // Prepare pack for host to device
                    out->verbose(CALL_INFO, 2, 0, "MemcpyH2D Device pointer (%s) addr: %p val: %p\n", dptr_name.c_str(), dptr, *dptr);

                    pack.cuda_memcpy.kind = cudaMemcpyHostToDevice;
                    pack.cuda_memcpy.dst = *dptr;
                    pack.cuda_memcpy.src = (uint64_t) real_data;
                    pack.cuda_memcpy.count = size;
                    pack.cuda_memcpy.payload = real_data;

                    // Create request
                    req = cpu->createGPUReqFromPacket(pack);   
                } else if (cudaCallType.find("memcpyD2H") != std::string::npos) {
                    // Prepare for device to host
                    out->verbose(CALL_INFO, 2, 0, "MemcpyD2H Device pointer (%s) addr: %p val: %p\n", dptr_name.c_str(), dptr, *dptr);

                    // Prepare enough host space
                    uint8_t *buf = (uint8_t *) malloc(sizeof(uint8_t) * size);
                    pack.cuda_memcpy.kind = cudaMemcpyDeviceToHost;
                    pack.cuda_memcpy.dst = (uint64_t) buf;
                    pack.cuda_memcpy.src = (uint64_t) *dptr;
                    pack.cuda_memcpy.count = size;
                    pack.cuda_memcpy.payload = real_data;

                    // Create request
                    req = cpu->createGPUReqFromPacket(pack);   
                }
            } else if (cudaCallType.find("kernel launch") != std::string::npos) {
                // Params
                std::stringstream ss;
                std::string func_name = trim(params_map.find(std::string("name"))->second);
                std::string ptx_name = trim(params_map.find(std::string("ptx_name"))->second);
                unsigned int gdx, gdy, gdz, bdx, bdy, bdz;
                size_t sharedMem;
                gdx = std::stoul(params_map.find(std::string("gdx"))->second);
                gdy = std::stoul(params_map.find(std::string("gdy"))->second);
                gdz = std::stoul(params_map.find(std::string("gdz"))->second);
                bdx = std::stoul(params_map.find(std::string("bdx"))->second);
                bdy = std::stoul(params_map.find(std::string("bdy"))->second);
                bdz = std::stoul(params_map.find(std::string("bdz"))->second);
                sharedMem = std::stoul(params_map.find(std::string("sharedBytes"))->second);
                cudaStream_t stream = NULL;
                std::string arguments = trim(params_map.find(std::string("args"))->second);


                // Configure call, register function, set args, and then launch kernel

                // Other request in sequence pack into the queue
                BalarCudaCallPacket_t config_call_pack;
                BalarCudaCallPacket_t set_arg_pack;
                BalarCudaCallPacket_t launch_pack;
                config_call_pack.cuda_call_id = GPU_CONFIG_CALL;
                set_arg_pack.cuda_call_id = GPU_SET_ARG;
                launch_pack.cuda_call_id = GPU_LAUNCH;

                // Configure call
                out->verbose(CALL_INFO, 2, 0, "Create pack with configurations: gdx: %d gdy: %d gdz: %d bdx: %d bdy: %d bdz: %d\n", gdx, gdy, gdz, bdx, bdy, bdz);

                config_call_pack.configure_call.gdx = gdx;
                config_call_pack.configure_call.gdy = gdy;
                config_call_pack.configure_call.gdz = gdz;
                config_call_pack.configure_call.bdx = bdx;
                config_call_pack.configure_call.bdy = bdy;
                config_call_pack.configure_call.bdz = bdz;
                config_call_pack.configure_call.sharedMem = sharedMem;
                config_call_pack.configure_call.stream = stream;
                Interfaces::StandardMem::Request* config_call_req = cpu->createGPUReqFromPacket(config_call_pack);


                // If not register function, register first
                // else use the config call as a req
                if(func_map->find(func_name) == func_map->end()) {
                    // New function, register first
                    // This map will growing thus will not have repeated
                    // id
                    uint64_t func_id = func_map->size();
                    func_map->insert({func_name, func_id});
                    out->verbose(CALL_INFO, 2, 0, "Create pack to register function '%s' to device function '%s' with fatbinhandle: %d\n", func_name.c_str(), ptx_name.c_str(), fatCubinHandle);
                    pack.cuda_call_id = GPU_REG_FUNCTION;
                    pack.register_function.fatCubinHandle = fatCubinHandle;
                    pack.register_function.hostFun = func_id;
                    strcpy(pack.register_function.deviceFun, ptx_name.c_str());

                    // Create request and set to active one
                    req = cpu->createGPUReqFromPacket(pack);  

                    // Put the config call request after the function
                    // registration
                    initReqs->push(config_call_req);
                } else {
                    // Return config call request as the active one
                    req = config_call_req;
                }

                // Arguments setup
                size_t offset = 0;
                while (!arguments.empty()) {
                    // Get argument value and size
                    size_t pos;
                    pos = arguments.find("/");
                    std::string arg_val = arguments.substr(0, pos);
                    arguments = arguments.substr(pos + 1);

                    pos = arguments.find("/");
                    std::string arg_size_str = arguments.substr(0, pos);
                    arguments = arguments.substr(pos + 1);

                    // Get argument size
                    size_t arg_size;
                    std::stringstream scanner;
                    scanner << arg_size_str;
                    scanner >> arg_size;

                    // Set argument size and offset, update offset as well
                    size_t align_amount = arg_size;
                    offset = (offset + align_amount - 1) / align_amount * align_amount;
                    set_arg_pack.setup_argument.size = arg_size;
                    set_arg_pack.setup_argument.offset = offset;
                    offset += arg_size;

                    // Get argument value
                    // Check if a device pointer first
                    // if not, convert as value
                    if (arg_val.find("dptr") != std::string::npos) {
                        // A device pointer
                        auto it = dptr_map->find(arg_val);
                        if (it == dptr_map->end()) {
                            // Invalid device pointer
                            out->fatal(CALL_INFO, -1, "Invalid device pointer name: '%s'\n", arg_val.c_str());
                        } else {
                            CUdeviceptr dptr = *(it->second);
                            set_arg_pack.setup_argument.arg = (uint64_t) dptr;
                        }
                    } else if (arg_val.find(".") != std::string::npos) {
                        // A float/double value
                        double val = std::stod(arg_val);
                        if (arg_size == 8) {
                            // double
                            set_arg_pack.setup_argument.arg = 0;
                            memcpy(set_arg_pack.setup_argument.value, &val, arg_size);
                        } else if (arg_size == 4) {
                            // float
                            float val_f = (float) val;
                            set_arg_pack.setup_argument.arg = 0;
                            memcpy(set_arg_pack.setup_argument.value, &val_f, arg_size);
                        } else {
                            out->fatal(CALL_INFO, -1, "Unknown floating point format for '%s' with size: %d\n", arg_val.c_str(), arg_size);
                        }
                    } else {
                        // Treated as an integer value
                        int val = std::stoi(arg_val);
                        set_arg_pack.setup_argument.arg = 0;
                        memcpy(set_arg_pack.setup_argument.value, &val, arg_size);
                    }

                    // Insert request to queue
                    Interfaces::StandardMem::Request* setup_arg_req = cpu->createGPUReqFromPacket(set_arg_pack);
                    initReqs->push(setup_arg_req);
                }

                // Launch kernel
                uint64_t launch_kernel_id = func_map->find(func_name)->second;
                out->verbose(CALL_INFO, 2, 0, "Create pack to launch function '%s' with id %d\n", func_name.c_str(), launch_kernel_id);
                launch_pack.cuda_launch.func = launch_kernel_id;
                Interfaces::StandardMem::Request* launch_req = cpu->createGPUReqFromPacket(launch_pack);
                initReqs->push(launch_req);
            } else if (cudaCallType.find("free") != std::string::npos) {
                // TODO Remove device pointer from hashmap? 
            }
        }
        return req;
    }
}