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

#include "sst_config.h"

#ifdef HAVE_BALAR_BRIDGE

#include "sst/elements/carcosa/components/balarRingBridge.h"
#include "sst/elements/carcosa/components/balarTraceParser.h"
#include "sst/elements/carcosa/components/pipelineStateRegistry.h"

// balar packet encode/decode helpers (templates).
#include <sst/elements/balar/util.h>

#include <algorithm>
#include <cinttypes>
#include <cstring>

using namespace SST;
using namespace SST::Interfaces;
using namespace SST::Carcosa;
using namespace SST::BalarComponent;

// ---------------------------------------------------------------------------
// StandardMem double-dispatch handler adapters.
// ---------------------------------------------------------------------------
class BalarRingBridge::CacheHandlers : public StandardMem::RequestHandler {
public:
    CacheHandlers(BalarRingBridge* b, SST::Output* out) : StandardMem::RequestHandler(out), b_(b) {}
    ~CacheHandlers() override {}
    void handle(StandardMem::ReadResp* resp) override  { b_->onCacheReadResp(resp); }
    void handle(StandardMem::WriteResp* resp) override { b_->onCacheWriteResp(resp); }
    void handle(StandardMem::FlushResp* resp) override { b_->onCacheFlushResp(resp); }
private:
    BalarRingBridge* b_;
};

class BalarRingBridge::MmioHandlers : public StandardMem::RequestHandler {
public:
    MmioHandlers(BalarRingBridge* b, SST::Output* out) : StandardMem::RequestHandler(out), b_(b) {}
    ~MmioHandlers() override {}
    void handle(StandardMem::ReadResp* resp) override  { b_->onMmioReadResp(resp); }
    void handle(StandardMem::WriteResp* resp) override { b_->onMmioWriteResp(resp); }
private:
    BalarRingBridge* b_;
};

BalarRingBridge::BalarRingBridge(ComponentId_t id, Params& params)
    : InterceptionAgentAPI(id, params)
{
    out_             = new Output("BalarRingBridge[@p:@l] ", 1, 0, Output::STDOUT);
    verbose_         = params.find<bool>("verbose", false);
    state_key_         = params.find<std::string>("state_key", "cpu0_vla");
    mmio_addr_         = params.find<uint64_t>("mmio_addr", 0);
    scratch_mem_addr_  = params.find<uint64_t>("scratch_mem_addr", 0);
    weight_stage_addr_ = params.find<uint64_t>("weight_stage_addr", 0x20000000);
    cache_line_size_   = params.find<uint64_t>("cache_line_size", 64);
    trace_file_        = params.find<std::string>("trace_file", "cuda_calls.trace");
    replay_each_cmd_   = params.find<bool>("replay_each_cmd", false);

    bool found = false;
    cuda_executable_ = params.find<std::string>("cuda_executable", found);
    if (!found)
        out_->fatal(CALL_INFO, -1, "BalarRingBridge: 'cuda_executable' is required (fatbin registration)\n");
    if (cache_line_size_ == 0)
        out_->fatal(CALL_INFO, -1, "BalarRingBridge: cache_line_size must be > 0\n");

    TimeConverter tc = getTimeConverter(params.find<std::string>("clock", "1GHz"));
    cache_link_ = loadUserSubComponent<StandardMem>(
        "cache_link", ComponentInfo::SHARE_NONE, tc,
        new StandardMem::Handler<BalarRingBridge, &BalarRingBridge::handleCacheEvent>(this));
    mmio_link_ = loadUserSubComponent<StandardMem>(
        "mmio_link", ComponentInfo::SHARE_NONE, tc,
        new StandardMem::Handler<BalarRingBridge, &BalarRingBridge::handleMmioEvent>(this));
    if (!cache_link_ || !mmio_link_)
        out_->fatal(CALL_INFO, -1,
            "BalarRingBridge: requires 'cache_link' and 'mmio_link' StandardMem slots into balar\n");

    cache_handlers_ = new CacheHandlers(this, out_);
    mmio_handlers_  = new MmioHandlers(this, out_);
}

BalarRingBridge::~BalarRingBridge()
{
    releasePendingD2H();
    delete trace_parser_;
    delete cache_handlers_;
    delete mmio_handlers_;
    delete out_;
}

void BalarRingBridge::agentInit(unsigned phase)
{
    if (cache_link_) cache_link_->init(phase);
    if (mmio_link_)  mmio_link_->init(phase);
}

