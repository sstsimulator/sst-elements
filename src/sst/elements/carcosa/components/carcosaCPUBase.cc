// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
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
#include "sst/elements/carcosa/components/carcosaCPUBase.h"
#include "sst/elements/carcosa/components/cpuEvent.h"
#include <sst/core/params.h>
#include <sst/core/interfaces/stringEvent.h>

#include "sst/elements/memHierarchy/util.h"
using namespace SST;
using namespace SST::Interfaces;
using namespace SST::MemHierarchy;
using namespace SST::Statistics;

CarcosaCPUBase::CarcosaCPUBase(ComponentId_t id, Params& params) :
    Component(id), rng(id, 13)
{
    requireLibrary("memHierarchy");
    uint32_t z_seed = params.find<uint32_t>("rngseed", 7);
    rng.restart(z_seed, 13);

    out.init("", params.find<unsigned int>("verbose", 1), 0, Output::STDOUT);

    bool found;

    memFreq = params.find<int>("memFreq", 1000, found);
    if (!found) {
        out.fatal(CALL_INFO, -1, "%s, Error: parameter 'memFreq' was not provided\n", getName().c_str());
    }
    if (memFreq == 0) {
        out.fatal(CALL_INFO, -1, "%s, Error: parameter 'memFreq' cannot be 0 (division by zero would occur)\n", getName().c_str());
    }

    UnitAlgebra memsize = params.find<UnitAlgebra>("memSize", UnitAlgebra("0B"), found);
    if (!found) {
        out.fatal(CALL_INFO, -1, "%s, Error: parameter 'memSize' was not provided\n", getName().c_str());
    }
    if (!(memsize.hasUnits("B"))) {
        out.fatal(CALL_INFO, -1, "%s, Error: memSize parameter requires units of 'B' (SI OK). You provided '%s'\n",
            getName().c_str(), memsize.toString().c_str());
    }

    maxAddr = memsize.getRoundedValue() - 1;

    mmioAddr = params.find<uint64_t>("mmio_addr", "0", found);
    if (found) {
        sst_assert(mmioAddr > maxAddr, CALL_INFO, -1, "incompatible parameters: mmio_addr must be >= memSize (mmio above physical memory addresses).\n");
    }

    maxOutstanding = params.find<uint64_t>("maxOutstanding", 10);

    ops = params.find<uint64_t>("opCount", 0, found);
    sst_assert(found, CALL_INFO, -1, "%s, Error: parameter 'opCount' was not provided\n", getName().c_str());

    unsigned readf    = params.find<unsigned>("read_freq",     25);
    unsigned writef   = params.find<unsigned>("write_freq",    75);
    unsigned flushf   = params.find<unsigned>("flush_freq",     0);
    unsigned flushinvf = params.find<unsigned>("flushinv_freq", 0);
    unsigned customf  = params.find<unsigned>("custom_freq",    0);
    unsigned llscf    = params.find<unsigned>("llsc_freq",      0);
    unsigned mmiof    = params.find<unsigned>("mmio_freq",      0);

    if (mmiof != 0 && mmioAddr == 0) {
        out.fatal(CALL_INFO, -1, "%s, Error: mmio_freq is > 0 but no mmio device has been specified via mmio_addr\n", getName().c_str());
    }

    high_mark     = readf + writef + flushf + flushinvf + customf + llscf + mmiof;
    if (high_mark == 0) {
        out.fatal(CALL_INFO, -1, "%s, Error: The input doesn't indicate a frequency for any command type.\n", getName().c_str());
    }
    write_mark    = writef;
    flush_mark    = write_mark + flushf;
    flushinv_mark = flush_mark + flushinvf;
    custom_mark   = flushinv_mark + customf;
    llsc_mark     = custom_mark + llscf;
    mmio_mark     = llsc_mark + mmiof;

    noncacheableRangeStart = params.find<uint64_t>("noncacheableRangeStart", 0);
    noncacheableRangeEnd   = params.find<uint64_t>("noncacheableRangeEnd",   0);
    noncacheableSize = noncacheableRangeEnd - noncacheableRangeStart;

    maxReqsPerIssue = params.find<uint32_t>("reqsPerIssue", 1);
    if (maxReqsPerIssue < 1) {
        out.fatal(CALL_INFO, -1, "%s, Error: StandardCPU cannot issue less than one request at a time...fix your input deck\n", getName().c_str());
    }

    HaliLink = configureLink("haliToCPU", new Event::Handler<CarcosaCPUBase, &CarcosaCPUBase::handleCpuEvent>(this));
    if (!HaliLink) {
        out.fatal(CALL_INFO, -1, "%s, Error: 'haliToCPU' port is not connected\n", getName().c_str());
    }

    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

    std::string clockFreq = params.find<std::string>("clock", "1GHz");
    clockHandler = new Clock::Handler<CarcosaCPUBase, &CarcosaCPUBase::clockTic>(this);
    clockTC = registerClock(clockFreq, clockHandler);

    memory = loadUserSubComponent<StandardMem>("memory", ComponentInfo::SHARE_NONE, clockTC,
        new StandardMem::Handler<CarcosaCPUBase, &CarcosaCPUBase::handleEvent>(this));

    if (!memory) {
        out.fatal(CALL_INFO, -1, "Unable to load memHierarchy.standardInterface subcomponent; check that 'memory' slot is filled in input.\n");
    }

    clock_ticks = 0;
    requestsPendingCycle = registerStatistic<uint64_t>("pendCycle");
    num_reads_issued     = registerStatistic<uint64_t>("reads");
    num_writes_issued    = registerStatistic<uint64_t>("writes");
    if (noncacheableSize != 0) {
        noncacheableReads  = registerStatistic<uint64_t>("readNoncache");
        noncacheableWrites = registerStatistic<uint64_t>("writeNoncache");
    }
    if (flushf != 0)     num_flushes_issued   = registerStatistic<uint64_t>("flushes");
    if (flushinvf != 0)  num_flushinvs_issued = registerStatistic<uint64_t>("flushinvs");
    if (customf != 0)    num_custom_issued     = registerStatistic<uint64_t>("customReqs");
    if (llscf != 0) {
        num_llsc_issued   = registerStatistic<uint64_t>("llsc");
        num_llsc_success  = registerStatistic<uint64_t>("llsc_success");
    }
    ll_issued = false;
}

