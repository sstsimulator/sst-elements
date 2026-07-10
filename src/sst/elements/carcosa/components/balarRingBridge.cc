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
#include "sst/elements/carcosa/components/pipelineStateRegistry.h"

// balar packet encode/decode helpers (templates).
#include <sst/elements/balar/util.h>

#include <algorithm>
#include <cinttypes>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <queue>
#include <sstream>

using namespace SST;
using namespace SST::Interfaces;
using namespace SST::Carcosa;
// Bring in balar's CUDA-call ABI: the BalarCudaCall*_t packets, the encode/decode
// templates, CudaAPIEnumToString, and the CudaAPI_t enum constants (CUDA_MALLOC,
// CUDA_MEMCPY, ...) which live in this namespace (mirrors balar's forked test CPU.cc).
using namespace SST::BalarComponent;

// Chained FNV-1a (carcosaHash.h) over D2H chunks; serialized packet SM makes
// chunk order deterministic so this equals one FNV over concatenated bytes —
// same construction as CriticalActionWatcher (do not fork the FNV constant).

namespace {

std::string brbTrim(const std::string& s)
{
    size_t start = s.find_first_not_of(" \t");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t");
    return s.substr(start, end - start + 1);
}

std::vector<std::string> brbSplit(const std::string& s, const std::string& delim)
{
    std::vector<std::string> out;
    size_t pos = 0;
    while (pos < s.size()) {
        size_t next = s.find(delim, pos);
        if (next == std::string::npos) { out.push_back(s.substr(pos)); break; }
        out.push_back(s.substr(pos, next - pos));
        pos = next + delim.size();
    }
    return out;
}

std::map<std::string, std::string> brbMapFromVec(const std::vector<std::string>& params, const std::string& delim)
{
    std::map<std::string, std::string> m;
    for (const auto& p : params) {
        size_t pos = p.find(delim);
        if (pos != std::string::npos) m[brbTrim(p.substr(0, pos))] = brbTrim(p.substr(pos + delim.size()));
    }
    return m;
}

std::string brbLookupParam(const std::map<std::string, std::string>& params, const std::string& key, SST::Output* out)
{
    auto it = params.find(key);
    if (it != params.end()) return brbTrim(it->second);
    for (const auto& param : params)
        if (brbTrim(param.first) == key) return brbTrim(param.second);
    std::ostringstream keys;
    for (const auto& param : params) keys << " '" << param.first << "'";
    out->fatal(CALL_INFO, -1, "Trace parameter '%s' not found. Available keys:%s\n", key.c_str(), keys.str().c_str());
    return "";
}

} // namespace

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

// CUDA-API trace parser (from balar/testcpu/). H2D weight payloads stage into
// weightStageAddr_ (not scratch after the command packet) so EccGuard can
// confine injection to weights only.
class BalarRingBridge::CudaAPITraceParser {
public:
    CudaAPITraceParser(BalarRingBridge* b, SST::Output* out,
                       const std::string& trace_file, const std::string& cuda_executable)
        : b_(b), out_(out), cuda_executable_(cuda_executable), fat_cubin_handle_(0), has_peeked_packet_(false)
    {
        trace_file_ = trace_file;
        size_t sep = trace_file.rfind("/");
        trace_base_path_ = (sep == std::string::npos) ? "./" : trace_file.substr(0, sep + 1);
        rewind();
    }

    // Reopen the trace from the top and re-queue the fatbin registration. Called
    // per replay so each ring Cmd re-runs the same staged GEMM sequence.
    void rewind()
    {
        if (trace_stream_.is_open()) trace_stream_.close();
        trace_stream_.open(trace_file_, std::ifstream::in);
        if (!trace_stream_.is_open())
            out_->fatal(CALL_INFO, -1, "BalarRingBridge: trace file '%s' does not exist\n", trace_file_.c_str());
        has_peeked_packet_ = false;
        std::queue<BalarCudaCallPacket_t> empty;
        std::swap(init_packets_, empty);
        // Register the fatbin only on the first replay; balar keeps the handle.
        if (!registered_fatbin_) {
            BalarCudaCallPacket_t fatbin{};
            fatbin.cuda_call_id = CUDA_REG_FAT_BINARY;
            fatbin.isSSTmem = false;
            strncpy(fatbin.register_fatbin.file_name, cuda_executable_.c_str(), BALAR_CUDA_MAX_FILE_NAME - 1);
            fatbin.register_fatbin.file_name[BALAR_CUDA_MAX_FILE_NAME - 1] = '\0';
            init_packets_.push(fatbin);
        }
    }

