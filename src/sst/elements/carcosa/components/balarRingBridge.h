// Copyright 2009-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2026, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory of the distribution.
//
// This file is part of the SST software package. For license information,
// see the LICENSE file in the top level directory of the distribution.

#ifndef CARCOSA_BALAR_RING_BRIDGE_H
#define CARCOSA_BALAR_RING_BRIDGE_H

// Built only under SST_CARCOSA_HAVE_BALAR / HAVE_BALAR_BRIDGE (needs balar CUDA
// headers). Body is fully guarded so libcarcosa still builds with no balar —
// the bridge is simply absent.
#ifdef HAVE_BALAR_BRIDGE

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>

#include <sst/core/output.h>
#include <sst/core/link.h>
#include <sst/core/interfaces/stdMem.h>
#include <sst/elements/carcosa/components/interceptionAgentAPI.h>
#include <sst/elements/carcosa/components/haliEvent.h>
#include <sst/elements/carcosa/components/ringProtocol.h>
#include <sst/elements/carcosa/components/carcosaHash.h>
#include <sst/elements/memHierarchy/memEvent.h>

// balar CUDA-call packet ABI (pulls in builtin_types.h / driver_types.h).
#include <sst/elements/balar/balar_packet.h>
// CUDA driver API types (CUdeviceptr) used by the trace parser's device-pointer
// map -- balar's forked test CPU (balar/testcpu/) includes these two explicitly for the same
// reason; balar_packet.h alone does not pull in cuda.h.
#include "builtin_types.h"
#include "cuda.h"

#include <map>
#include <string>
#include <utility>
#include <vector>

namespace SST {
namespace Carcosa {

/**
 * Ring GPU substrate on balar; dedicated H2D weight stage for EccGuard confinement.
 */
class BalarRingBridge : public InterceptionAgentAPI
{
public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        BalarRingBridge,
        "carcosa",
        "BalarRingBridge",
        SST_ELI_ELEMENT_VERSION(1, 0, 0),
        "Hali-ring GPU bridge: replays a staged GEMM CUDA-call sequence onto a real "
        "balar/GPGPU-Sim device, folds the D2H result into an action checksum, and "
        "replies Done over the ring (execution-driven ECC confinement on the real substrate)",
        SST::Carcosa::InterceptionAgentAPI
    )

    SST_ELI_DOCUMENT_PARAMS(
        {"verbose",          "Enable verbose output.", "false"},
        {"clock",            "Clock for the StandardMem interfaces into balar.", "1GHz"},
        {"state_key",        "PipelineStateRegistry key shared with the CPU driver / ActionScorer.", "cpu0_vla"},
        {"mmio_addr",        "BalarMMIO base address (doorbell).", "0"},
        {"scratch_mem_addr", "Scratch region base for CUDA-call packets (control; NOT injected).", "0"},
        {"weight_stage_addr","Dedicated H2D weight-payload staging region (the injected weight buffer).", "0x20000000"},
        {"cache_line_size",  "Cache line size for scratch/weight writes and flush.", "64"},
        {"trace_file",       "GEMM CUDA-API trace replayed per ring Cmd.", "cuda_calls.trace"},
        {"cuda_executable",  "CUDA binary for fatbin registration.", ""},
        {"replay_each_cmd",  "If true, replay the full trace on every ring Cmd; if false, replay once then Done-only.", "false"}
    )

    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
        {"cache_link", "StandardMem for scratch/weight staging, flush, and return-packet reads (through EccGuard).",
         "SST::Interfaces::StandardMem"},
        {"mmio_link",  "StandardMem for the BalarMMIO doorbell and status reads.",
         "SST::Interfaces::StandardMem"}
    )

    BalarRingBridge(ComponentId_t id, Params& params);
    BalarRingBridge() : InterceptionAgentAPI() {}
    // Out-of-line so the (forward-declared) CudaAPITraceParser / handler types are
    // complete at the delete site.
    ~BalarRingBridge() override;

    // Unused data-plane hook (this agent speaks the ring + its own StandardMem links).
    bool handleInterceptedEvent(SST::MemHierarchy::MemEvent* ev,
                                SST::Link* highlink) override {
        (void)ev; (void)highlink; return false;
    }

    void setRingLink(SST::Link* leftLink) override { ringLink_ = leftLink; }
    void handleRingEvent(SST::Carcosa::HaliEvent* ev) override;

    void agentInit(unsigned phase) override;
    void agentSetup() override;

private:
    enum class IfacePath { CACHE, MMIO };

    // StandardMem response demux.
    void handleCacheEvent(SST::Interfaces::StandardMem::Request* req);
    void handleMmioEvent(SST::Interfaces::StandardMem::Request* req);