void CarcosaCPUBase::handleCpuEvent(SST::Event *ev)
{
    Carcosa::CpuEvent *event = dynamic_cast<Carcosa::CpuEvent*>(ev);
    if (event) {
        delete event;
    } else {
        out.output("ERROR: Unexpected event type received\n");
    }
}

void CarcosaCPUBase::init(unsigned int phase)
{
    memory->init(phase);
}

void CarcosaCPUBase::setup() {
    memory->setup();
    lineSize = memory->getLineSize();
}

void CarcosaCPUBase::finish() { }

void CarcosaCPUBase::handleEvent(StandardMem::Request *req)
{
    std::map<uint64_t, std::pair<SimTime_t,std::string>>::iterator i = requests.find(req->getID());
    if (requests.end() == i) {
        out.fatal(CALL_INFO, -1, "Event (%" PRIx64 ") not found!\n", req->getID());
    } else {
        if (i->second.second == "StoreConditional" && req->getSuccess())
            num_llsc_success->addData(1);
        requests.erase(i);
    }
#ifdef __SST_DEBUG_OUTPUT__
    out.output("Received Response (%s), %6zu pending requests.\n",
               req->getString().c_str(), requests.size());
#endif
    delete req;
}

bool CarcosaCPUBase::clockTic(Cycle_t)
{
    if (clock_ticks % 1000 == 0) {
#ifdef __SST_DEBUG_OUTPUT__
        out.output("test1 open from carcosa\n");
#endif
        Carcosa::CpuEvent *ev = new Carcosa::CpuEvent("test1.txt", 0, 200);
        HaliLink->send(ev);
        Interfaces::StandardMem::Request* req = createFlushCache();
        if (req->needsResponse()) {
            requests[req->getID()] = std::make_pair(getCurrentSimTime(), std::string("FlushCache"));
        }
        memory->send(req);
    } else if (clock_ticks % 500 == 0) {
#ifdef __SST_DEBUG_OUTPUT__
        out.output("test2 open from carcosa\n");
#endif
        Carcosa::CpuEvent *ev = new Carcosa::CpuEvent("test2.txt", 0, 200);
        HaliLink->send(ev);
    }
    ++clock_ticks;

    requestsPendingCycle->addData((uint64_t)requests.size());

#ifdef __SST_DEBUG_OUTPUT__
    out.output("ops %" PRIu64 ", memFreq %" PRIu64 "\n", ops, memFreq);
#endif
    if ((0 != ops) && (0 == (rng.generateNextUInt32() % memFreq))) {
        if (requests.size() < maxOutstanding) {
            uint32_t reqsToSend = 1;
            if (maxReqsPerIssue > 1) reqsToSend += rng.generateNextUInt32() % maxReqsPerIssue;
            if (reqsToSend > (maxOutstanding - requests.size())) reqsToSend = maxOutstanding - requests.size();
            if (reqsToSend > ops) reqsToSend = ops;

            for (int i = 0; i < (int)reqsToSend; i++) {
                StandardMem::Addr addr = rng.generateNextUInt64() % 200;
                rng.generateNextUInt32(); // consume RNG to maintain deterministic sequence
                Interfaces::StandardMem::Request* req = createRead(addr);
                if (req->needsResponse()) {
                    requests[req->getID()] = std::make_pair(getCurrentSimTime(), std::string("Read"));
                }
                memory->send(req);
                ops--;
            }
        }
    }

    if (0 == ops && requests.empty()) {
        out.verbose(CALL_INFO, 1, 0, "%s: Test Completed Successfully\n", getName().c_str());
        primaryComponentOKToEndSim();
        return true;
    }
    return false;
}