void BalarRingBridge::agentSetup()
{
    if (cache_link_) cache_link_->setup();
    if (mmio_link_)  mmio_link_->setup();
    trace_parser_ = new BalarTraceParser(out_, trace_file_, cuda_executable_);
    if (verbose_)
        out_->output("BalarRingBridge: setup, scratch=0x%" PRIx64 " weights=0x%" PRIx64
                     " mmio=0x%" PRIx64 " trace=%s\n",
                     scratch_mem_addr_, weight_stage_addr_, mmio_addr_, trace_file_.c_str());
}

void BalarRingBridge::handleCacheEvent(StandardMem::Request* req) { req->handle(cache_handlers_); }
void BalarRingBridge::handleMmioEvent(StandardMem::Request* req)  { req->handle(mmio_handlers_); }

void BalarRingBridge::uint64ToData(uint64_t num, std::vector<uint8_t>* data)
{
    data->clear();
    for (size_t i = 0; i < sizeof(uint64_t); i++) { data->push_back(num & 0xFF); num >>= 8; }
}

uint64_t BalarRingBridge::dataToUInt64(std::vector<uint8_t>* data)
{
    uint64_t retval = 0;
    for (int i = (int)data->size() - 1; i >= 0; i--) { retval <<= 8; retval |= (*data)[i]; }
    return retval;
}

// ---------------------------------------------------------------------------
// Ring trigger
// ---------------------------------------------------------------------------
void BalarRingBridge::handleRingEvent(HaliEvent* ev)
{
    // The driver dispatches SeqLen then Cmd per GPU-resident kernel. On Cmd we run
    // the staged GEMM replay onto balar and reply Done only when the D2H result has
    // been read. SeqLen/Exit and other ring traffic are ignored.
    if (!ev || ev->getStr() != RingTag::Cmd) return;

    if (verbose_) out_->output("BalarRingBridge: ring Cmd %u -> GEMM replay\n", ev->getNum());

    // If we already replayed once and are not replaying per-Cmd, just release the
    // barrier: the resident-weight result is stable, so re-launching is redundant.
    if (replayed_once_ && !replay_each_cmd_) {
        if (ring_link_) ring_link_->send(new HaliEvent(RingTag::Done, 0u));
        return;
    }
    if (replay_active_) { ++cmd_pending_; return; }  // serialize; drain on finish

    beginTrace();
}

void BalarRingBridge::beginTrace()
{
    replay_active_ = true;
    checksum_      = kFnv1a64OffsetBasis;  // fresh per-replay fold
    if (!trace_parser_) { finishReplay(); return; }
    trace_parser_->rewind();
    issueNextPacket();
}

void BalarRingBridge::issueNextPacket()
{
    BalarTracePacket trace_packet;
    if (trace_parser_ && trace_parser_->next(trace_packet)) {
        BalarCudaCallPacket_t& pack = trace_packet.packet;
        if (trace_packet.is_h2d) {
            pack.isSSTmem = true;
            pack.cuda_memcpy.src = weight_stage_addr_;
            pending_weight_payload_ = std::move(trace_packet.h2d_payload);
        }
        if (trace_packet.is_d2h) {
            releasePendingD2H();
            pending_d2h_bytes_ = trace_packet.d2h_bytes;
            if (trace_packet.d2h_bytes >= cache_line_size_) {
                pack.isSSTmem = true;
                pack.cuda_memcpy.dst = scratch_mem_addr_ + sizeof(BalarCudaCallPacket_t);
                pack.cuda_memcpy.dst_buf = nullptr;
                pending_d2h_is_sst_ = true;
                pending_d2h_sst_addr_ = pack.cuda_memcpy.dst;
            } else {
                pending_d2h_host_buf_.assign(trace_packet.d2h_bytes, 0);
                pack.cuda_memcpy.dst = pending_d2h_host_buf_.empty()
                    ? 0 : reinterpret_cast<uint64_t>(pending_d2h_host_buf_.data());
            }
        }
        beginPacketIssue(pack);
    } else {
        finishReplay();
    }
}

void BalarRingBridge::beginPacketIssue(const BalarCudaCallPacket_t& pack)
{
    BalarCudaCallPacket_t pack_copy = pack;
    std::vector<uint8_t>* encoded = encode_balar_packet<BalarCudaCallPacket_t>(&pack_copy);

    stage_segments_.clear();
    flush_ranges_.clear();

    // Segment 0: the encoded command packet, in the control scratch region.
    stage_segments_.push_back({scratch_mem_addr_, std::vector<uint8_t>(encoded->begin(), encoded->end())});
    delete encoded;

    // Segment 1 (H2D only): the weight payload, in the DEDICATED separable region.
    if (!pending_weight_payload_.empty()) {
        stage_segments_.push_back({weight_stage_addr_, std::move(pending_weight_payload_)});
        pending_weight_payload_.clear();
    }

    seg_index_  = 0;
    seg_offset_ = 0;
    writes_outstanding_ = 0;
    packet_issue_active_ = true;

    if (verbose_)
        out_->output("BalarRingBridge: issue %s (%zu segments) scratch=0x%" PRIx64 "\n",
                     CudaAPIEnumToString(pack.cuda_call_id), stage_segments_.size(), scratch_mem_addr_);

    sendNextStageChunk();
}

