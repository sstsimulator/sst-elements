// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
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
#include "testcpu/standardCPU.h"

#include <sst/core/params.h>
#include <sst/core/interfaces/stringEvent.h>

#include "util.h"

using namespace SST;
using namespace SST::Interfaces;
using namespace SST::MemHierarchy;
using namespace SST::Statistics;

/* Constructor */
standardCPU::standardCPU(ComponentId_t id, Params& params) :
    Component(id), rng(id, 13)
{
    // Restart the RNG to ensure completely consistent results
    // Seed with user-provided seed
    uint32_t z_seed = params.find<uint32_t>("rngseed", 7);
    rng.restart(z_seed, 13);

    out.init("", params.find<unsigned int>("verbose", 1), 0, Output::STDOUT);
    
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

    maxOutstanding = params.find<uint64_t>("maxOutstanding", 10);

    /* Required parameter - opCount */
    ops = params.find<uint64_t>("opCount", 0, found);
    sst_assert(found, CALL_INFO, -1, "%s, Error: parameter 'opCount' was not provided\n", getName().c_str());

    /* Frequency of different ops */
    unsigned readf = params.find<unsigned>("read_freq", 25);
    unsigned writef = params.find<unsigned>("write_freq", 75);
    unsigned flushf = params.find<unsigned>("flush_freq", 0);
    unsigned flushinvf = params.find<unsigned>("flushinv_freq", 0);
    unsigned customf = params.find<unsigned>("custom_freq", 0);
    unsigned llscf = params.find<unsigned>("llsc_freq", 0);
    unsigned mmiof = params.find<unsigned>("mmio_freq", 0);

    if (mmiof != 0 && mmioAddr == 0) {
        out.fatal(CALL_INFO, -1, "%s, Error: mmio_freq is > 0 but no mmio device has been specified via mmio_addr\n", getName().c_str());
    }

    high_mark = readf + writef + flushf + flushinvf + customf + llscf + mmiof; /* Numbers less than this and above other marks indicate read */
    if (high_mark == 0) {
        out.fatal(CALL_INFO, -1, "%s, Error: The input doesn't indicate a frequency for any command type.\n", getName().c_str());
    }
    write_mark = writef;    /* Numbers less than this indicate write */
    flush_mark = write_mark + flushf; /* Numbers less than this indicate flush */
    flushinv_mark = flush_mark + flushinvf; /* Numbers less than this indicate flush-inv */
    custom_mark = flushinv_mark + customf; /* Numbers less than this indicate flush */
    llsc_mark = custom_mark + llscf; /* Numbers less than this indicate LL-SC */
    mmio_mark = llsc_mark + mmiof; /* Numbers less than this indicate MMIO read or write */

    noncacheableRangeStart = params.find<uint64_t>("noncacheableRangeStart", 0);
    noncacheableRangeEnd = params.find<uint64_t>("noncacheableRangeEnd", 0);
    noncacheableSize = noncacheableRangeEnd - noncacheableRangeStart;

    maxReqsPerIssue = params.find<uint32_t>("reqsPerIssue", 1);
    if (maxReqsPerIssue < 1) {
        out.fatal(CALL_INFO, -1, "%s, Error: StandardCPU cannot issue less than one request at a time...fix your input deck\n", getName().c_str());
    }

    // Tell the simulator not to end until we OK it
    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

    //set our clock
    std::string clockFreq = params.find<std::string>("clock", "1GHz");
    clockHandler = new Clock::Handler<standardCPU>(this, &standardCPU::clockTic);
    clockTC = registerClock( clockFreq, clockHandler );

    /* Find the interface the user provided in the Python and load it*/
    memory = loadUserSubComponent<StandardMem>("memory", ComponentInfo::SHARE_NONE, clockTC, new StandardMem::Handler<standardCPU>(this, &standardCPU::handleEvent));

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
    ll_issued = false;
}

void standardCPU::init(unsigned int phase)
{
    memory->init(phase);
}

void standardCPU::setup() {
    memory->setup();
    lineSize = memory->getLineSize();
}

void standardCPU::finish() { }

// incoming events are scanned and deleted
void standardCPU::handleEvent(StandardMem::Request *req)
{
    std::map<uint64_t, std::pair<SimTime_t,std::string>>::iterator i = requests.find(req->getID());
    if ( requests.end() == i ) {
        out.fatal(CALL_INFO, -1, "Event (%" PRIx64 ") not found!\n", req->getID());
    } else {
        SimTime_t et = getCurrentSimTime() - i->second.first;
        if (i->second.second == "StoreConditional" && req->getSuccess())
            num_llsc_success->addData(1);
        requests.erase(i);
    }

    delete req;
}