StandardMem::Request* CarcosaCPUBase::createWrite(Addr addr) {
    addr = ((addr % maxAddr) >> 2) << 2;
    std::vector<uint8_t> data(4);
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
    out.verbose(CALL_INFO, 2, 0, "%s: %" PRIu64 " Issued %sWrite for address 0x%" PRIx64 "\n",
                getName().c_str(), ops, req->getNoncacheable() ? "Noncacheable " : "", addr);
    return req;
}

StandardMem::Request* CarcosaCPUBase::createRead(Addr addr) {
    addr = ((addr % maxAddr) >> 2) << 2;
    StandardMem::Request* req = new Interfaces::StandardMem::Read(addr, 4);
    num_reads_issued->addData(1);
    if (addr >= noncacheableRangeStart && addr < noncacheableRangeEnd) {
#ifdef __SST_DEBUG_OUTPUT__
        out.output("non cacheable read\n");
#endif
        req->setNoncacheable();
        noncacheableReads->addData(1);
    }
#ifdef __SST_DEBUG_OUTPUT__
    out.output("%s: %" PRIu64 " Issued %sRead for address 0x%" PRIx64 "\n",
               getName().c_str(), ops, req->getNoncacheable() ? "Noncacheable " : "", addr);
#endif
    return req;
}

StandardMem::Request* CarcosaCPUBase::createFlush(Addr addr) {
    addr = ((addr % (maxAddr - noncacheableSize) >> 2) << 2);
    if (addr >= noncacheableRangeStart && addr < noncacheableRangeEnd)
        addr += noncacheableRangeEnd;
    addr = addr - (addr % lineSize);
    StandardMem::Request* req = new Interfaces::StandardMem::FlushAddr(addr, lineSize, false, 10);
    num_flushes_issued->addData(1);
    out.verbose(CALL_INFO, 2, 0, "%s: %" PRIu64 " Issued FlushAddr for address 0x%" PRIx64 "\n",
                getName().c_str(), ops, addr);
    return req;
}

