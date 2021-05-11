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
#include "testcpu/standardCPU.h"

#include <sst/core/params.h>
#include <sst/core/simulation.h>
#include <sst/core/interfaces/stringEvent.h>
#include <sst/core/interfaces/stdMem.h>

using namespace SST;
using namespace SST::Experimental::Interfaces;
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
        out.fatal(CALL_INFO, -1, "%s, Error: parameter 'memSize' was not provided\n");
    }
    if (!(memsize.hasUnits("B"))) {
        out.fatal(CALL_INFO, -1, "%s, Error: memSize parameter requires units of 'B' (SI OK). You provided '%s'\n",
            getName().c_str(), memsize.toString().c_str() );
    }

    maxAddr = memsize.getRoundedValue() - 1;

    maxOutstanding = params.find<uint64_t>("maxOutstanding", 10);

    /* Required parameter - opCount */
    ops = params.find<uint64_t>("opCount", 0, found);
    if (!found) {
        out.fatal(CALL_INFO, -1, "%s, Error: parameter 'opCount' was not provided\n", 
                getName().c_str());
    }

    /* Frequency of different ops */
    unsigned readf = params.find<unsigned>("read_freq", 25);
    unsigned writef = params.find<unsigned>("write_freq", 75);
    unsigned flushf = params.find<unsigned>("flush_freq", 0);
    unsigned flushinvf = params.find<unsigned>("flushinv_freq", 0);
    unsigned customf = params.find<unsigned>("custom_freq", 0);
    unsigned llscf = params.find<unsigned>("llsc_freq", 0);

    high_mark = readf + writef + flushf + flushinvf + customf + llscf; /* Numbers less than this and above other marks indicate read */
    if (high_mark == 0) {
        out.fatal(CALL_INFO, -1, "%s, Error: The input doesn't indicate a frequency for any command type.\n", getName().c_str());
    }
    write_mark = writef;    /* Numbers less than this indicate write */
    flush_mark = write_mark + flushf; /* Numbers less than this indicate flush */
    flushinv_mark = flush_mark + flushinvf; /* Numbers less than this indicate flush-inv */
    custom_mark = flushinv_mark + customf; /* Numbers less than this indicate flush */
    llsc_mark = custom_mark + llscf;

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
                Experimental::Interfaces::StandardMem::Request* req;
                
                if (ll_issued) {
                    req = new Experimental::Interfaces::StandardMem::StoreConditional(ll_addr, size, data);
                    num_llsc_issued->addData(1);
                    ll_issued = false;
                    cmdString = "StoreConditional";

                // Write
                }else if (instNum < write_mark) {
                    cmdString = "Write";
                    addr = ((addr % maxAddr)>>2) << 2;
                    req = new Experimental::Interfaces::StandardMem::Write(addr, size, data);
                    num_writes_issued->addData(1);
                // Flush
                } else if (instNum < flush_mark) {
                    cmdString = "FlushLine";
                    size = lineSize;
                    addr = ((addr % (maxAddr - noncacheableSize)>>2) << 2);
                    if (addr >= noncacheableRangeStart && addr < noncacheableRangeEnd)
                        addr += noncacheableRangeEnd;
                    addr = addr - (addr % lineSize);
                    req = new Experimental::Interfaces::StandardMem::FlushAddr(addr, size, false, 10);
                    num_flushes_issued->addData(1);
                } else if (instNum < flushinv_mark) {
                    cmdString = "FlushLineInv";
                    size = lineSize;
                    addr = ((addr % (maxAddr - noncacheableRangeEnd)>>2) << 2) + noncacheableRangeEnd;
                    addr = ((addr % (maxAddr - noncacheableSize)>>2) << 2);
                    if (addr >= noncacheableRangeStart && addr < noncacheableRangeEnd)
                        addr += noncacheableRangeEnd;
                    addr = addr - (addr % lineSize);
                    req = new Experimental::Interfaces::StandardMem::FlushAddr(addr, size, true, 10);
                    num_flushinvs_issued->addData(1);
                // Read
                } else if (instNum < custom_mark) {
                } else if (instNum < llsc_mark) {
                    addr = ((addr % (maxAddr - noncacheableRangeEnd)>>2) << 2) + noncacheableRangeEnd;
                    addr = ((addr % (maxAddr - noncacheableSize)>>2) << 2);
                    if (addr >= noncacheableRangeStart && addr < noncacheableRangeEnd)
                        addr += noncacheableRangeEnd;
                    cmdString = "LoadLink";
                    req = new Experimental::Interfaces::StandardMem::LoadLink(addr, size);
                    ll_addr = addr;
                    ll_issued = true;
                } else {
                    addr = ((addr % maxAddr)>>2) << 2;
                    req = new Experimental::Interfaces::StandardMem::Read(addr, size);
                    num_reads_issued->addData(1);
                }

                bool noncacheable = ( addr >= noncacheableRangeStart && addr < noncacheableRangeEnd );
                if ( noncacheable ) {
                    req->setNoncacheable();
                    if ( cmdString == "Write" ) { noncacheableWrites->addData(1); }
                    else if (cmdString == "Read" ) { noncacheableReads->addData(1); }
                }

		requests[req->getID()] =  std::make_pair(getCurrentSimTime(), cmdString);
		memory->send(req);

		out.verbose(CALL_INFO, 2, 0, "%s: %d Issued %s%s for address 0x%" PRIx64 "\n",
                            getName().c_str(), ops, noncacheable ? "Noncacheable " : "" , cmdString.c_str(), addr);

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


