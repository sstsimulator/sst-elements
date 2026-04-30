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

#ifndef CARCOSA_CARCOSACPUBASE_H
#define CARCOSA_CARCOSACPUBASE_H

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>

#include <sst/core/interfaces/stdMem.h>
#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include <sst/core/output.h>
#include <sst/core/rng/marsaglia.h>

#include "sst/elements/memHierarchy/util.h"

using namespace SST::Statistics;

namespace SST {
namespace MemHierarchy {
using Req = SST::Interfaces::StandardMem::Request;

/**
 * Shared base class for CarcosaCPU and FaultInjCPU.
 * Contains all common state and logic; not ELI-registered.
 */
class CarcosaCPUBase : public SST::Component {
public:
    CarcosaCPUBase(SST::ComponentId_t id, SST::Params& params);

    void handleCpuEvent(SST::Event *ev);
    void init(unsigned int phase) override;
    void setup() override;
    void finish() override;
    void emergencyShutdown() override;

protected:
    void handleEvent(Interfaces::StandardMem::Request *ev);
    virtual bool clockTic(SST::Cycle_t);

    SST::Link* HaliLink;
    Output out;
    uint64_t ops;
    uint64_t memFreq;
    uint64_t maxAddr;
    uint64_t mmioAddr;
    uint64_t lineSize;
    uint64_t maxOutstanding;
    unsigned high_mark;
    unsigned write_mark;
    unsigned flush_mark;
    unsigned flushinv_mark;
    unsigned custom_mark;
    unsigned llsc_mark;
    unsigned mmio_mark;
    uint32_t maxReqsPerIssue;
    uint64_t noncacheableRangeStart, noncacheableRangeEnd, noncacheableSize;
    uint64_t clock_ticks;
    Statistic<uint64_t>* requestsPendingCycle;
    Statistic<uint64_t>* num_reads_issued;
    Statistic<uint64_t>* num_writes_issued;
    Statistic<uint64_t>* num_flushes_issued;
    Statistic<uint64_t>* num_flushinvs_issued;
    Statistic<uint64_t>* num_custom_issued;
    Statistic<uint64_t>* num_llsc_issued;
    Statistic<uint64_t>* num_llsc_success;
    Statistic<uint64_t>* noncacheableReads;
    Statistic<uint64_t>* noncacheableWrites;

    bool ll_issued;
    Interfaces::StandardMem::Addr ll_addr;

    std::map<Interfaces::StandardMem::Request::id_t, std::pair<SimTime_t, std::string>> requests;

    Interfaces::StandardMem *memory;

    SST::RNG::MarsagliaRNG rng;

    TimeConverter clockTC;
    Clock::HandlerBase *clockHandler;

    Interfaces::StandardMem::Request* createWrite(uint64_t addr);
    Interfaces::StandardMem::Request* createRead(Addr addr);
    Interfaces::StandardMem::Request* createFlush(Addr addr);
    Interfaces::StandardMem::Request* createFlushInv(Addr addr);
    Interfaces::StandardMem::Request* createFlushCache();
    Interfaces::StandardMem::Request* createLL(Addr addr);
    Interfaces::StandardMem::Request* createSC();
    Interfaces::StandardMem::Request* createMMIOWrite();
    Interfaces::StandardMem::Request* createMMIORead();
};

} // namespace MemHierarchy
} // namespace SST

#endif // CARCOSA_CARCOSACPUBASE_H