bool standardCPU::clockTic( Cycle_t )
{
    ++clock_ticks;

    // Histogram bin the requests pending per cycle
    requestsPendingCycle->addData((uint64_t) requests.size());

    // communicate?
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
                
                if (ll_issued) {
                    req = createSC();
                    cmdString = "StoreConditional";
                }else if (instNum < write_mark) {
                    req = createWrite(addr);
                    cmdString = "Write";
                } else if (instNum < flush_mark) {
                    req = createFlush(addr);
                    cmdString = "Flush";
                } else if (instNum < flushinv_mark) {
                    req = createFlushInv(addr);
                    cmdString = "FlushInv";
                } else if (instNum < custom_mark) {
                } else if (instNum < llsc_mark) {
                    req = createLL(addr);
                    cmdString = "LoadLink";
                } else if (instNum < mmio_mark) {
                    bool opType = rng.generateNextUInt32() % 2;
                    if (opType) {
                        req = createMMIORead();
                        cmdString = "ReadMMIO";
                    } else {
                        req = createMMIOWrite();
                        cmdString = "WriteMMIO";
                    }
                } else {
                    req = createRead(addr);
                }

                if (req->needsResponse()) {
		    requests[req->getID()] =  std::make_pair(getCurrentSimTime(), cmdString);
                }
            
                memory->send(req);

                ops--;
	    }
        }
    }

    // Check whether to end the simulation
    if ( 0 == ops && requests.empty() ) {
        out.verbose(CALL_INFO, 1, 0, "StandardCPU: Test Completed Successfuly\n");
        primaryComponentOKToEndSim();
        return true;    // Turn our clock off while we wait for any other CPUs to end
    }

    // return false so we keep going
    return false;
}

/* Methods for sending different kinds of requests */
StandardMem::Request* standardCPU::createWrite(Addr addr) {
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

StandardMem::Request* standardCPU::createRead(Addr addr) {
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

StandardMem::Request* standardCPU::createFlush(Addr addr) {
    addr = ((addr % (maxAddr - noncacheableSize)>>2) << 2);
    if (addr >= noncacheableRangeStart && addr < noncacheableRangeEnd)
        addr += noncacheableRangeEnd;
    addr = addr - (addr % lineSize);
    StandardMem::Request* req = new Interfaces::StandardMem::FlushAddr(addr, lineSize, false, 10);
    num_flushes_issued->addData(1);
    out.verbose(CALL_INFO, 2, 0, "%s: %" PRIu64 " Issued FlushAddr for address 0x%" PRIx64 "\n", getName().c_str(), ops,  addr);
    return req;
}

StandardMem::Request* standardCPU::createFlushInv(Addr addr) {
    addr = ((addr % (maxAddr - noncacheableSize)>>2) << 2);
    if (addr >= noncacheableRangeStart && addr < noncacheableRangeEnd)
        addr += noncacheableRangeEnd;
    addr = addr - (addr % lineSize);
    StandardMem::Request* req = new Interfaces::StandardMem::FlushAddr(addr, lineSize, true, 10);
    num_flushinvs_issued->addData(1);
    out.verbose(CALL_INFO, 2, 0, "%s: %" PRIu64 " Issued FlushAddrInv for address 0x%" PRIx64 "\n", getName().c_str(), ops,  addr);
    return req;
}

StandardMem::Request* standardCPU::createLL(Addr addr) {
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

StandardMem::Request* standardCPU::createSC() {
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

StandardMem::Request* standardCPU::createMMIOWrite() {
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

StandardMem::Request* standardCPU::createMMIORead() {
    StandardMem::Request* req = new Interfaces::StandardMem::Read(mmioAddr, sizeof(int32_t));
    out.verbose(CALL_INFO, 2, 0, "%s: %" PRIu64 " Issued MMIO Read for address 0x%" PRIx64 "\n", getName().c_str(), ops, mmioAddr);
    return req;
}

void standardCPU::emergencyShutdown() {
    if (out.getVerboseLevel() > 1) {
        if (out.getOutputLocation() == Output::STDOUT)
            out.setOutputLocation(Output::STDERR);
        
        out.output("MemHierarchy::standardCPU %s\n", getName().c_str());
        out.output("  Outstanding events: %zu\n", requests.size());
        out.output("End MemHierarchy::standardCPU %s\n", getName().c_str());
    }
}
