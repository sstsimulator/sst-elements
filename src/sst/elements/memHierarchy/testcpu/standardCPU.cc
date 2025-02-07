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
    Component(id), rng_(id, 13), rng_comm_(id, 13)
{
    // Restart the RNG to ensure completely consistent results
    // Seed with user-provided seed
    uint32_t z_seed = params.find<uint32_t>("rngseed", 7);
    rng_.restart(z_seed, 13);

    z_seed = params.find<uint32_t>("rngseed_comm", 7);
    rng_comm_.restart(z_seed, 13);

    out_.init("", params.find<unsigned int>("verbose", 1), 0, Output::STDOUT);
    
    bool found;

    /* Required parameter - memFreq */
    mem_freq_ = params.find<int>("memFreq", 0, found);
    if (!found) {
        out_.fatal(CALL_INFO, -1,"%s, Error: parameter 'memFreq' was not provided\n", 
                getName().c_str());
    }

    /* Required parameter - memSize */
    UnitAlgebra memsize = params.find<UnitAlgebra>("memSize", UnitAlgebra("0B"), found);
    if ( !found ) {
        out_.fatal(CALL_INFO, -1, "%s, Error: parameter 'memSize' was not provided\n",
                getName().c_str());
    }
    if (!(memsize.hasUnits("B"))) {
        out_.fatal(CALL_INFO, -1, "%s, Error: memSize parameter requires units of 'B' (SI OK). You provided '%s'\n",
            getName().c_str(), memsize.toString().c_str() );
    }

    max_addr_ = memsize.getRoundedValue() - 1;

    mmio_addr_ = params.find<uint64_t>("mmio_addr", "0", found);
    if (found) {
        sst_assert(mmio_addr_ > max_addr_, CALL_INFO, -1, "incompatible parameters: mmio_addr must be >= memSize (mmio above physical memory addresses).\n");
    }

    max_outstanding_ = params.find<uint64_t>("maxOutstanding", 10);

    /* Required parameter - opCount */
    op_count_ = params.find<uint64_t>("opCount", 0, found);
    sst_assert(found, CALL_INFO, -1, "%s, Error: parameter 'opCount' was not provided\n", getName().c_str());

    /* Frequency of different ops */
    unsigned readf = params.find<unsigned>("read_freq", 25);
    unsigned writef = params.find<unsigned>("write_freq", 75);
    unsigned flushf = params.find<unsigned>("flush_freq", 0);
    unsigned flushinvf = params.find<unsigned>("flushinv_freq", 0);
    unsigned flushcachef = params.find<unsigned>("flushcache_freq", 0);
    unsigned customf = params.find<unsigned>("custom_freq", 0);
    unsigned llscf = params.find<unsigned>("llsc_freq", 0);
    unsigned mmiof = params.find<unsigned>("mmio_freq", 0);

    if (mmiof != 0 && mmio_addr_ == 0) {
        out_.fatal(CALL_INFO, -1, "%s, Error: mmio_freq is > 0 but no mmio device has been specified via mmio_addr\n", getName().c_str());
    }

    high_mark_ = readf + writef + flushf + flushinvf + flushcachef + customf + llscf + mmiof; /* Numbers less than this and above other marks indicate read */
    if (high_mark_ == 0) {
        out_.fatal(CALL_INFO, -1, "%s, Error: The input doesn't indicate a frequency for any command type.\n", getName().c_str());
    }
    write_mark_ = writef;    /* Numbers less than this indicate write */
    flush_mark_ = write_mark_ + flushf; /* Numbers less than this indicate flush */
    flushinv_mark_ = flush_mark_ + flushinvf; /* Numbers less than this indicate flush-inv */
    flushcache_mark_ = flushinv_mark_ + flushcachef; /* Numbers less than this indicate flush-cache */
    custom_mark_ = flushcache_mark_ + customf; /* Numbers less than this indicate flush */
    llsc_mark_ = custom_mark_ + llscf; /* Numbers less than this indicate LL-SC */
    mmio_mark_ = llsc_mark_ + mmiof; /* Numbers less than this indicate MMIO read or write */
    
    noncacheable_range_start_ = params.find<uint64_t>("noncacheableRangeStart", 0);
    noncacheable_range_end_ = params.find<uint64_t>("noncacheableRangeEnd", 0);
    noncacheable_size_ = noncacheable_range_end_ - noncacheable_range_start_;

    max_reqs_per_issue_ = params.find<uint32_t>("reqsPerIssue", 1);
    if (max_reqs_per_issue_ < 1) {
        out_.fatal(CALL_INFO, -1, "%s, Error: StandardCPU cannot issue less than one request at a time...fix your input deck\n", getName().c_str());
    }

    // Tell the simulator not to end until we OK it
    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

    //set our clock
    std::string clockFreq = params.find<std::string>("clock", "1GHz");
    clock_handler_ = new Clock::Handler<standardCPU>(this, &standardCPU::clockTic);
    clock_timeconverter_ = registerClock( clockFreq, clock_handler_ );

    /* Find the interface the user provided in the Python and load it*/
    memory_ = loadUserSubComponent<StandardMem>("memory", ComponentInfo::SHARE_NONE, clock_timeconverter_, new StandardMem::Handler<standardCPU>(this, &standardCPU::handleEvent));

    if (!memory_) {
        out_.fatal(CALL_INFO, -1, "Unable to load memHierarchy.standardInterface subcomponent; check that 'memory' slot is filled in input.\n");
    }

    clock_ticks_ = 0;
    stat_requests_pending_per_cycle_ = registerStatistic<uint64_t>("pendCycle");
    stat_num_reads_issued_ = registerStatistic<uint64_t>("reads");
    stat_num_writes_issued_ = registerStatistic<uint64_t>("writes");
    if (noncacheable_size_ != 0) {
        stat_noncacheable_reads_ = registerStatistic<uint64_t>("readNoncache");
        stat_noncacheable_writes_ = registerStatistic<uint64_t>("writeNoncache");
    }
    if (flushf != 0 ) {
        stat_num_flushes_issued_ = registerStatistic<uint64_t>("flushes");
    }
    if (flushinvf != 0) {
        stat_num_flushinvs_issued_ = registerStatistic<uint64_t>("flushinvs");
    }
    if (flushcachef != 0) {
        stat_num_flushcache_issued_ = registerStatistic<uint64_t>("flushcaches");
    }
    if (customf != 0) {
        stat_num_custom_issued_ = registerStatistic<uint64_t>("customReqs");
    }

    if (llscf != 0) {
        stat_num_llsc_issued_ = registerStatistic<uint64_t>("llsc");
        stat_num_llsc_success_ = registerStatistic<uint64_t>("llsc_success");
    }
    ll_issued_ = false;

    init_write_count_ = params.find<uint64_t>("test_init", 0);
}