StandardMem::Request* CarcosaCPUBase::createFlushInv(Addr addr) {
    addr = ((addr % (maxAddr - noncacheableSize) >> 2) << 2);
    if (addr >= noncacheableRangeStart && addr < noncacheableRangeEnd)
        addr += noncacheableRangeEnd;
    addr = addr - (addr % lineSize);
    StandardMem::Request* req = new Interfaces::StandardMem::FlushAddr(addr, lineSize, true, 10);
    num_flushinvs_issued->addData(1);
    out.verbose(CALL_INFO, 2, 0, "%s: %" PRIu64 " Issued FlushAddrInv for address 0x%" PRIx64 "\n",
                getName().c_str(), ops, addr);
    return req;
}

StandardMem::Request* CarcosaCPUBase::createFlushCache() {
    return new Interfaces::StandardMem::FlushCache();
}

StandardMem::Request* CarcosaCPUBase::createLL(Addr addr) {
    Addr cacheableSize = maxAddr + 1 - noncacheableRangeEnd + noncacheableRangeStart;
    addr = (addr % (cacheableSize >> 2)) << 2;
    if (addr >= noncacheableRangeStart && addr < noncacheableRangeEnd)
        addr += noncacheableRangeEnd;
    addr = (addr >> 2) << 2;
    StandardMem::Request* req = new Interfaces::StandardMem::LoadLink(addr, 4);
    ll_addr = addr;
    ll_issued = true;
    out.verbose(CALL_INFO, 2, 0, "%s: %" PRIu64 " Issued LoadLink for address 0x%" PRIx64 "\n",
                getName().c_str(), ops, addr);
    return req;
}

StandardMem::Request* CarcosaCPUBase::createSC() {
    std::vector<uint8_t> data(4);
    data[0] = (ll_addr >> 24) & 0xff;
    data[1] = (ll_addr >> 16) & 0xff;
    data[2] = (ll_addr >>  8) & 0xff;
    data[3] = (ll_addr >>  0) & 0xff;
    StandardMem::Request* req = new Interfaces::StandardMem::StoreConditional(ll_addr, data.size(), data);
    num_llsc_issued->addData(1);
    ll_issued = false;
    out.verbose(CALL_INFO, 2, 0, "%s: %" PRIu64 " Issued StoreConditional for address 0x%" PRIx64 "\n",
                getName().c_str(), ops, ll_addr);
    return req;
}

StandardMem::Request* CarcosaCPUBase::createMMIOWrite() {
    bool posted = rng.generateNextUInt32() % 2;
    int32_t payload = rng.generateNextInt32();
    payload >>= 16;
    int32_t payload_cp = payload;
    std::vector<uint8_t> data;
    for (int i = 0; i < (int)sizeof(int32_t); i++) {
        data.push_back(payload & 0xFF);
        payload >>= 8;
    }
    StandardMem::Request* req = new Interfaces::StandardMem::Write(mmioAddr, sizeof(int32_t), data, posted);
    out.verbose(CALL_INFO, 2, 0, "%s: %" PRIu64 " Issued MMIO Write for address 0x%" PRIx64 " with payload %d\n",
                getName().c_str(), ops, mmioAddr, payload_cp);
    return req;
}

StandardMem::Request* CarcosaCPUBase::createMMIORead() {
    StandardMem::Request* req = new Interfaces::StandardMem::Read(mmioAddr, sizeof(int32_t));
    out.verbose(CALL_INFO, 2, 0, "%s: %" PRIu64 " Issued MMIO Read for address 0x%" PRIx64 "\n",
                getName().c_str(), ops, mmioAddr);
    return req;
}

void CarcosaCPUBase::emergencyShutdown() {
    if (out.getVerboseLevel() > 1) {
        if (out.getOutputLocation() == Output::STDOUT)
            out.setOutputLocation(Output::STDERR);
        out.output("CarcosaCPUBase %s\n", getName().c_str());
        out.output("  Outstanding events: %zu\n", requests.size());
        out.output("End CarcosaCPUBase %s\n", getName().c_str());
    }
}