void BalarRingBridge::sendNextStageChunk()
{
    // Walk segments in order, emitting cache-line-sized writes. When the last chunk
    // of the last segment is issued we flip to flushing (once its WriteResp lands).
    while (seg_index_ < stage_segments_.size()) {
        StageSegment& seg = stage_segments_[seg_index_];
        if (seg_offset_ >= seg.bytes.size()) { ++seg_index_; seg_offset_ = 0; continue; }
        size_t chunk = std::min<size_t>(seg.bytes.size() - seg_offset_, cache_line_size_);
        std::vector<uint8_t> payload(seg.bytes.begin() + seg_offset_,
                                     seg.bytes.begin() + seg_offset_ + chunk);
        auto* req = new StandardMem::Write(seg.addr + seg_offset_, chunk, payload, false);
        requests_[req->getID()] = std::make_pair("StageWrite", IfacePath::CACHE);
        ++writes_outstanding_;
        cache_link_->send(req);
        seg_offset_ += chunk;
        return;  // one outstanding chunk at a time (in-order, like balar's forked test CPU)
    }
}

void BalarRingBridge::onCacheWriteResp(StandardMem::WriteResp* resp)
{
    auto it = requests_.find(resp->getID());
    if (it == requests_.end()) out_->fatal(CALL_INFO, -1, "BalarRingBridge: unknown cache WriteResp\n");
    if (it->second.first == "StageWrite") {
        if (writes_outstanding_ > 0) --writes_outstanding_;
        // More chunks to write?
        bool more = (seg_index_ < stage_segments_.size()) &&
                    (seg_offset_ < stage_segments_[seg_index_].bytes.size() ||
                     seg_index_ + 1 < stage_segments_.size());
        if (more) {
            sendNextStageChunk();
        } else if (writes_outstanding_ == 0) {
            // All segments staged; build per-region flush ranges and flush.
            flush_ranges_.clear();
            for (auto& seg : stage_segments_) flush_ranges_.push_back({seg.addr, seg.bytes.size()});
            // A prior replay may have cached the D2H destination. Invalidate its
            // lines before balar's DMA writes beneath this interface so the
            // completion readback cannot observe stale cache data.
            if (pending_d2h_is_sst_ && pending_d2h_bytes_ > 0) {
                uint64_t first_line = pending_d2h_sst_addr_ - (pending_d2h_sst_addr_ % cache_line_size_);
                uint64_t last_addr = pending_d2h_sst_addr_ + pending_d2h_bytes_ - 1;
                uint64_t last_line = last_addr - (last_addr % cache_line_size_);
                flush_ranges_.push_back({first_line, (size_t)(last_line - first_line + cache_line_size_)});
            }
            size_t total_lines = 0;
            for (auto& r : flush_ranges_)
                total_lines += (r.second + cache_line_size_ - 1) / cache_line_size_;
            flushes_remaining_ = total_lines;
            flush_line_index_  = 0;
            sendNextFlush();
        }
    } else {
        out_->fatal(CALL_INFO, -1, "BalarRingBridge: unexpected cache WriteResp %s\n", it->second.first.c_str());
    }
    requests_.erase(it);
    delete resp;
}

void BalarRingBridge::sendNextFlush()
{
    if (flushes_remaining_ == 0) { sendDoorbell(); return; }
    // Map a linear line index onto (region, line-within-region).
    size_t idx = flush_line_index_;
    StandardMem::Addr line_addr = 0;
    for (auto& r : flush_ranges_) {
        size_t lines = (r.second + cache_line_size_ - 1) / cache_line_size_;
        if (idx < lines) { line_addr = r.first + idx * cache_line_size_; break; }
        idx -= lines;
    }
    auto* req = new StandardMem::FlushAddr(line_addr, cache_line_size_, true, 1);
    requests_[req->getID()] = std::make_pair("StageFlush", IfacePath::CACHE);
    cache_link_->send(req);
    ++flush_line_index_;
}