void standardCPU::init(unsigned int phase)
{
    memory_->init(phase);

    while (init_write_count_ != 0) {
        StandardMem::Addr addr = rng_.generateNextUInt64();
        memory_->sendUntimedData( createWrite(addr) );
        init_write_count_--;
    }
}

void standardCPU::setup() {
    memory_->setup();
    line_size_ = memory_->getLineSize();
}

void standardCPU::finish() { }

// incoming events are scanned and deleted
void standardCPU::handleEvent(StandardMem::Request *req)
{
    std::map<uint64_t, std::pair<SimTime_t,std::string>>::iterator i = requests_.find(req->getID());
    if ( requests_.end() == i ) {
        out_.output("%s, Error, Event (%" PRIu64 ") for request (%s) not found\n", getName().c_str(), req->getID(), req->getString().c_str());
        for (const auto& [key, value] : requests_)
            out_.output("\t%" PRIu64 " %s, %" PRIu64 "\n", key, value.second.c_str(), value.first);
        out_.fatal(CALL_INFO, -1, "%s, Error: Event (%" PRIu64 ") not found!\n", getName().c_str(), req->getID());
    } else {
        SimTime_t et = getCurrentSimTime() - i->second.first;
        if (i->second.second == "StoreConditional" && req->getSuccess())
            stat_num_llsc_success_->addData(1);
        requests_.erase(i);
    }

    delete req;
}