    bool getNextPacket(BalarCudaCallPacket_t& pack)
    {
        if (has_peeked_packet_) { pack = peeked_packet_; has_peeked_packet_ = false; return true; }
        if (!init_packets_.empty()) { pack = init_packets_.front(); init_packets_.pop(); return true; }
        if (trace_stream_.eof()) return false;

        std::string line;
        std::getline(trace_stream_, line);
        if (line.empty()) return false;
        out_->verbose(CALL_INFO, 2, 0, "Trace: %s\n", line.c_str());

        pack = BalarCudaCallPacket_t{};
        pack.isSSTmem = false;

        size_t first_col = line.find(":");
        std::string cuda_call_type = line.substr(0, first_col);
        { std::string rest = line.substr(first_col + 1); line = brbTrim(rest); }
        auto params_map = brbMapFromVec(brbSplit(line, ","), ":");

        if (cuda_call_type.find("memalloc") != std::string::npos) {
            pack.cuda_call_id = CUDA_MALLOC;
            std::string dptr_name = brbLookupParam(params_map, "dptr", out_);
            size_t size = 0;
            std::stringstream(brbLookupParam(params_map, "size", out_)) >> size;
            auto it = dptr_map_.find(dptr_name);
            if (it == dptr_map_.end()) {
                auto* dptr = (CUdeviceptr*)malloc(sizeof(CUdeviceptr));
                dptr_map_[dptr_name] = dptr;
                pack.cuda_malloc.devPtr = (void**)dptr;
            } else {
                pack.cuda_malloc.devPtr = (void**)it->second;
            }
            pack.cuda_malloc.size = size;
            return true;
        }
        if (cuda_call_type.find("memcpyH2D") != std::string::npos || cuda_call_type.find("memcpyD2H") != std::string::npos) {
            pack.cuda_call_id = CUDA_MEMCPY;
            std::string dptr_name = brbLookupParam(params_map, "device_ptr", out_);
            size_t size = 0;
            std::stringstream(brbLookupParam(params_map, "size", out_)) >> size;
            std::string data_path = trace_base_path_ + brbLookupParam(params_map, "data_file", out_);
            std::ifstream data_stream(data_path, std::ios::binary);
            if (!data_stream.is_open())
                out_->fatal(CALL_INFO, -1, "BalarRingBridge: data file '%s' not found\n", data_path.c_str());
            std::vector<uint8_t> file_data(size);
            data_stream.read((char*)file_data.data(), size);
            uint8_t* real_data = (uint8_t*)malloc(size);
            memcpy(real_data, file_data.data(), size);
            auto it = dptr_map_.find(dptr_name);
            if (it == dptr_map_.end())
                out_->fatal(CALL_INFO, -1, "BalarRingBridge: unknown device pointer '%s'\n", dptr_name.c_str());
            if (cuda_call_type.find("memcpyH2D") != std::string::npos) {
                pack.isSSTmem = true;
                pack.cuda_memcpy.kind = cudaMemcpyHostToDevice;
                pack.cuda_memcpy.dst = *it->second;
                pack.cuda_memcpy.count = size;
                pack.cuda_memcpy.payload = (uint64_t)real_data;
                // Separable staging: the H2D source is the dedicated weight region,
                // NOT contiguous with the command packet.
                pack.cuda_memcpy.src = b_->weightStageAddr_;
                b_->pending_weight_payload_ = std::move(file_data);
            } else {
                pack.cuda_memcpy.kind = cudaMemcpyDeviceToHost;
                pack.cuda_memcpy.src = (uint64_t)*it->second;
                pack.cuda_memcpy.count = size;
                pack.cuda_memcpy.payload = (uint64_t)real_data;
                if (size >= b_->cacheLineSize_) {
                    pack.isSSTmem = true;
                    pack.cuda_memcpy.dst = b_->scratchMemAddr_ + sizeof(BalarCudaCallPacket_t);
                    pack.cuda_memcpy.dst_buf = nullptr;
                    b_->pending_d2h_is_sst_ = true;
                    b_->pending_d2h_sst_addr_ = pack.cuda_memcpy.dst;
                } else {
                    uint8_t* buf = size ? (uint8_t*)malloc(size) : nullptr;
                    pack.cuda_memcpy.dst = (uint64_t)buf;
                    b_->pending_d2h_host_buf_ = buf;
                }
                b_->pending_d2h_bytes_ = size;
            }
            return true;
        }
        if (cuda_call_type.find("kernel launch") != std::string::npos) {
            std::string func_name = brbLookupParam(params_map, "name", out_);
            std::string ptx_name = brbLookupParam(params_map, "ptx_name", out_);
            BalarCudaCallPacket_t config{}, set_arg{}, launch{}, reg_fn{};
            config.cuda_call_id = CUDA_CONFIG_CALL;
            set_arg.cuda_call_id = CUDA_SET_ARG;
            launch.cuda_call_id = CUDA_LAUNCH;
            config.configure_call.gdx = std::stoul(brbLookupParam(params_map, "gdx", out_));
            config.configure_call.gdy = std::stoul(brbLookupParam(params_map, "gdy", out_));
            config.configure_call.gdz = std::stoul(brbLookupParam(params_map, "gdz", out_));
            config.configure_call.bdx = std::stoul(brbLookupParam(params_map, "bdx", out_));
            config.configure_call.bdy = std::stoul(brbLookupParam(params_map, "bdy", out_));
            config.configure_call.bdz = std::stoul(brbLookupParam(params_map, "bdz", out_));
            config.configure_call.sharedMem = std::stoul(brbLookupParam(params_map, "sharedBytes", out_));
            config.configure_call.stream = nullptr;
            init_packets_.push(config);

            if (func_map_.find(func_name) == func_map_.end()) {
                uint64_t func_id = func_map_.size();
                func_map_[func_name] = func_id;
                reg_fn.cuda_call_id = CUDA_REG_FUNCTION;
                reg_fn.register_function.fatCubinHandle = fat_cubin_handle_;
                reg_fn.register_function.hostFun = func_id;
                strncpy(reg_fn.register_function.deviceFun, ptx_name.c_str(), BALAR_CUDA_MAX_KERNEL_NAME - 1);
                pack = reg_fn;
            } else {
                pack = config;
                init_packets_.pop();
            }

            std::string arguments = brbLookupParam(params_map, "args", out_);
            size_t offset = 0;
            while (!arguments.empty()) {
                size_t pos = arguments.find("/");
                std::string arg_val = arguments.substr(0, pos);
                arguments = arguments.substr(pos + 1);
                pos = arguments.find("/");
                std::string arg_size_str = arguments.substr(0, pos);
                arguments = arguments.substr(pos + 1);
                size_t arg_size = 0;
                std::stringstream(arg_size_str) >> arg_size;
                size_t align_amount = arg_size;
                offset = (offset + align_amount - 1) / align_amount * align_amount;
                set_arg.setup_argument.size = arg_size;
                set_arg.setup_argument.offset = offset;
                offset += arg_size;
                if (arg_val.find("dptr") != std::string::npos) {
                    set_arg.setup_argument.arg = (uint64_t)*dptr_map_.at(arg_val);
                } else if (arg_val.find(".") != std::string::npos) {
                    double val = std::stod(arg_val);
                    set_arg.setup_argument.arg = 0;
                    if (arg_size == 8) { memcpy(set_arg.setup_argument.value, &val, arg_size); }
                    else { float val_f = (float)val; memcpy(set_arg.setup_argument.value, &val_f, arg_size); }
                } else {
                    int val = std::stoi(arg_val);
                    set_arg.setup_argument.arg = 0;
                    memcpy(set_arg.setup_argument.value, &val, arg_size);
                }
                init_packets_.push(set_arg);
            }
            launch.cuda_launch.func = func_map_.at(func_name);
            init_packets_.push(launch);
            return true;
        }
        if (cuda_call_type.find("free") != std::string::npos) {
            pack.cuda_call_id = CUDA_FREE;
            std::string dptr_name = brbLookupParam(params_map, "dptr", out_);
            pack.cuda_free.devPtr = (void*)*dptr_map_.at(dptr_name);
            return true;
        }
        return false;
    }