void BalarRingBridge::onCacheFlushResp(StandardMem::FlushResp* resp)
{
    auto it = requests_.find(resp->getID());
    if (it == requests_.end()) out_->fatal(CALL_INFO, -1, "BalarRingBridge: unknown cache FlushResp\n");
    if (flushes_remaining_ > 0) --flushes_remaining_;
    if (flushes_remaining_ > 0) sendNextFlush();
    else sendDoorbell();
    requests_.erase(it);
    delete resp;
}

void BalarRingBridge::sendDoorbell()
{
    std::vector<uint8_t> payload;
    uint64ToData(scratch_mem_addr_, &payload);
    auto* req = new StandardMem::Write(mmio_addr_, payload.size(), payload, false);
    requests_[req->getID()] = std::make_pair("Doorbell", IfacePath::MMIO);
    mmio_link_->send(req);
}

void BalarRingBridge::onMmioWriteResp(StandardMem::WriteResp* resp)
{
    auto it = requests_.find(resp->getID());
    if (it == requests_.end()) out_->fatal(CALL_INFO, -1, "BalarRingBridge: unknown mmio WriteResp\n");
    if (it->second.first == "Doorbell") sendStartCudaRetRead();
    else out_->fatal(CALL_INFO, -1, "BalarRingBridge: unexpected mmio WriteResp %s\n", it->second.first.c_str());
    requests_.erase(it);
    delete resp;
}

void BalarRingBridge::sendStartCudaRetRead()
{
    auto* req = new StandardMem::Read(mmio_addr_, sizeof(uint64_t));
    requests_[req->getID()] = std::make_pair("Start_CUDA_ret", IfacePath::MMIO);
    mmio_link_->send(req);
}

void BalarRingBridge::onMmioReadResp(StandardMem::ReadResp* resp)
{
    auto it = requests_.find(resp->getID());
    if (it == requests_.end()) out_->fatal(CALL_INFO, -1, "BalarRingBridge: unknown mmio ReadResp\n");
    if (it->second.first == "Start_CUDA_ret") {
        uint64_t ret_addr = dataToUInt64(&(resp->data));
        sendReadRetPacket(ret_addr);
    } else {
        out_->fatal(CALL_INFO, -1, "BalarRingBridge: unexpected mmio ReadResp %s\n", it->second.first.c_str());
    }
    requests_.erase(it);
    delete resp;
}

void BalarRingBridge::sendReadRetPacket(uint64_t ret_addr)
{
    auto* req = new StandardMem::Read(ret_addr, sizeof(BalarCudaCallReturnPacket_t));
    requests_[req->getID()] = std::make_pair("Read_CUDA_ret_packet", IfacePath::CACHE);
    cache_link_->send(req);
}

void BalarRingBridge::sendNextD2HRead()
{
    size_t remaining = pending_d2h_read_bytes_ - pending_d2h_read_offset_;
    uint64_t addr = pending_d2h_sst_addr_ + pending_d2h_read_offset_;
    size_t line_remaining = cache_line_size_ - (addr % cache_line_size_);
    pending_d2h_read_chunk_ = std::min(remaining, line_remaining);

    auto* req = new StandardMem::Read(addr, pending_d2h_read_chunk_);
    requests_[req->getID()] = std::make_pair("Read_D2H_payload", IfacePath::CACHE);
    cache_link_->send(req);
}

void BalarRingBridge::onCacheReadResp(StandardMem::ReadResp* resp)
{
    auto it = requests_.find(resp->getID());
    if (it == requests_.end()) out_->fatal(CALL_INFO, -1, "BalarRingBridge: unknown cache ReadResp\n");
    if (it->second.first == "Read_CUDA_ret_packet") {
        auto* ret = decode_balar_packet<BalarCudaCallReturnPacket_t>(&(resp->data));
        bool complete = completeCudaCall(ret);
        delete ret;
        requests_.erase(it);
        delete resp;
        if (complete) finishCudaCall();
        else sendNextD2HRead();
        return;
    }
    if (it->second.first == "Read_D2H_payload") {
        if (resp->data.size() != pending_d2h_read_chunk_)
            out_->fatal(CALL_INFO, -1,
                        "BalarRingBridge: D2H readback returned %zu bytes, expected %zu\n",
                        resp->data.size(), pending_d2h_read_chunk_);

        checksum_ = fnv1a64(resp->data.data(), pending_d2h_read_chunk_, checksum_);
        pending_d2h_read_offset_ += pending_d2h_read_chunk_;
        bool complete = pending_d2h_read_offset_ == pending_d2h_read_bytes_;
        requests_.erase(it);
        delete resp;

        if (complete) {
            if (verbose_)
                out_->output("BalarRingBridge: D2H %zu bytes from SST memory -> checksum=0x%" PRIx64 "\n",
                             pending_d2h_read_bytes_, checksum_);
            releasePendingD2H();
            finishCudaCall();
        } else {
            sendNextD2HRead();
        }
        return;
    }
    out_->fatal(CALL_INFO, -1, "BalarRingBridge: unexpected cache ReadResp %s\n", it->second.first.c_str());
    requests_.erase(it);
    delete resp;
}