bool standardCPU::clockTic( Cycle_t )
{
    ++clock_ticks_;

    // Histogram bin the requests pending per cycle
    stat_requests_pending_per_cycle_->addData((uint64_t) requests_.size());

    // communicate?
    if (requests_.size() < max_outstanding_ && 0 != op_count_ ) {
        if (0 == (rng_comm_.generateNextUInt32() % mem_freq_)) {
            // yes, communicate
            // create event
            // x4 to prevent splitting blocks
            uint32_t reqsToSend = 1;
            if (max_reqs_per_issue_ > 1) reqsToSend += rng_comm_.generateNextUInt32() % max_reqs_per_issue_;
            if (reqsToSend > (max_outstanding_ - requests_.size())) reqsToSend = max_outstanding_ - requests_.size();
            if (reqsToSend > op_count_) reqsToSend = op_count_;

            for (int i = 0; i < reqsToSend; i++) {

                StandardMem::Addr addr = rng_.generateNextUInt64();
                
                uint32_t instNum = rng_.generateNextUInt32() % high_mark_;
                uint64_t size = 4;
                std::string cmdString = "Read";
                Interfaces::StandardMem::Request* req;
                
                if (ll_issued_) {
                    req = createSC();
                    cmdString = "StoreConditional";
                } else if (instNum < write_mark_) {
                    req = createWrite(addr);
                    cmdString = "Write";
                } else if (instNum < flush_mark_) {
                    req = createFlush(addr);
                    cmdString = "Flush";
                } else if (instNum < flushinv_mark_) {
                    req = createFlushInv(addr);
                    cmdString = "FlushInv";
                } else if (instNum < flushcache_mark_) {
                    req = createFlushCache();
                    cmdString = "FlushCache";
                } else if (instNum < custom_mark_) {
                } else if (instNum < llsc_mark_) {
                    req = createLL(addr);
                    cmdString = "LoadLink";
                } else if (instNum < mmio_mark_) {
                    bool opType = rng_.generateNextUInt32() % 2;
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
		            requests_[req->getID()] =  std::make_pair(getCurrentSimTime(), cmdString);
                }
            
                memory_->send(req);

                op_count_--;
	        }
        }
    }

    // Check whether to end the simulation
    if ( 0 == op_count_ && requests_.empty() ) {
        out_.verbose(CALL_INFO, 1, 0, "StandardCPU: Test Completed Successfuly\n");
        primaryComponentOKToEndSim();
        return true;    // Turn our clock off while we wait for any other CPUs to end
    }

    // return false so we keep going
    return false;
}

/* Methods for sending different kinds of requests */
StandardMem::Request* standardCPU::createWrite(Addr addr) {
    addr = ((addr % max_addr_)>>2) << 2;
    // Dummy payload
    std::vector<uint8_t> data;
    data.resize(4);
    data[0] = (addr >> 24) & 0xff;
    data[1] = (addr >> 16) & 0xff;
    data[2] = (addr >>  8) & 0xff;
    data[3] = (addr >>  0) & 0xff;

    StandardMem::Request* req = new Interfaces::StandardMem::Write(addr, data.size(), data);
    stat_num_writes_issued_->addData(1);
    if (addr >= noncacheable_range_start_ && addr < noncacheable_range_end_) {
        req->setNoncacheable();
        stat_noncacheable_writes_->addData(1);
    }
    out_.verbose(CALL_INFO, 2, 0, "%s: %" PRIu64 " Issued %sWrite for address 0x%" PRIx64 "\n", getName().c_str(), op_count_, req->getNoncacheable() ? "Noncacheable " : "", addr);
    return req;
}

StandardMem::Request* standardCPU::createRead(Addr addr) {
    addr = ((addr % max_addr_)>>2) << 2;
    StandardMem::Request* req = new Interfaces::StandardMem::Read(addr, 4);
    stat_num_reads_issued_->addData(1);
    if (addr >= noncacheable_range_start_ && addr < noncacheable_range_end_) {
        req->setNoncacheable();
        stat_noncacheable_reads_->addData(1);
    }
    out_.verbose(CALL_INFO, 2, 0, "%s: %" PRIu64 " Issued %sRead for address 0x%" PRIx64 "\n", getName().c_str(), op_count_, req->getNoncacheable() ? "Noncacheable " : "", addr);
    return req;
}

StandardMem::Request* standardCPU::createFlush(Addr addr) {
    addr = ((addr % (max_addr_ - noncacheable_size_)>>2) << 2);
    if (addr >= noncacheable_range_start_ && addr < noncacheable_range_end_)
        addr += noncacheable_range_end_;
    addr = addr - (addr % line_size_);
    StandardMem::Request* req = new Interfaces::StandardMem::FlushAddr(addr, line_size_, false, 10);
    stat_num_flushes_issued_->addData(1);
    out_.verbose(CALL_INFO, 2, 0, "%s: %" PRIu64 " Issued FlushAddr for address 0x%" PRIx64 "\n", getName().c_str(), op_count_,  addr);
    return req;
}

