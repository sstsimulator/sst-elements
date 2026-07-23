// Copyright 2009-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2026, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef BALAR_DOORBELL_TEST_CPU_H
#define BALAR_DOORBELL_TEST_CPU_H

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>

#include <sst/core/interfaces/stdMem.h>
#include <sst/core/component.h>
#include <sst/core/output.h>
#include "balar_packet.h"

#include "builtin_types.h"
#include "cuda.h"

#include <fstream>
#include <map>
#include <queue>
#include <string>
#include <vector>

namespace SST {
namespace BalarComponent {

class DoorbellTestCPU : public SST::Component {
public:
    SST_ELI_REGISTER_COMPONENT(
        DoorbellTestCPU,
        "balar",
        "DoorbellTestCPU",
        SST_ELI_ELEMENT_VERSION(1, 0, 0),
        "Doorbell-style test CPU: cache_link for scratch+flush, mmio_link for GPU doorbell",
        COMPONENT_CATEGORY_PROCESSOR)

    SST_ELI_DOCUMENT_PARAMS(
        { "verbose", "(uint) Verbosity", "1" },
        { "clock", "(UnitAlgebra/string) Clock frequency", "1GHz" },
        { "mmio_addr", "(uint) BalarMMIO base address (doorbell)", "0" },
        { "scratch_mem_addr", "(uint) Scratch region for CUDA call packets", "0" },
        { "cache_line_size", "(uint) Cache line size for scratch writes and flush", "64" },
        { "mode", "(string) trace or doorbell_only", "trace" },
        { "trace_file", "(string) CUDA API trace file (trace mode)", "cuda_calls.trace" },
        { "cuda_executable", "(string) CUDA binary for fatbin registration", "" },
        { "enable_memcpy_dump", "(bool) Dump D2H memcpy verification files", "false" })

    SST_ELI_DOCUMENT_STATISTICS(
        { "cache_writes", "Scratch packet writes via cache_link", "count", 1 },
        { "mmio_writes", "Doorbell and MMIO writes via mmio_link", "count", 1 },
        { "flush_count", "FlushAddr(inv) issued before doorbell", "count", 1 },
        { "cuda_calls_completed", "Completed CUDA API round-trips", "count", 1 },
        { "total_memD2H_bytes", "Total D2H memcpy bytes checked", "count", 1 },
        { "correct_memD2H_bytes", "Correct D2H memcpy bytes", "count", 1 },
        { "correct_memD2H_ratio", "D2H byte correctness ratio", "count", 1 })

    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
        { "cache_link", "Cache path for scratch writes and return packet reads", "SST::Interfaces::StandardMem" },
        { "mmio_link", "MMIO path for doorbell and status reads", "SST::Interfaces::StandardMem" })

    DoorbellTestCPU(SST::ComponentId_t id, SST::Params& params);
    ~DoorbellTestCPU() override;
    void init(unsigned int phase) override;
    void setup() override;
    void finish() override;
    void emergencyShutdown() override;

private:
    enum class DriverMode { TRACE, DOORBELL_ONLY };

    enum class IfacePath { CACHE, MMIO };

    void handleCacheEvent(Interfaces::StandardMem::Request* req);
    void handleMmioEvent(Interfaces::StandardMem::Request* req);
    bool clockTic(SST::Cycle_t cycle);

    void beginPacketIssue(const BalarCudaCallPacket_t& pack);
    void sendNextScratchChunk();
    void sendNextFlush();
    void sendDoorbell();
    void sendStartCudaRetRead();
    void sendReadRetPacket(uint64_t ret_addr);

    void onCacheWriteResp(Interfaces::StandardMem::WriteResp* resp);
    void onCacheWriteReq(Interfaces::StandardMem::Write* write);
    void onCacheFlushResp(Interfaces::StandardMem::FlushResp* resp);
    void onCacheReadResp(Interfaces::StandardMem::ReadResp* resp);
    void onMmioWriteResp(Interfaces::StandardMem::WriteResp* resp);
    void onMmioWriteReq(Interfaces::StandardMem::Write* write);
    void onMmioReadResp(Interfaces::StandardMem::ReadResp* resp);

    void completeCudaCall(const BalarCudaCallReturnPacket_t* ret_pack);
    void tryFinishSimulation();

    static void uint64ToData(uint64_t num, std::vector<uint8_t>* data);
    static uint64_t dataToUInt64(std::vector<uint8_t>* data);

    Output out;
    DriverMode mode_;
    uint64_t scratchMemAddr_;
    uint64_t mmioAddr_;
    uint64_t cacheLineSize_;
    bool enable_memcpy_dump_;
    std::string cuda_executable_;
    // Gates the one-shot initial fatbin issue in DOORBELL_ONLY mode so the
    // trigger doesn't depend on a magic clock-tick number.
    bool issued_initial_packet_;
    // NOTE: redundant with stat_cuda_calls_completed_->getCollectionCount();
    // kept as a plain counter to avoid coupling control flow to the stats subsystem.
    uint64_t calls_completed_;

    Interfaces::StandardMem* cache_link_;
    Interfaces::StandardMem* mmio_link_;

    TimeConverter clockTC_;
    Clock::HandlerBase* clock_handler_;

    std::map<Interfaces::StandardMem::Request::id_t, std::pair<std::string, IfacePath>> requests_;

    std::vector<uint8_t> packet_buffer_;
    size_t scratch_bytes_remaining_;
    size_t scratch_write_offset_;
    size_t flushes_remaining_;
    size_t flushes_outstanding_;
    bool packet_issue_active_;
    std::vector<uint8_t> scratch_append_payload_;

    class CudaAPITraceParser;
    CudaAPITraceParser* trace_parser_;

    class CacheHandlers;
    class MmioHandlers;
    CacheHandlers* cache_handlers_;
    MmioHandlers* mmio_handlers_;

    Statistic<uint64_t>* stat_cache_writes_;
    Statistic<uint64_t>* stat_mmio_writes_;
    Statistic<uint64_t>* stat_flush_count_;
    Statistic<uint64_t>* stat_cuda_calls_completed_;
    Statistic<uint64_t>* stat_total_memD2H_bytes_;
    Statistic<uint64_t>* stat_correct_memD2H_bytes_;
    Statistic<double>* stat_correct_memD2H_ratio_;
};

} // namespace BalarComponent
} // namespace SST

#endif // BALAR_DOORBELL_TEST_CPU_H