bool BalarRingBridge::completeCudaCall(const BalarCudaCallReturnPacket_t* ret_pack)
{
    if (ret_pack->cuda_call_id == CUDA_REG_FAT_BINARY && trace_parser_) {
        trace_parser_->setFatbinHandle(ret_pack->fat_cubin_handle);
    } else if (ret_pack->cuda_call_id == CUDA_MALLOC) {
        if (ret_pack->cudamalloc.devptr_addr)
            *(CUdeviceptr*)ret_pack->cudamalloc.devptr_addr = ret_pack->cudamalloc.malloc_addr;
        if (verbose_) out_->output("BalarRingBridge: cudaMalloc -> 0x%" PRIx64 "\n",
                                   (uint64_t)ret_pack->cudamalloc.malloc_addr);
    } else if (ret_pack->cuda_call_id == CUDA_MEMCPY &&
               ret_pack->cudamemcpy.kind == cudaMemcpyDeviceToHost) {
        // Fold GPU D2H into the action checksum. sim_data is NULL for SST-memory
        // copies — keep this call active while cache-line reads fetch the DMA
        // bytes balar committed to simulated memory.
        size_t n = ret_pack->cudamemcpy.size;
        const uint8_t* data = (const uint8_t*)ret_pack->cudamemcpy.sim_data;
        if (n != pending_d2h_bytes_)
            out_->fatal(CALL_INFO, -1,
                        "BalarRingBridge: D2H completion size %zu does not match request size %zu\n",
                        n, pending_d2h_bytes_);
        if (data && n) {
            checksum_ = fnv1a64(data, n, checksum_);
            if (verbose_) out_->output("BalarRingBridge: D2H %zu bytes -> checksum=0x%" PRIx64 "\n",
                                       n, checksum_);
            releasePendingD2H();
        } else if (pending_d2h_is_sst_ && n) {
            pending_d2h_read_bytes_ = n;
            pending_d2h_read_offset_ = 0;
            return false;
        } else if (n) {
            out_->fatal(CALL_INFO, -1,
                        "BalarRingBridge: D2H completion returned no data for a non-SST copy\n");
        } else {
            releasePendingD2H();
        }
    }
    return true;
}

void BalarRingBridge::finishCudaCall()
{
    packet_issue_active_ = false;
    issueNextPacket();
}

void BalarRingBridge::releasePendingD2H()
{
    pending_d2h_host_buf_.clear();
    pending_d2h_sst_addr_ = 0;
    pending_d2h_bytes_ = 0;
    pending_d2h_read_bytes_ = 0;
    pending_d2h_read_offset_ = 0;
    pending_d2h_read_chunk_ = 0;
    pending_d2h_is_sst_ = false;
}

void BalarRingBridge::finishReplay()
{
    ++replays_;
    replayed_once_ = true;
    replay_active_ = false;

    // Publish the running checksum into the shared pipeline state; the CPU driver
    // snapshots it into the frame's actionChecksum at ACTUATE close (same slot the
    // mini-GPU used), and ActionScorer diffs it against the golden log.
    if (!state_key_.empty()) {
        PipelineStateBase* s = PipelineStateRegistry<PipelineStateBase>::getMutable(state_key_);
        if (!s) s = PipelineStateRegistry<PipelineStateBase>::getOrCreate(state_key_);
        s->publishWatcherChecksum(checksum_);
    }

    if (verbose_)
        out_->output("BalarRingBridge: replay %" PRIu64 " done, checksum=0x%" PRIx64 "\n",
                     replays_, checksum_);

    if (ring_link_) ring_link_->send(new HaliEvent(RingTag::Done, 0u));

    // Drain ALL Cmds that arrived mid-replay. In Done-only mode each queued
    // Cmd gets its own Done; in replay-each mode start the next replay, which
    // drains the rest through its own finishReplay.
    while (cmd_pending_ > 0) {
        --cmd_pending_;
        if (replay_each_cmd_) { beginTrace(); break; }
        if (ring_link_) ring_link_->send(new HaliEvent(RingTag::Done, 0u));
    }
}

#endif // HAVE_BALAR_BRIDGE
