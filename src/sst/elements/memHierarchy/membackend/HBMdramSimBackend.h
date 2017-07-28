// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2017, Sandia Corporation
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
// using the GATech DRAMSim port from:
// https://github.com/gthparch/HBM
// -------------------------------------------------


#ifndef _H_SST_MEMH_HBM_DRAMSIM_BACKEND
#define _H_SST_MEMH_HBM_DRAMSIM_BACKEND

#include "membackend/memBackend.h"

#ifdef DEBUG
#define OLD_DEBUG DEBUG
#undef DEBUG
#endif

#include <DRAMSim.h>

#ifdef OLD_DEBUG
#define DEBUG OLD_DEBUG
#undef OLD_DEBUG
#endif

namespace SST {
namespace MemHierarchy {

class HBMDRAMSimMemory : public SimpleMemBackend {
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
    HBMDRAMSimMemory(Component *comp, Params &params);
	virtual bool issueRequest(ReqId, Addr, bool, unsigned );
    virtual bool clock(Cycle_t cycle);
    virtual void finish();

protected:
    void dramSimDone(unsigned int id, uint64_t addr, uint64_t clockcycle);

    DRAMSim::MultiChannelMemorySystem *memSystem;
    std::map<uint64_t, std::deque<ReqId> > dramReqs;
};

}
}

#endif
