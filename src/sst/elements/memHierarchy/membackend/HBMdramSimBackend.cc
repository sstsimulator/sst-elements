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

// Originally derived from the DRAMSim membackend
// Modified to provide HBM-specific functionality
// using the DRAMSim port from:
// https://github.com/tactcomplabs/HBM

#include <sst_config.h>
#include "sst/elements/memHierarchy/util.h"
#include "membackend/HBMdramSimBackend.h"

using namespace SST;
using namespace SST::MemHierarchy;
using namespace HBMDRAMSim;

HBMDRAMSimMemory::HBMDRAMSimMemory(ComponentId_t id, Params &params) : SimpleMemBackend(id, params){ 
    std::string deviceIniFilename = params.find<std::string>("device_ini", NO_STRING_DEFINED);
    if(NO_STRING_DEFINED == deviceIniFilename)
        output->fatal(CALL_INFO, -1, "Model must define a 'device_ini' file parameter\n");
    std::string systemIniFilename = params.find<std::string>("system_ini", NO_STRING_DEFINED);
    if(NO_STRING_DEFINED == systemIniFilename)
        output->fatal(CALL_INFO, -1, "Model must define a 'system_ini' file parameter\n");

    if (params.find<bool>("tracing", false)) {
      registerStatistics();
    }


    UnitAlgebra ramSize = UnitAlgebra(params.find<std::string>("mem_size", "0B"));
    if (ramSize.getRoundedValue() % (1024*1024) != 0) {
        output->fatal(CALL_INFO, -1, "For HBMDRAMSim, backend.mem_size must be a multiple of 1MiB. Note: for units in base-10 use 'MB', for base-2 use 'MiB'. You specified '%s'\n", ramSize.toString().c_str());
    }
    unsigned int ramSizeMiB = ramSize.getRoundedValue() / (1024*1024);

    memSystem = HBMDRAMSim::getMemorySystemInstance(
            deviceIniFilename, systemIniFilename, "", ramSizeMiB);

    HBMDRAMSim::Callback<HBMDRAMSimMemory, void, unsigned int, uint64_t, uint64_t>
        *readDataCB, *writeDataCB;

    readDataCB = new HBMDRAMSim::Callback<HBMDRAMSimMemory, void, unsigned int, uint64_t, uint64_t>(
            this, &HBMDRAMSimMemory::dramSimDone);
    writeDataCB = new HBMDRAMSim::Callback<HBMDRAMSimMemory, void, unsigned int, uint64_t, uint64_t>(
            this, &HBMDRAMSimMemory::dramSimDone);

    memSystem->RegisterCallbacks(readDataCB, writeDataCB, NULL);
}


bool HBMDRAMSimMemory::issueRequest(ReqId id, Addr addr, bool isWrite, unsigned ){
    bool ok = memSystem->willAcceptTransaction(addr);
    if(!ok) return false;
    ok = memSystem->addTransaction(isWrite, addr);
    if(!ok) return false;  // This *SHOULD* always be ok
#ifdef __SST_DEBUG_OUTPUT__
    output->debug(_L10_, "Issued transaction for address %" PRIx64 "\n", (Addr)addr);
#endif
    dramReqs[addr].push_back(id);
    return true;
}

void HBMDRAMSimMemory::registerStatistics(){
  TBandwidth = registerStatistic<double>("TotalBandwidth");
  BytesTransferred = registerStatistic<uint64_t>("BytesTransferred");
  TotalReads = registerStatistic<uint64_t>("TotalReads");
  TotalWrites = registerStatistic<uint64_t>("TotalWrites");
  TotalXactions = registerStatistic<uint64_t>("TotalTransactions");
  PendingReads = registerStatistic<uint64_t>("PendingReads");
  PendingRtns = registerStatistic<uint64_t>("PendingReturns");
}

bool HBMDRAMSimMemory::clock(Cycle_t cycle){
    memSystem->update();

    // retrieve the statistics
    double tbandwidth = 0.;
    uint64_t bytes_transferred = 0x00ull;
    uint64_t total_reads = 0x00ull;
    uint64_t total_writes = 0x00ull;
    uint64_t total_xactions = 0x00ull;
    uint64_t pending_reads = 0x00ull;
    uint64_t pending_rtns = 0x00ull;

    if( !memSystem->getStats( &tbandwidth, TOTAL_BANDWIDTH ) ){
      TBandwidth->addData(tbandwidth);
    }

    if( !memSystem->getStats( &bytes_transferred, TOTAL_BYTES_TRANSFERRED ) ){
      BytesTransferred->addData(bytes_transferred);
    }

    if( !memSystem->getStats( &total_reads, TOTAL_READS ) ){
      TotalReads->addData(total_reads);
    }

    if( !memSystem->getStats( &total_writes, TOTAL_WRITES ) ){
      TotalWrites->addData(total_writes);
    }

    if( !memSystem->getStats( &total_xactions, TOTAL_TRANSACTIONS ) ){
      TotalXactions->addData(total_xactions);
    }

    if( !memSystem->getStats( &pending_reads, PENDING_READ_TRANSACTIONS ) ){
      PendingReads->addData(pending_reads);
    }

    if( !memSystem->getStats( &pending_rtns, PENDING_RTN_TRANSACTIONS ) ){
      PendingRtns->addData(pending_rtns);
    }
    return false;
}

void HBMDRAMSimMemory::finish(){
    memSystem->printStats(true);
}

void HBMDRAMSimMemory::dramSimDone(unsigned int id,
                                   uint64_t addr,
                                   uint64_t clockcycle){
    std::deque<ReqId> &reqs = dramReqs[addr];
#ifdef __SST_DEBUG_OUTPUT__
    output->debug(_L10_, "Memory Request for %" PRIx64 " Finished [%zu reqs]\n", (Addr)addr, reqs.size());
#endif
    if (reqs.size() == 0) output->fatal(CALL_INFO, -1, "Error: reqs.size() is 0 at HBMDRAMSimMemory done\n");
    ReqId reqId = reqs.front();
    reqs.pop_front();
    if(0 == reqs.size())
        dramReqs.erase(addr);

    handleMemResponse(reqId);
}
