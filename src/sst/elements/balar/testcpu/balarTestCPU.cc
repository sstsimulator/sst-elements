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
    Component(id), rng(id, 13)
{
    // Restart the RNG to ensure completely consistent results
    // Seed with user-provided seed
    uint32_t z_seed = params.find<uint32_t>("rngseed", 7);
    rng.restart(z_seed, 13);

    out.init("BalarTestCPU[@f:@l:@p] ", params.find<unsigned int>("verbose", 1), 0, Output::STDOUT);
    
    bool found;

    /* Required parameter - memFreq */
    memFreq = params.find<int>("memFreq", 0, found);
    if (!found) {
        out.fatal(CALL_INFO, -1,"%s, Error: parameter 'memFreq' was not provided\n", 
                getName().c_str());
    }

    /* Required parameter - memSize */
    UnitAlgebra memsize = params.find<UnitAlgebra>("memSize", UnitAlgebra("0B"), found);
    if ( !found ) {
        out.fatal(CALL_INFO, -1, "%s, Error: parameter 'memSize' was not provided\n",
                getName().c_str());
    }
    if (!(memsize.hasUnits("B"))) {
        out.fatal(CALL_INFO, -1, "%s, Error: memSize parameter requires units of 'B' (SI OK). You provided '%s'\n",
            getName().c_str(), memsize.toString().c_str() );
    }

    maxAddr = memsize.getRoundedValue() - 1;

    mmioAddr = params.find<uint64_t>("mmio_addr", "0", found);
    if (found) {
        sst_assert(mmioAddr > maxAddr, CALL_INFO, -1, "incompatible parameters: mmio_addr must be >= memSize (mmio above physical memory addresses).\n");
    }

    // TODO: Start and end range for gpu
    gpuAddr = params.find<uint64_t>("gpu_addr", "0", found);  // range for gpu address space
    // if (found) {
    //     sst_assert(gpuAddr > mmioAddr, CALL_INFO, -1, "incompatible parameters: gpu_addr must be >= mmio_addr (gpu_addr above mmio).\n");
    // }

    

    maxOutstanding = params.find<uint64_t>("maxOutstanding", 10);

    /* Required parameter - opCount */
    ops = params.find<uint64_t>("opCount", 0, found);
    sst_assert(found, CALL_INFO, -1, "%s, Error: parameter 'opCount' was not provided\n", getName().c_str());

    /* Frequency of different ops */
    // TODO Change to CUDA calls?
    unsigned readf = params.find<unsigned>("read_freq", 25);
    unsigned writef = params.find<unsigned>("write_freq", 75);
    unsigned flushf = params.find<unsigned>("flush_freq", 0);
    unsigned flushinvf = params.find<unsigned>("flushinv_freq", 0);
    unsigned customf = params.find<unsigned>("custom_freq", 0);
    unsigned llscf = params.find<unsigned>("llsc_freq", 0);
    unsigned mmiof = params.find<unsigned>("mmio_freq", 0);
    unsigned gpuf = params.find<unsigned>("gpu_freq", 0);   // gpu request frequency

    if (gpuf != 0 && gpuAddr == 0) {
        out.fatal(CALL_INFO, -1, "%s, Error: gpu_freq is > 0 but no gpu address space has been specified via gpu_addr\n", getName().c_str());
    }

    if (mmiof != 0 && mmioAddr == 0) {
        out.fatal(CALL_INFO, -1, "%s, Error: mmio_freq is > 0 but no mmio device has been specified via mmio_addr\n", getName().c_str());
    }

    high_mark = readf + writef + flushf + flushinvf + customf + llscf + mmiof + gpuf; /* Numbers less than this and above other marks indicate read */
    if (high_mark == 0) {
        out.fatal(CALL_INFO, -1, "%s, Error: The input doesn't indicate a frequency for any command type.\n", getName().c_str());
    }
    write_mark = writef;    /* Numbers less than this indicate write */
    flush_mark = write_mark + flushf; /* Numbers less than this indicate flush */
    flushinv_mark = flush_mark + flushinvf; /* Numbers less than this indicate flush-inv */
    custom_mark = flushinv_mark + customf; /* Numbers less than this indicate flush */
    llsc_mark = custom_mark + llscf; /* Numbers less than this indicate LL-SC */
    mmio_mark = llsc_mark + mmiof; /* Numbers less than this indicate MMIO read or write */
    gpu_mark = mmio_mark + gpuf;  /* Numbers less than this indicate gpu requests */

    noncacheableRangeStart = params.find<uint64_t>("noncacheableRangeStart", 0);
    noncacheableRangeEnd = params.find<uint64_t>("noncacheableRangeEnd", 0);
    noncacheableSize = noncacheableRangeEnd - noncacheableRangeStart;

    maxReqsPerIssue = params.find<uint32_t>("reqsPerIssue", 1);
    if (maxReqsPerIssue < 1) {
        out.fatal(CALL_INFO, -1, "%s, Error: BalarTestCPU cannot issue less than one request at a time...fix your input deck\n", getName().c_str());
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
    requestsPendingCycle = registerStatistic<uint64_t>("pendCycle");
    num_reads_issued = registerStatistic<uint64_t>("reads");
    num_writes_issued = registerStatistic<uint64_t>("writes");
    if (noncacheableSize != 0) {
        noncacheableReads = registerStatistic<uint64_t>("readNoncache");
        noncacheableWrites = registerStatistic<uint64_t>("writeNoncache");
    }
    if (flushf != 0 ) {
        num_flushes_issued = registerStatistic<uint64_t>("flushes");
    }
    if (flushinvf != 0) {
        num_flushinvs_issued = registerStatistic<uint64_t>("flushinvs");
    }
    if (customf != 0) {
        num_custom_issued = registerStatistic<uint64_t>("customReqs");
    }

    if (llscf != 0) {
        num_llsc_issued = registerStatistic<uint64_t>("llsc");
        num_llsc_success = registerStatistic<uint64_t>("llsc_success");
    }

    if (gpuf != 0) {
        num_gpu_issued = registerStatistic<uint64_t>("gpu");
        num_llsc_success = registerStatistic<uint64_t>("gpu_success");
    }
    ll_issued = false;

    // Trace parser initialization
    std::string traceFile = params.find<std::string>("trace_file", "cuda_calls.trace");
    std::string cudaExecutable = params.find<std::string>("cuda_executable", found);
    sst_assert(found, CALL_INFO, -1, "%s, Error: parameter 'cuda_executable' was not provided\n", getName().c_str());
    out.verbose(CALL_INFO, 2, 0, "Trace file: %s, cuda_executable: %s\n", traceFile.c_str(), cudaExecutable.c_str());

    // Bind response handler to cpu
    handlers = new mmioHandlers(this, &out);

    // Bind trace parser
    trace_parser = new CudaAPITraceParser(this, &out, traceFile, cudaExecutable);
}

void BalarTestCPU::init(unsigned int phase)
{
    memory->init(phase);
}

void BalarTestCPU::setup() {
    memory->setup();
    lineSize = memory->getLineSize();
}

void BalarTestCPU::finish() { }

// incoming events are scanned and deleted
// TODO: Handle response here?
void BalarTestCPU::handleEvent(StandardMem::Request *req)
{
    // TODO: Create a handler class here to handle incoming requests response?
    req->handle(handlers);

    // std::map<uint64_t, std::pair<SimTime_t,std::string>>::iterator i = requests.find(req->getID());
    // if ( requests.end() == i ) {
    //     out.fatal(CALL_INFO, -1, "Event (%" PRIx64 ") not found!\n", req->getID());
    // } else {
    //     SimTime_t et = getCurrentSimTime() - i->second.first;
    //     if (i->second.second == "StoreConditional" && req->getSuccess())
    //         num_llsc_success->addData(1);
    //     requests.erase(i);
    // }

    // // TODO: Check if the gpu call's return are correct

    // delete req;
}

bool BalarTestCPU::clockTic( Cycle_t ) {
    ++clock_ticks;

    Interfaces::StandardMem::Request* req = trace_parser->getNextCall();
    if (!(req == nullptr)) {
        // Send gpu cuda call request
        memory->send(req);

        // Add cuda call requests to pending map
        std::string cmdString = "Read";
        requests[req->getID()] = std::make_pair(getCurrentSimTime(), cmdString);

        // Check cudal call retval
        req = checkCudaReturn();
        memory->send(req);
        requests[req->getID()] = std::make_pair(getCurrentSimTime(), cmdString);
    }

    // Check whether to end the simulation
    if ( req == nullptr && requests.empty() ) {
        out.verbose(CALL_INFO, 1, 0, "BalarTestCPU: Test Completed Successfuly\n");
        primaryComponentOKToEndSim();
        return true;    // Turn our clock off while we wait for any other CPUs to end
    }

    // return false so we keep going
    return false;
}

/** Original clockTic
bool BalarTestCPU::clockTic( Cycle_t )
{
    ++clock_ticks;

    // Histogram bin the requests pending per cycle
    requestsPendingCycle->addData((uint64_t) requests.size());

    // communicate?
    // TODO: Need to manually create a call sequence to cuda functions
    if ((0 != ops) && (0 == (rng.generateNextUInt32() % memFreq))) {
        if ( requests.size() < maxOutstanding ) {
            // yes, communicate
            // create event
            // x4 to prevent splitting blocks
            uint32_t reqsToSend = 1;
            if (maxReqsPerIssue > 1) reqsToSend += rng.generateNextUInt32() % maxReqsPerIssue;
            if (reqsToSend > (maxOutstanding - requests.size())) reqsToSend = maxOutstanding - requests.size();
            if (reqsToSend > ops) reqsToSend = ops;

            for (int i = 0; i < reqsToSend; i++) {

                StandardMem::Addr addr = rng.generateNextUInt64();

                std::vector<uint8_t> data;
                data.resize(4);
                data[0] = (addr >> 24) & 0xff;
                data[1] = (addr >> 16) & 0xff;
                data[2] = (addr >>  8) & 0xff;
                data[3] = (addr >>  0) & 0xff;
                
                uint32_t instNum = rng.generateNextUInt32() % high_mark;
                uint64_t size = 4;
                std::string cmdString = "Read";
                Interfaces::StandardMem::Request* req;

                // Send gpu cuda call request
                req = createGPUReq();
		        memory->send(req);

                // Add cuda call requests to pending map
                requests[req->getID()] = std::make_pair(getCurrentSimTime(), cmdString);

                // Check cudal call retval
                req = checkCudaReturn();
		        memory->send(req);
                requests[req->getID()] = std::make_pair(getCurrentSimTime(), cmdString);

                ops--;
	        }
        }
    }

    // Check whether to end the simulation
    if ( 0 == ops && requests.empty() ) {
        out.verbose(CALL_INFO, 1, 0, "BalarTestCPU: Test Completed Successfuly\n");
        primaryComponentOKToEndSim();
        return true;    // Turn our clock off while we wait for any other CPUs to end
    }

    // return false so we keep going
    return false;
}*/

/* Methods for sending different kinds of requests */
StandardMem::Request* BalarTestCPU::createWrite(Addr addr) {
    addr = ((addr % maxAddr)>>2) << 2;
    // Dummy payload
    std::vector<uint8_t> data;
    data.resize(4);
    data[0] = (addr >> 24) & 0xff;
    data[1] = (addr >> 16) & 0xff;
    data[2] = (addr >>  8) & 0xff;
    data[3] = (addr >>  0) & 0xff;

    StandardMem::Request* req = new Interfaces::StandardMem::Write(addr, data.size(), data);
    num_writes_issued->addData(1);
    if (addr >= noncacheableRangeStart && addr < noncacheableRangeEnd) {
        req->setNoncacheable();
        noncacheableWrites->addData(1);
    }
    out.verbose(CALL_INFO, 2, 0, "%s: %" PRIu64 " Issued %sWrite for address 0x%" PRIx64 "\n", getName().c_str(), ops, req->getNoncacheable() ? "Noncacheable " : "", addr);
    return req;
}

StandardMem::Request* BalarTestCPU::createRead(Addr addr) {
    addr = ((addr % maxAddr)>>2) << 2;
    StandardMem::Request* req = new Interfaces::StandardMem::Read(addr, 4);
    num_reads_issued->addData(1);
    if (addr >= noncacheableRangeStart && addr < noncacheableRangeEnd) {
        req->setNoncacheable();
        noncacheableReads->addData(1);
    }
    out.verbose(CALL_INFO, 2, 0, "%s: %" PRIu64 " Issued %sRead for address 0x%" PRIx64 "\n", getName().c_str(), ops, req->getNoncacheable() ? "Noncacheable " : "", addr);
    return req;
}

StandardMem::Request* BalarTestCPU::createFlush(Addr addr) {
    addr = ((addr % (maxAddr - noncacheableSize)>>2) << 2);
    if (addr >= noncacheableRangeStart && addr < noncacheableRangeEnd)
        addr += noncacheableRangeEnd;
    addr = addr - (addr % lineSize);
    StandardMem::Request* req = new Interfaces::StandardMem::FlushAddr(addr, lineSize, false, 10);
    num_flushes_issued->addData(1);
    out.verbose(CALL_INFO, 2, 0, "%s: %" PRIu64 " Issued FlushAddr for address 0x%" PRIx64 "\n", getName().c_str(), ops,  addr);
    return req;
}

StandardMem::Request* BalarTestCPU::createFlushInv(Addr addr) {
    addr = ((addr % (maxAddr - noncacheableSize)>>2) << 2);
    if (addr >= noncacheableRangeStart && addr < noncacheableRangeEnd)
        addr += noncacheableRangeEnd;
    addr = addr - (addr % lineSize);
    StandardMem::Request* req = new Interfaces::StandardMem::FlushAddr(addr, lineSize, true, 10);
    num_flushinvs_issued->addData(1);
    out.verbose(CALL_INFO, 2, 0, "%s: %" PRIu64 " Issued FlushAddrInv for address 0x%" PRIx64 "\n", getName().c_str(), ops,  addr);
    return req;
}

StandardMem::Request* BalarTestCPU::createLL(Addr addr) {
    // Addr needs to be a cacheable range
    Addr cacheableSize = maxAddr + 1 - noncacheableRangeEnd + noncacheableRangeStart;
    addr = (addr % (cacheableSize >> 2)) << 2;
    if (addr >= noncacheableRangeStart && addr < noncacheableRangeEnd) {
        addr += noncacheableRangeEnd;
    }
    // Align addr
    addr = (addr >> 2) << 2;

    StandardMem::Request* req = new Interfaces::StandardMem::LoadLink(addr, 4);
    // Set these so we issue a matching sc 
    ll_addr = addr;
    ll_issued = true;

    out.verbose(CALL_INFO, 2, 0, "%s: %" PRIu64 " Issued LoadLink for address 0x%" PRIx64 "\n", getName().c_str(), ops, addr);
    return req;
}

StandardMem::Request* BalarTestCPU::createSC() {
    std::vector<uint8_t> data;
    data.resize(4);
    data[0] = (ll_addr >> 24) & 0xff;
    data[1] = (ll_addr >> 16) & 0xff;
    data[2] = (ll_addr >>  8) & 0xff;
    data[3] = (ll_addr >>  0) & 0xff;
    StandardMem::Request* req = new Interfaces::StandardMem::StoreConditional(ll_addr, data.size(), data);
    num_llsc_issued->addData(1);
    ll_issued = false;
    out.verbose(CALL_INFO, 2, 0, "%s: %" PRIu64 " Issued StoreConditional for address 0x%" PRIx64 "\n", getName().c_str(), ops, ll_addr);
    return req;
}

StandardMem::Request* BalarTestCPU::createMMIOWrite() {
    bool posted = rng.generateNextUInt32() % 2;
    int32_t payload = rng.generateNextInt32();
    payload >>= 16; // Shrink the number a bit
    int32_t payload_cp = payload;
    std::vector<uint8_t> data;
    for (int i = 0; i < sizeof(int32_t); i++) {
        data.push_back(payload & 0xFF);
        payload >>=8;
    }
    StandardMem::Request* req = new Interfaces::StandardMem::Write(mmioAddr, sizeof(int32_t), data, posted);
    out.verbose(CALL_INFO, 2, 0, "%s: %" PRIu64 " Issued MMIO Write for address 0x%" PRIx64 " with payload %d\n", getName().c_str(), ops, mmioAddr, payload_cp);
    return req;
}

StandardMem::Request* BalarTestCPU::createMMIORead() {
    StandardMem::Request* req = new Interfaces::StandardMem::Read(mmioAddr, sizeof(int32_t));
    out.verbose(CALL_INFO, 2, 0, "%s: %" PRIu64 " Issued MMIO Read for address 0x%" PRIx64 "\n", getName().c_str(), ops, mmioAddr);
    return req;
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

Interfaces::StandardMem::Request* BalarTestCPU::createGPUReq() {
    BalarCudaCallPacket_t *pack_ptr =  new BalarCudaCallPacket_t();
    uint8_t funcType = rng.generateNextUInt32() % 11 + 1;
    enum GpuApi_t cuda_call_id = (enum GpuApi_t)(funcType * 2 - 1);
    pack_ptr->cuda_call_id = cuda_call_id; 
    vector<uint8_t> *buffer = encode_balar_packet<BalarCudaCallPacket_t>(pack_ptr);

    StandardMem::Request* req = new Interfaces::StandardMem::Write(gpuAddr, buffer->size(), *buffer, false);
    // TODO: Write Request for parameters to gpu address
    num_gpu_issued->addData(1);

    out.verbose(_INFO_, "GPU request sent %s, CUDA Function enum %s\n", getName().c_str(), gpu_api_to_string(cuda_call_id)->c_str());
    return req;
}

Interfaces::StandardMem::Request* 
    BalarTestCPU::createGPUReqFromPacket(BalarCudaCallPacket_t pack) {
    vector<uint8_t> *buffer = encode_balar_packet<BalarCudaCallPacket_t>(&pack);

    StandardMem::Request* req = new Interfaces::StandardMem::Write(gpuAddr, buffer->size(), *buffer, false);
    num_gpu_issued->addData(1);

    out.verbose(_INFO_, "creating GPU request %s, CUDA Function enum %s\n", getName().c_str(), gpu_api_to_string(pack.cuda_call_id)->c_str());
    return req;
}


Interfaces::StandardMem::Request* BalarTestCPU::checkCudaReturn() {
    // TODO Check last packet send now
    StandardMem::Request* req = new Interfaces::StandardMem::Read(mmioAddr, sizeof(BalarCudaCallReturnPacket_t));
    out.verbose(_INFO_,  "%s: %" PRIu64 " creating Cuda return value Read for address 0x%" PRIx64 "\n", getName().c_str(), ops, mmioAddr);
    return req;
}

/**
 * @brief mmio event handler for a read we issued
 *        Check the pending map for the request corresponding to the response
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
        vector<uint8_t> *data_ptr = &(resp->data);
        BalarCudaCallReturnPacket_t *ret_pack_ptr = decode_balar_packet<BalarCudaCallReturnPacket_t>(data_ptr);
        out->verbose(_INFO_, "%s: get response from read request (%d) with enum: \"%s\"\n", cpu->getName().c_str(), resp->getID(), gpu_api_to_string(ret_pack_ptr->cuda_call_id)->c_str());

        enum GpuApi_t api_type = ret_pack_ptr->cuda_call_id;
        if (api_type == GPU_REG_FAT_BINARY) {
            out->verbose(_INFO_, "Fatbin handle: %d\n", ret_pack_ptr->fat_cubin_handle);
        }

        // TODO Handle return values from API

        cpu->requests.erase(i);
    }

    delete resp;
}

/**
 * @brief mmio event handler for a write we issued
 *        Check the pending map for the request corresponding to the response
 *        Remove that request after finish handling it.
 * 
 * @param resp 
 */
void BalarTestCPU::mmioHandlers::handle(Interfaces::StandardMem::WriteResp* resp) {
    // Find the request from pending requests map
    std::map<uint64_t, std::pair<SimTime_t,std::string>>::iterator i = cpu->requests.find(resp->getID());
    if ( cpu->requests.end() == i ) {
        out->fatal(_INFO_, "Event (%" PRIx64 ") not found!\n", resp->getID());
    } else {
        out->verbose(_INFO_, "%s: get response from write request (%d)\n", cpu->getName().c_str(), resp->getID());
        cpu->requests.erase(i);
    }
    delete resp;
}

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

    // TODO Inserting initialization request like register fatbinary to initReqs
    Interfaces::StandardMem::Request* req;
    BalarCudaCallPacket_t fatbin_pack;
    fatbin_pack.cuda_call_id = GPU_REG_FAT_BINARY;
    strcpy(fatbin_pack.register_fatbin.file_name, cudaExecutable.c_str());
    req = cpu->createGPUReqFromPacket(fatbin_pack);
    initReqs->push(req);
}

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
            } else if (cudaCallType.find("memcpyH2D") != std::string::npos) {
                // TODO Need to wait for malloc to be finished
                pack.cuda_call_id = GPU_MEMCPY;
                pack.cuda_memcpy.kind = cudaMemcpyHostToDevice;

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
                uint8_t *host_data = (uint8_t *) malloc(sizeof(uint8_t) * size);
                dataStream.read((char *)host_data, size);

                // Get device pointer value
                CUdeviceptr* dptr = nullptr;
                auto pair = dptr_map->find(dptr_name);
                if (pair == dptr_map->end()) {
                    out->fatal(CALL_INFO, -1,"Error: device pointer: '%s' not exist in hashmap\n", dptr_name.c_str());
                } else {
                    dptr = pair->second;
                }

                // Prepare pack
                out->verbose(CALL_INFO, 2, 0, "MemcpyH2D Device pointer (%s) addr: %p val: %p\n", dptr_name.c_str(), dptr, *dptr);

                pack.cuda_memcpy.dst = *dptr;
                pack.cuda_memcpy.src = (uint64_t) host_data;
                pack.cuda_memcpy.count = size;
                pack.cuda_memcpy.payload = host_data;

                // Create request
                req = cpu->createGPUReqFromPacket(pack);   
            } else if (cudaCallType.find("memcpyD2H") != std::string::npos || cudaCallType.find("memalloc") != std::string::npos) {
                pack.cuda_call_id = GPU_MEMCPY;
                pack.cuda_memcpy.kind = cudaMemcpyDeviceToHost;
                
            } else if (cudaCallType.find("kernel launch") != std::string::npos) {
                
            } else if (cudaCallType.find("free") != std::string::npos) {
                
            }
        }
        return req;
    }
}