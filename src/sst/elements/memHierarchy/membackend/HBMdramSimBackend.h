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

// -------------------------------------------------
// Originally derived from the DRAMSim membackend
// Modified to provide HBM-specific functionality
// using the DRAMSim port from:
// https://github.com/tactcomplabs/HBM
// -------------------------------------------------


#ifndef _H_SST_MEMH_HBM_DRAMSIM_BACKEND
#define _H_SST_MEMH_HBM_DRAMSIM_BACKEND

#include "sst/elements/memHierarchy/membackend/memBackend.h"

#ifdef DEBUG
#define OLD_DEBUG DEBUG
#undef DEBUG
#endif

#include <HBMDRAMSim.h>

#ifdef OLD_DEBUG
#define DEBUG OLD_DEBUG
#undef OLD_DEBUG
#endif

namespace SST {
namespace MemHierarchy {

class HBMDRAMSimMemory : public SimpleMemBackend {
public:
/* Element Library Info */
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(HBMDRAMSimMemory, "memHierarchy", "HBMDRAMSimMemory", SST_ELI_ELEMENT_VERSION(1,0,0), "HBM DRAMSim-driven memory timings", SST::MemHierarchy::SimpleMemBackend)

#define HBMDRAMSIMMEMORY_ELI_PARAMS MEMBACKEND_ELI_PARAMS,\
            /* Own parameters */\
            {"verbose",     "Sets the verbosity of the backend output", "0" },\
            {"device_ini",  "Name of the DRAMSim Device config file",   NULL },\
            {"system_ini",  "Name of the DRAMSim Device system file",   NULL }

    SST_ELI_DOCUMENT_PARAMS( HBMDRAMSIMMEMORY_ELI_PARAMS )

#define HBMDRAMSIMMEMORY_ELI_STATS {"TotalBandwidth",      "Total Bandwidth",              "GB/s",     1},\
            {"BytesTransferred",    "Total Bytes Transferred",      "bytes",    1},\
            {"TotalReads",          "Total Queued Reads",           "count",    1},\
            {"TotalWrites",         "Total Queued Writes",          "count",    1},\
            {"TotalTransactions",   "Total Number of Transactions", "count",    1},\
            {"PendingReads",        "Pending Transactions",         "count",    1},\
            {"PendingReturns",      "Pending Returns",              "count",    1}

    SST_ELI_DOCUMENT_STATISTICS( HBMDRAMSIMMEMORY_ELI_STATS )

/* Class definition */
private:
  Statistic<double>* TBandwidth;
  Statistic<uint64_t>* BytesTransferred;
  Statistic<uint64_t>* TotalReads;
  Statistic<uint64_t>* TotalWrites;
  Statistic<uint64_t>* TotalXactions;
  Statistic<uint64_t>* PendingReads;
  Statistic<uint64_t>* PendingRtns;

  void registerStatistics();

public:
    HBMDRAMSimMemory(ComponentId_t id, Params &params);
    virtual bool issueRequest(ReqId, Addr, bool, unsigned );
    virtual bool clock(Cycle_t cycle);
    virtual void finish();

protected:
    void dramSimDone(unsigned int id, uint64_t addr, uint64_t clockcycle);

    HBMDRAMSim::MultiChannelMemorySystem *memSystem;
    std::map<uint64_t, std::deque<ReqId> > dramReqs;

private:
};

}
}

#endif