StandardMem::Request* standardCPU::createFlushInv(Addr addr) {
    addr = ((addr % (max_addr_ - noncacheable_size_)>>2) << 2);
    if (addr >= noncacheable_range_start_ && addr < noncacheable_range_end_)
        addr += noncacheable_range_end_;
    addr = addr - (addr % line_size_);
    StandardMem::Request* req = new Interfaces::StandardMem::FlushAddr(addr, line_size_, true, 10);
    stat_num_flushinvs_issued_->addData(1);
    out_.verbose(CALL_INFO, 2, 0, "%s: %" PRIu64 " Issued FlushAddrInv for address 0x%" PRIx64 "\n", getName().c_str(), op_count_,  addr);
    return req;
}

StandardMem::Request* standardCPU::createFlushCache() {
    StandardMem::Request* req = new Interfaces::StandardMem::FlushCache();
    stat_num_flushcache_issued_->addData(1);
    out_.verbose(CALL_INFO, 2, 0, "%s: %" PRIu64 " Issued FlushCache\n", getName().c_str(), op_count_);
    return req;
}

StandardMem::Request* standardCPU::createLL(Addr addr) {
    // Addr needs to be a cacheable range
    Addr cacheableSize = max_addr_ + 1 - noncacheable_range_end_ + noncacheable_range_start_;
    addr = (addr % (cacheableSize >> 2)) << 2;
    if (addr >= noncacheable_range_start_ && addr < noncacheable_range_end_) {
        addr += noncacheable_range_end_;
    }
    // Align addr
    addr = (addr >> 2) << 2;

    StandardMem::Request* req = new Interfaces::StandardMem::LoadLink(addr, 4);
    // Set these so we issue a matching sc 
    ll_addr_ = addr;
    ll_issued_ = true;

    out_.verbose(CALL_INFO, 2, 0, "%s: %" PRIu64 " Issued LoadLink for address 0x%" PRIx64 "\n", getName().c_str(), op_count_, addr);
    return req;
}

StandardMem::Request* standardCPU::createSC() {
    std::vector<uint8_t> data;
    data.resize(4);
    data[0] = (ll_addr_ >> 24) & 0xff;
    data[1] = (ll_addr_ >> 16) & 0xff;
    data[2] = (ll_addr_ >>  8) & 0xff;
    data[3] = (ll_addr_ >>  0) & 0xff;
    StandardMem::Request* req = new Interfaces::StandardMem::StoreConditional(ll_addr_, data.size(), data);
    stat_num_llsc_issued_->addData(1);
    ll_issued_ = false;
    out_.verbose(CALL_INFO, 2, 0, "%s: %" PRIu64 " Issued StoreConditional for address 0x%" PRIx64 "\n", getName().c_str(), op_count_, ll_addr_);
    return req;
}

StandardMem::Request* standardCPU::createMMIOWrite() {
    bool posted = rng_.generateNextUInt32() % 2;
    int32_t payload = rng_.generateNextInt32();
    payload >>= 16; // Shrink the number a bit
    int32_t payload_cp = payload;
    std::vector<uint8_t> data;
    for (int i = 0; i < sizeof(int32_t); i++) {
        data.push_back(payload & 0xFF);
        payload >>=8;
    }
    StandardMem::Request* req = new Interfaces::StandardMem::Write(mmio_addr_, sizeof(int32_t), data, posted);
    out_.verbose(CALL_INFO, 2, 0, "%s: %" PRIu64 " Issued MMIO Write for address 0x%" PRIx64 " with payload %d\n", getName().c_str(), op_count_, mmio_addr_, payload_cp);
    return req;
}

StandardMem::Request* standardCPU::createMMIORead() {
    StandardMem::Request* req = new Interfaces::StandardMem::Read(mmio_addr_, sizeof(int32_t));
    out_.verbose(CALL_INFO, 2, 0, "%s: %" PRIu64 " Issued MMIO Read for address 0x%" PRIx64 "\n", getName().c_str(), op_count_, mmio_addr_);
    return req;
}

void standardCPU::emergencyShutdown() {
    if (out_.getVerboseLevel() > 1) {
        if (out_.getOutputLocation() == Output::STDOUT)
            out_.setOutputLocation(Output::STDERR);
        
        out_.output("MemHierarchy::standardCPU %s\n", getName().c_str());
        out_.output("  Outstanding events: %zu\n", requests_.size());
        out_.output("End MemHierarchy::standardCPU %s\n", getName().c_str());
    }
}