    // Packet-issue state machine (mirrors balar's forked test CPU), driven by the ring.
    void beginTrace();                 // kick the first CUDA-call packet of a replay
    void issueNextPacket();            // pull the next packet from the trace, or finish
    void beginPacketIssue(const SST::BalarComponent::BalarCudaCallPacket_t& pack);
    void sendNextStageChunk();         // walk the staging segments (packet + weights)
    void sendNextFlush();
    void sendDoorbell();
    void sendStartCudaRetRead();
    void sendReadRetPacket(uint64_t ret_addr);
    void sendNextD2HRead();

    void onCacheWriteResp(SST::Interfaces::StandardMem::WriteResp* resp);
    void onCacheFlushResp(SST::Interfaces::StandardMem::FlushResp* resp);
    void onCacheReadResp(SST::Interfaces::StandardMem::ReadResp* resp);
    void onMmioWriteResp(SST::Interfaces::StandardMem::WriteResp* resp);
    void onMmioReadResp(SST::Interfaces::StandardMem::ReadResp* resp);

    // Returns false when an SST-memory D2H readback must finish asynchronously.
    bool completeCudaCall(const SST::BalarComponent::BalarCudaCallReturnPacket_t* ret_pack);
    void finishCudaCall();
    void releasePendingD2H();
    void finishReplay();               // publish checksum + send Done, arm for next Cmd

    static void uint64ToData(uint64_t num, std::vector<uint8_t>* data);
    static uint64_t dataToUInt64(std::vector<uint8_t>* data);

    SST::Output*                  out_       = nullptr;
    SST::Link*                    ringLink_  = nullptr;
    SST::Interfaces::StandardMem* cache_link_ = nullptr;
    SST::Interfaces::StandardMem* mmio_link_  = nullptr;

    std::string stateKey_;
    uint64_t    mmioAddr_        = 0;
    uint64_t    scratchMemAddr_  = 0;
    uint64_t    weightStageAddr_ = 0x20000000;
    uint64_t    cacheLineSize_   = 64;
    std::string traceFile_;
    std::string cudaExecutable_;
    bool        replayEachCmd_   = false;
    bool        verbose_         = false;

    // Per-request bookkeeping: tag + which link it went out on.
    std::map<SST::Interfaces::StandardMem::Request::id_t,
             std::pair<std::string, IfacePath>> requests_;

    // Staging: one or more (addr, bytes) segments written before the doorbell.
    // Segment 0 is always the encoded command packet at scratchMemAddr_; for an
    // H2D memcpy a second segment holds the weight payload at weightStageAddr_.
    struct StageSegment { uint64_t addr; std::vector<uint8_t> bytes; };
    std::vector<StageSegment> stage_segments_;
    // Filled by the trace parser for an H2D memcpy: the weight payload to stage at
    // weightStageAddr_ (a dedicated, separable region) rather than intermingled
    // with the command packet. Consumed by beginPacketIssue.
    std::vector<uint8_t> pending_weight_payload_;
    size_t   seg_index_        = 0;    // which segment we're writing
    size_t   seg_offset_       = 0;    // byte offset within the current segment
    size_t   writes_outstanding_ = 0;  // chunk writes still in flight (across segments)
    size_t   flushes_remaining_ = 0;
    size_t   flush_line_index_  = 0;
    // Staged regions plus any D2H destination that must be invalidated pre-DMA.
    std::vector<std::pair<uint64_t, size_t>> flush_ranges_;  // (base, bytes)
    bool     packet_issue_active_ = false;

    // Replay lifecycle.
    bool     replay_active_ = false;   // a trace replay is in flight for the current Cmd
    bool     replayed_once_ = false;   // at least one full replay has completed
    uint64_t cmd_pending_   = 0;       // ring Cmds queued while a replay is active

    // Chained FNV-1a over D2H results (same construction as CriticalActionWatcher);
    // published to watcherActionChecksum. Reset to the offset basis each replay.
    uint64_t checksum_ = kFnv1a64OffsetBasis;
    uint64_t replays_  = 0;

    // State for the single in-flight D2H memcpy. Small, non-SST copies return a
    // directly populated host buffer. SST-memory copies instead require an
    // asynchronous StandardMem readback after balar's DMA completion.
    uint8_t* pending_d2h_host_buf_ = nullptr;
    uint64_t pending_d2h_sst_addr_ = 0;
    size_t   pending_d2h_bytes_ = 0;
    size_t   pending_d2h_read_bytes_ = 0;
    size_t   pending_d2h_read_offset_ = 0;
    size_t   pending_d2h_read_chunk_ = 0;
    bool     pending_d2h_is_sst_ = false;

    class CudaAPITraceParser;
    CudaAPITraceParser* trace_parser_ = nullptr;

    // Handler adapters for StandardMem's double-dispatch RequestHandler.
    class CacheHandlers;
    class MmioHandlers;
    CacheHandlers* cache_handlers_ = nullptr;
    MmioHandlers*  mmio_handlers_  = nullptr;
};

} // namespace Carcosa
} // namespace SST

#endif // HAVE_BALAR_BRIDGE
#endif // CARCOSA_BALAR_RING_BRIDGE_H