    bool hasNextPacket()
    {
        if (has_peeked_packet_) return true;
        has_peeked_packet_ = getNextPacket(peeked_packet_);
        return has_peeked_packet_;
    }

    void setFatbinHandle(uint64_t handle) { fat_cubin_handle_ = handle; registered_fatbin_ = true; }

private:
    BalarRingBridge* b_;
    SST::Output* out_;
    std::string cuda_executable_;
    std::string trace_file_;
    std::string trace_base_path_;
    std::ifstream trace_stream_;
    std::queue<BalarCudaCallPacket_t> init_packets_;
    std::map<std::string, CUdeviceptr*> dptr_map_;
    std::map<std::string, uint64_t> func_map_;
    uint64_t fat_cubin_handle_;
    bool registered_fatbin_ = false;
    bool has_peeked_packet_;
    BalarCudaCallPacket_t peeked_packet_;
};

// ---------------------------------------------------------------------------
// Construction / lifecycle
// ---------------------------------------------------------------------------
BalarRingBridge::BalarRingBridge(ComponentId_t id, Params& params)
    : InterceptionAgentAPI(id, params)
{
    out_             = new Output("BalarRingBridge[@p:@l] ", 1, 0, Output::STDOUT);
    verbose_         = params.find<bool>("verbose", false);
    stateKey_        = params.find<std::string>("state_key", "cpu0_vla");
    mmioAddr_        = params.find<uint64_t>("mmio_addr", 0);
    scratchMemAddr_  = params.find<uint64_t>("scratch_mem_addr", 0);
    weightStageAddr_ = params.find<uint64_t>("weight_stage_addr", 0x20000000);
    cacheLineSize_   = params.find<uint64_t>("cache_line_size", 64);
    traceFile_       = params.find<std::string>("trace_file", "cuda_calls.trace");
    replayEachCmd_   = params.find<bool>("replay_each_cmd", false);

    bool found = false;
    cudaExecutable_  = params.find<std::string>("cuda_executable", found);
    if (!found)
        out_->fatal(CALL_INFO, -1, "BalarRingBridge: 'cuda_executable' is required (fatbin registration)\n");
    if (cacheLineSize_ == 0)
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
    delete out_;
    delete trace_parser_;
    delete cache_handlers_;
    delete mmio_handlers_;
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
    trace_parser_ = new CudaAPITraceParser(this, out_, traceFile_, cudaExecutable_);
    if (verbose_)
        out_->output("BalarRingBridge: setup, scratch=0x%" PRIx64 " weights=0x%" PRIx64
                     " mmio=0x%" PRIx64 " trace=%s\n",
                     scratchMemAddr_, weightStageAddr_, mmioAddr_, traceFile_.c_str());
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
    if (replayed_once_ && !replayEachCmd_) {
        if (ringLink_) ringLink_->send(new HaliEvent(RingTag::Done, 0u));
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
    BalarCudaCallPacket_t pack{};
    if (trace_parser_ && trace_parser_->getNextPacket(pack)) {
        beginPacketIssue(pack);
    } else {
        finishReplay();
    }
}

// ---------------------------------------------------------------------------
// Packet-issue state machine (mirrors balar's forked test CPU, ring-driven)
// ---------------------------------------------------------------------------
void BalarRingBridge::beginPacketIssue(const BalarCudaCallPacket_t& pack)
{
    BalarCudaCallPacket_t pack_copy = pack;
    std::vector<uint8_t>* encoded = encode_balar_packet<BalarCudaCallPacket_t>(&pack_copy);

    stage_segments_.clear();
    flush_ranges_.clear();

    // Segment 0: the encoded command packet, in the control scratch region.
    stage_segments_.push_back({scratchMemAddr_, std::vector<uint8_t>(encoded->begin(), encoded->end())});
    delete encoded;

    // Segment 1 (H2D only): the weight payload, in the DEDICATED separable region.
    if (!pending_weight_payload_.empty()) {
        stage_segments_.push_back({weightStageAddr_, std::move(pending_weight_payload_)});
        pending_weight_payload_.clear();
    }

    seg_index_  = 0;
    seg_offset_ = 0;
    writes_outstanding_ = 0;
    packet_issue_active_ = true;

    if (verbose_)
        out_->output("BalarRingBridge: issue %s (%zu segments) scratch=0x%" PRIx64 "\n",
                     CudaAPIEnumToString(pack.cuda_call_id), stage_segments_.size(), scratchMemAddr_);

    sendNextStageChunk();
}

void BalarRingBridge::sendNextStageChunk()
{
    // Walk segments in order, emitting cache-line-sized writes. When the last chunk
    // of the last segment is issued we flip to flushing (once its WriteResp lands).
    while (seg_index_ < stage_segments_.size()) {
        StageSegment& seg = stage_segments_[seg_index_];
        if (seg_offset_ >= seg.bytes.size()) { ++seg_index_; seg_offset_ = 0; continue; }
        size_t chunk = std::min<size_t>(seg.bytes.size() - seg_offset_, cacheLineSize_);
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
                uint64_t first_line = pending_d2h_sst_addr_ - (pending_d2h_sst_addr_ % cacheLineSize_);
                uint64_t last_addr = pending_d2h_sst_addr_ + pending_d2h_bytes_ - 1;
                uint64_t last_line = last_addr - (last_addr % cacheLineSize_);
                flush_ranges_.push_back({first_line, (size_t)(last_line - first_line + cacheLineSize_)});
            }
            size_t total_lines = 0;
            for (auto& r : flush_ranges_)
                total_lines += (r.second + cacheLineSize_ - 1) / cacheLineSize_;
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
        size_t lines = (r.second + cacheLineSize_ - 1) / cacheLineSize_;
        if (idx < lines) { line_addr = r.first + idx * cacheLineSize_; break; }
        idx -= lines;
    }
    auto* req = new StandardMem::FlushAddr(line_addr, cacheLineSize_, true, 1);
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
    uint64ToData(scratchMemAddr_, &payload);
    auto* req = new StandardMem::Write(mmioAddr_, payload.size(), payload, false);
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
    auto* req = new StandardMem::Read(mmioAddr_, sizeof(uint64_t));
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
    size_t line_remaining = cacheLineSize_ - (addr % cacheLineSize_);
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
    free(pending_d2h_host_buf_);
    pending_d2h_host_buf_ = nullptr;
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
    if (!stateKey_.empty()) {
        PipelineStateBase* s = PipelineStateRegistry<PipelineStateBase>::getMutable(stateKey_);
        if (!s) s = PipelineStateRegistry<PipelineStateBase>::getOrCreate(stateKey_);
        s->watcherActionChecksum      = checksum_;
        s->watcherActionChecksumValid = true;
    }

    if (verbose_)
        out_->output("BalarRingBridge: replay %" PRIu64 " done, checksum=0x%" PRIx64 "\n",
                     replays_, checksum_);

    if (ringLink_) ringLink_->send(new HaliEvent(RingTag::Done, 0u));

    // Drain ALL Cmds that arrived mid-replay. In Done-only mode each queued
    // Cmd gets its own Done; in replay-each mode start the next replay, which
    // drains the rest through its own finishReplay.
    while (cmd_pending_ > 0) {
        --cmd_pending_;
        if (replayEachCmd_) { beginTrace(); break; }
        if (ringLink_) ringLink_->send(new HaliEvent(RingTag::Done, 0u));
    }
}

#endif // HAVE_BALAR_BRIDGE
