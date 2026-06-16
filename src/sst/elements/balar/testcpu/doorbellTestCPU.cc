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

#include <sst_config.h>

#include <sst/core/params.h>
#include <sst/elements/memHierarchy/util.h>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

#include "testcpu/doorbellTestCPU.h"
#include "util.h"

using namespace SST;
using namespace SST::Interfaces;
using namespace SST::BalarComponent;
using namespace SST::Statistics;

namespace {

std::string trim(const std::string& s)
{
    size_t start = s.find_first_not_of(" \t");
    if (start == std::string::npos)
        return "";
    size_t end = s.find_last_not_of(" \t");
    return s.substr(start, end - start + 1);
}

std::vector<std::string> split(const std::string& s, const std::string& delim)
{
    std::vector<std::string> out;
    size_t pos = 0;
    while (pos < s.size()) {
        size_t next = s.find(delim, pos);
        if (next == std::string::npos) {
            out.push_back(s.substr(pos));
            break;
        }
        out.push_back(s.substr(pos, next - pos));
        pos = next + delim.size();
    }
    return out;
}

std::map<std::string, std::string> map_from_vec(const std::vector<std::string>& params, const std::string& delim)
{
    std::map<std::string, std::string> m;
    for (const auto& p : params) {
        size_t pos = p.find(delim);
        if (pos != std::string::npos) {
            m[trim(p.substr(0, pos))] = trim(p.substr(pos + delim.size()));
        }
    }
    return m;
}

std::string lookup_param(const std::map<std::string, std::string>& params, const std::string& key, SST::Output* out)
{
    auto it = params.find(key);
    if (it != params.end()) {
        return trim(it->second);
    }
    for (const auto& param : params) {
        if (trim(param.first) == key) {
            return trim(param.second);
        }
    }
    std::ostringstream keys;
    for (const auto& param : params) {
        keys << " '" << param.first << "'";
    }
    out->fatal(CALL_INFO, -1, "Trace parameter '%s' not found. Available keys:%s\n", key.c_str(), keys.str().c_str());
    return "";
}

} // namespace

class DoorbellTestCPU::CacheHandlers : public StandardMem::RequestHandler {
public:
    friend class DoorbellTestCPU;
    CacheHandlers(DoorbellTestCPU* cpu, SST::Output* out) : StandardMem::RequestHandler(out), cpu_(cpu) {}
    virtual ~CacheHandlers() {}
    void handle(StandardMem::ReadResp* resp) override { cpu_->onCacheReadResp(resp); }
    void handle(StandardMem::WriteResp* resp) override { cpu_->onCacheWriteResp(resp); }
    void handle(StandardMem::FlushResp* resp) override { cpu_->onCacheFlushResp(resp); }
    void handle(StandardMem::Write* write) override { cpu_->onCacheWriteReq(write); }

private:
    DoorbellTestCPU* cpu_;
};

class DoorbellTestCPU::MmioHandlers : public StandardMem::RequestHandler {
public:
    friend class DoorbellTestCPU;
    MmioHandlers(DoorbellTestCPU* cpu, SST::Output* out) : StandardMem::RequestHandler(out), cpu_(cpu) {}
    virtual ~MmioHandlers() {}
    void handle(StandardMem::ReadResp* resp) override { cpu_->onMmioReadResp(resp); }
    void handle(StandardMem::WriteResp* resp) override { cpu_->onMmioWriteResp(resp); }
    void handle(StandardMem::Write* write) override { cpu_->onMmioWriteReq(write); }

private:
    DoorbellTestCPU* cpu_;
};

class DoorbellTestCPU::CudaAPITraceParser {
public:
    CudaAPITraceParser(DoorbellTestCPU* cpu, SST::Output* out, const std::string& trace_file, const std::string& cuda_executable)
        : cpu_(cpu), out_(out), cuda_executable_(cuda_executable), fat_cubin_handle_(0), has_peeked_packet_(false)
    {
        trace_stream_.open(trace_file, std::ifstream::in);
        if (!trace_stream_.is_open()) {
            out_->fatal(CALL_INFO, -1, "Error: trace file '%s' does not exist\n", trace_file.c_str());
        }
        size_t sep = trace_file.rfind("/");
        trace_base_path_ = (sep == std::string::npos) ? "./" : trace_file.substr(0, sep + 1);

        BalarCudaCallPacket_t fatbin{};
        fatbin.cuda_call_id = CUDA_REG_FAT_BINARY;
        fatbin.isSSTmem = false;
        strncpy(fatbin.register_fatbin.file_name, cuda_executable_.c_str(), BALAR_CUDA_MAX_FILE_NAME - 1);
        fatbin.register_fatbin.file_name[BALAR_CUDA_MAX_FILE_NAME - 1] = '\0';
        init_packets_.push(fatbin);
    }

    bool getNextPacket(BalarCudaCallPacket_t& pack)
    {
        if (has_peeked_packet_) {
            pack = peeked_packet_;
            has_peeked_packet_ = false;
            return true;
        }
        if (!init_packets_.empty()) {
            pack = init_packets_.front();
            init_packets_.pop();
            return true;
        }
        if (trace_stream_.eof()) {
            return false;
        }

        std::string line;
        std::getline(trace_stream_, line);
        if (line.empty()) {
            return false;
        }
        out_->verbose(CALL_INFO, 2, 0, "Trace: %s\n", line.c_str());

        pack = BalarCudaCallPacket_t{};
        pack.isSSTmem = false;

        size_t first_col = line.find(":");
        std::string cuda_call_type = line.substr(0, first_col);
        {
            std::string rest = line.substr(first_col + 1);
            line = trim(rest);
        }
        auto params_map = map_from_vec(split(line, ","), ":");

        if (cuda_call_type.find("memalloc") != std::string::npos) {
            pack.cuda_call_id = CUDA_MALLOC;
            std::string dptr_name = lookup_param(params_map, "dptr", out_);
            size_t size = 0;
            std::stringstream(lookup_param(params_map, "size", out_)) >> size;
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
            std::string dptr_name = lookup_param(params_map, "device_ptr", out_);
            size_t size = 0;
            std::stringstream(lookup_param(params_map, "size", out_)) >> size;
            std::string data_path = trace_base_path_ + lookup_param(params_map, "data_file", out_);
            std::ifstream data_stream(data_path, std::ios::binary);
            if (!data_stream.is_open()) {
                out_->fatal(CALL_INFO, -1, "Error: data file '%s' not found\n", data_path.c_str());
            }
            std::vector<uint8_t> file_data(size);
            data_stream.read((char*)file_data.data(), size);
            uint8_t* real_data = (uint8_t*)malloc(size);
            memcpy(real_data, file_data.data(), size);
            auto it = dptr_map_.find(dptr_name);
            if (it == dptr_map_.end()) {
                out_->fatal(CALL_INFO, -1, "Unknown device pointer '%s'\n", dptr_name.c_str());
            }
            if (cuda_call_type.find("memcpyH2D") != std::string::npos) {
                pack.isSSTmem = true;
                pack.cuda_memcpy.kind = cudaMemcpyHostToDevice;
                pack.cuda_memcpy.dst = *it->second;
                pack.cuda_memcpy.count = size;
                pack.cuda_memcpy.payload = (uint64_t)real_data;
                pack.cuda_memcpy.src = cpu_->scratchMemAddr_ + sizeof(BalarCudaCallPacket_t);
                cpu_->scratch_append_payload_ = std::move(file_data);
            } else {
                uint8_t* buf = (uint8_t*)malloc(size);
                pack.cuda_memcpy.kind = cudaMemcpyDeviceToHost;
                pack.cuda_memcpy.dst = (uint64_t)buf;
                pack.cuda_memcpy.src = (uint64_t)*it->second;
                pack.cuda_memcpy.count = size;
                pack.cuda_memcpy.payload = (uint64_t)real_data;
                if (size >= cpu_->cacheLineSize_) {
                    pack.isSSTmem = true;
                    pack.cuda_memcpy.dst = cpu_->scratchMemAddr_ + sizeof(BalarCudaCallPacket_t);
                    pack.cuda_memcpy.dst_buf = buf;
                }
            }
            return true;
        }
        if (cuda_call_type.find("kernel launch") != std::string::npos) {
            std::string func_name = lookup_param(params_map, "name", out_);
            std::string ptx_name = lookup_param(params_map, "ptx_name", out_);
            BalarCudaCallPacket_t config{}, set_arg{}, launch{}, reg_fn{};
            config.cuda_call_id = CUDA_CONFIG_CALL;
            set_arg.cuda_call_id = CUDA_SET_ARG;
            launch.cuda_call_id = CUDA_LAUNCH;
            config.configure_call.gdx = std::stoul(lookup_param(params_map, "gdx", out_));
            config.configure_call.gdy = std::stoul(lookup_param(params_map, "gdy", out_));
            config.configure_call.gdz = std::stoul(lookup_param(params_map, "gdz", out_));
            config.configure_call.bdx = std::stoul(lookup_param(params_map, "bdx", out_));
            config.configure_call.bdy = std::stoul(lookup_param(params_map, "bdy", out_));
            config.configure_call.bdz = std::stoul(lookup_param(params_map, "bdz", out_));
            config.configure_call.sharedMem = std::stoul(lookup_param(params_map, "sharedBytes", out_));
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

            std::string arguments = lookup_param(params_map, "args", out_);
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
                    if (arg_size == 8) {
                        memcpy(set_arg.setup_argument.value, &val, arg_size);
                    } else {
                        float val_f = (float)val;
                        memcpy(set_arg.setup_argument.value, &val_f, arg_size);
                    }
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
            std::string dptr_name = lookup_param(params_map, "dptr", out_);
            pack.cuda_free.devPtr = (void*)*dptr_map_.at(dptr_name);
            return true;
        }
        return false;
    }

    bool hasNextPacket()
    {
        if (has_peeked_packet_) {
            return true;
        }
        has_peeked_packet_ = getNextPacket(peeked_packet_);
        return has_peeked_packet_;
    }

    void setFatbinHandle(uint64_t handle) { fat_cubin_handle_ = handle; }

private:
    DoorbellTestCPU* cpu_;
    SST::Output* out_;
    std::string cuda_executable_;
    std::string trace_base_path_;
    std::ifstream trace_stream_;
    std::queue<BalarCudaCallPacket_t> init_packets_;
    std::map<std::string, CUdeviceptr*> dptr_map_;
    std::map<std::string, uint64_t> func_map_;
    uint64_t fat_cubin_handle_;
    bool has_peeked_packet_;
    BalarCudaCallPacket_t peeked_packet_;
};

DoorbellTestCPU::DoorbellTestCPU(ComponentId_t id, Params& params) : Component(id)
{
    out.init("DoorbellTestCPU[@f:@l:@p] ", params.find<unsigned int>("verbose", 1), 0, Output::STDOUT);

    bool found = false;
    mmioAddr_ = params.find<uint64_t>("mmio_addr", "0", found);
    scratchMemAddr_ = params.find<uint64_t>("scratch_mem_addr", "0", found);
    cacheLineSize_ = params.find<uint64_t>("cache_line_size", "64", found);
    if (cacheLineSize_ == 0) {
        out.fatal(CALL_INFO, -1, "cache_line_size must be > 0\n");
    }

    std::string mode_str = params.find<std::string>("mode", "trace");
    if (mode_str == "doorbell_only") {
        mode_ = DriverMode::DOORBELL_ONLY;
    } else {
        mode_ = DriverMode::TRACE;
    }

    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

    std::string clock_freq = params.find<std::string>("clock", "1GHz");
    clock_handler_ = new Clock::Handler<DoorbellTestCPU, &DoorbellTestCPU::clockTic>(this);
    clockTC_ = registerClock(clock_freq, clock_handler_);

    cache_link_ = loadUserSubComponent<StandardMem>(
        "cache_link", ComponentInfo::SHARE_NONE, clockTC_,
        new StandardMem::Handler<DoorbellTestCPU, &DoorbellTestCPU::handleCacheEvent>(this));
    mmio_link_ = loadUserSubComponent<StandardMem>(
        "mmio_link", ComponentInfo::SHARE_NONE, clockTC_,
        new StandardMem::Handler<DoorbellTestCPU, &DoorbellTestCPU::handleMmioEvent>(this));

    if (!cache_link_ || !mmio_link_) {
        out.fatal(CALL_INFO, -1, "DoorbellTestCPU requires cache_link and mmio_link subcomponents\n");
    }

    cache_handlers_ = new CacheHandlers(this, &out);
    mmio_handlers_ = new MmioHandlers(this, &out);

    stat_cache_writes_ = registerStatistic<uint64_t>("cache_writes");
    stat_mmio_writes_ = registerStatistic<uint64_t>("mmio_writes");
    stat_flush_count_ = registerStatistic<uint64_t>("flush_count");
    stat_cuda_calls_completed_ = registerStatistic<uint64_t>("cuda_calls_completed");
    stat_total_memD2H_bytes_ = registerStatistic<uint64_t>("total_memD2H_bytes");
    stat_correct_memD2H_bytes_ = registerStatistic<uint64_t>("correct_memD2H_bytes");
    stat_correct_memD2H_ratio_ = registerStatistic<double>("correct_memD2H_ratio");

    enable_memcpy_dump_ = params.find<bool>("enable_memcpy_dump", false);
    issued_initial_packet_ = false;
    calls_completed_ = 0;
    packet_issue_active_ = false;
    scratch_append_payload_.clear();
    trace_parser_ = nullptr;

    cuda_executable_ = params.find<std::string>("cuda_executable", found);
    if (mode_ == DriverMode::TRACE) {
        if (!found) {
            out.fatal(CALL_INFO, -1, "cuda_executable required in trace mode\n");
        }
        std::string trace_file = params.find<std::string>("trace_file", "cuda_calls.trace");
        trace_parser_ = new CudaAPITraceParser(this, &out, trace_file, cuda_executable_);
    } else if (!found) {
        cuda_executable_ = "./balar_trace/vectorAdd";
    }
}

void DoorbellTestCPU::init(unsigned int phase)
{
    cache_link_->init(phase);
    mmio_link_->init(phase);
}

void DoorbellTestCPU::setup()
{
    cache_link_->setup();
    mmio_link_->setup();
}

void DoorbellTestCPU::finish() {}

void DoorbellTestCPU::emergencyShutdown()
{
    if (out.getVerboseLevel() > 1) {
        out.output("DoorbellTestCPU %s outstanding=%zu\n", getName().c_str(), requests_.size());
    }
}

void DoorbellTestCPU::handleCacheEvent(StandardMem::Request* req) { req->handle(cache_handlers_); }
void DoorbellTestCPU::handleMmioEvent(StandardMem::Request* req) { req->handle(mmio_handlers_); }

void DoorbellTestCPU::uint64ToData(uint64_t num, std::vector<uint8_t>* data)
{
    data->clear();
    for (size_t i = 0; i < sizeof(uint64_t); i++) {
        data->push_back(num & 0xFF);
        num >>= 8;
    }
}

uint64_t DoorbellTestCPU::dataToUInt64(std::vector<uint8_t>* data)
{
    uint64_t retval = 0;
    for (int i = (int)data->size() - 1; i >= 0; i--) {
        retval <<= 8;
        retval |= (*data)[i];
    }
    return retval;
}

void DoorbellTestCPU::beginPacketIssue(const BalarCudaCallPacket_t& pack)
{
    BalarCudaCallPacket_t pack_copy = pack;
    vector<uint8_t>* encoded = encode_balar_packet<BalarCudaCallPacket_t>(&pack_copy);
    packet_buffer_.assign(encoded->begin(), encoded->end());
    delete encoded;
    if (!scratch_append_payload_.empty()) {
        packet_buffer_.insert(packet_buffer_.end(), scratch_append_payload_.begin(), scratch_append_payload_.end());
        scratch_append_payload_.clear();
    }

    scratch_write_offset_ = 0;
    scratch_bytes_remaining_ = packet_buffer_.size();
    packet_issue_active_ = true;

    out.verbose(_INFO_, "%s: issue packet %s (%zu bytes) to scratch 0x%lx\n", getName().c_str(),
        CudaAPIEnumToString(pack.cuda_call_id), packet_buffer_.size(), scratchMemAddr_);
    sendNextScratchChunk();
}

void DoorbellTestCPU::sendNextScratchChunk()
{
    if (scratch_bytes_remaining_ == 0) {
        return;
    }
    size_t chunk = std::min(scratch_bytes_remaining_, cacheLineSize_);
    std::vector<uint8_t> payload(packet_buffer_.begin() + scratch_write_offset_,
        packet_buffer_.begin() + scratch_write_offset_ + chunk);
    auto* req = new StandardMem::Write(scratchMemAddr_ + scratch_write_offset_, chunk, payload, false);
    requests_[req->getID()] = std::make_pair("ScratchWrite", IfacePath::CACHE);
    stat_cache_writes_->addData(1);
    cache_link_->send(req);
    scratch_write_offset_ += chunk;
    scratch_bytes_remaining_ -= chunk;
}

void DoorbellTestCPU::sendNextFlush()
{
    if (flushes_remaining_ == 0) {
        sendDoorbell();
        return;
    }
    StandardMem::Addr line_addr = scratchMemAddr_ + (flushes_outstanding_ * cacheLineSize_);
    auto* req = new StandardMem::FlushAddr(line_addr, cacheLineSize_, true, 1);
    requests_[req->getID()] = std::make_pair("ScratchFlush", IfacePath::CACHE);
    stat_flush_count_->addData(1);
    cache_link_->send(req);
    flushes_outstanding_++;
}

void DoorbellTestCPU::sendDoorbell()
{
    std::vector<uint8_t> payload;
    uint64ToData(scratchMemAddr_, &payload);
    auto* req = new StandardMem::Write(mmioAddr_, payload.size(), payload, false);
    requests_[req->getID()] = std::make_pair("Doorbell", IfacePath::MMIO);
    stat_mmio_writes_->addData(1);
    mmio_link_->send(req);
}

void DoorbellTestCPU::sendStartCudaRetRead()
{
    auto* req = new StandardMem::Read(mmioAddr_, sizeof(uint64_t));
    requests_[req->getID()] = std::make_pair("Start_CUDA_ret", IfacePath::MMIO);
    mmio_link_->send(req);
}

void DoorbellTestCPU::sendReadRetPacket(uint64_t ret_addr)
{
    auto* req = new StandardMem::Read(ret_addr, sizeof(BalarCudaCallReturnPacket_t));
    requests_[req->getID()] = std::make_pair("Read_CUDA_ret_packet", IfacePath::CACHE);
    cache_link_->send(req);
}

void DoorbellTestCPU::onCacheWriteReq(StandardMem::Write* write)
{
    if (!write->posted) {
        cache_link_->send(write->makeResponse());
    }
    delete write;
}

void DoorbellTestCPU::onCacheWriteResp(StandardMem::WriteResp* resp)
{
    auto it = requests_.find(resp->getID());
    if (it == requests_.end()) {
        out.fatal(CALL_INFO, -1, "Unknown cache WriteResp %ld\n", resp->getID());
    }
    if (it->second.first == "ScratchWrite") {
        if (scratch_bytes_remaining_ > 0) {
            sendNextScratchChunk();
        } else {
            size_t packet_size = packet_buffer_.size();
            flushes_remaining_ = (packet_size + cacheLineSize_ - 1) / cacheLineSize_;
            flushes_outstanding_ = 0;
            sendNextFlush();
        }
    } else {
        out.fatal(CALL_INFO, -1, "Unexpected cache WriteResp type %s\n", it->second.first.c_str());
    }
    requests_.erase(it);
    delete resp;
}

void DoorbellTestCPU::onCacheFlushResp(StandardMem::FlushResp* resp)
{
    auto it = requests_.find(resp->getID());
    if (it == requests_.end()) {
        out.fatal(CALL_INFO, -1, "Unknown cache FlushResp %ld\n", resp->getID());
    }
    if (flushes_remaining_ > 0) {
        flushes_remaining_--;
    }
    if (flushes_remaining_ > 0) {
        sendNextFlush();
    } else {
        sendDoorbell();
    }
    requests_.erase(it);
    delete resp;
}

void DoorbellTestCPU::onCacheReadResp(StandardMem::ReadResp* resp)
{
    auto it = requests_.find(resp->getID());
    if (it == requests_.end()) {
        out.fatal(CALL_INFO, -1, "Unknown cache ReadResp %ld\n", resp->getID());
    }
    if (it->second.first == "Read_CUDA_ret_packet") {
        auto* ret = decode_balar_packet<BalarCudaCallReturnPacket_t>(&(resp->data));
        completeCudaCall(ret);
        delete ret;
        packet_issue_active_ = false;
        ++calls_completed_;
        requests_.erase(it);
        tryFinishSimulation();
    } else {
        out.fatal(CALL_INFO, -1, "Unexpected cache ReadResp %s\n", it->second.first.c_str());
        requests_.erase(it);
    }
    delete resp;
}

void DoorbellTestCPU::onMmioWriteReq(StandardMem::Write* write)
{
    if (!write->posted) {
        mmio_link_->send(write->makeResponse());
    }
    delete write;
}

void DoorbellTestCPU::onMmioWriteResp(StandardMem::WriteResp* resp)
{
    auto it = requests_.find(resp->getID());
    if (it == requests_.end()) {
        out.fatal(CALL_INFO, -1, "Unknown mmio WriteResp %ld\n", resp->getID());
    }
    if (it->second.first == "Doorbell") {
        sendStartCudaRetRead();
    } else {
        out.fatal(CALL_INFO, -1, "Unexpected mmio WriteResp %s\n", it->second.first.c_str());
    }
    requests_.erase(it);
    delete resp;
}

void DoorbellTestCPU::onMmioReadResp(StandardMem::ReadResp* resp)
{
    auto it = requests_.find(resp->getID());
    if (it == requests_.end()) {
        out.fatal(CALL_INFO, -1, "Unknown mmio ReadResp %ld\n", resp->getID());
    }
    if (it->second.first == "Start_CUDA_ret") {
        uint64_t ret_addr = dataToUInt64(&(resp->data));
        sendReadRetPacket(ret_addr);
    } else {
        out.fatal(CALL_INFO, -1, "Unexpected mmio ReadResp %s\n", it->second.first.c_str());
    }
    requests_.erase(it);
    delete resp;
}

void DoorbellTestCPU::completeCudaCall(const BalarCudaCallReturnPacket_t* ret_pack)
{
    stat_cuda_calls_completed_->addData(1);
    if (ret_pack->cuda_call_id == CUDA_REG_FAT_BINARY && trace_parser_) {
        trace_parser_->setFatbinHandle(ret_pack->fat_cubin_handle);
    } else if (ret_pack->cuda_call_id == CUDA_MALLOC) {
        out.verbose(_INFO_, "cudaMalloc -> 0x%lx\n", ret_pack->cudamalloc.malloc_addr);
        if (ret_pack->cudamalloc.devptr_addr) {
            *(CUdeviceptr*)ret_pack->cudamalloc.devptr_addr = ret_pack->cudamalloc.malloc_addr;
        }
    } else if (ret_pack->cuda_call_id == CUDA_MEMCPY && ret_pack->cudamemcpy.kind == cudaMemcpyDeviceToHost) {
        size_t tot = ret_pack->cudamemcpy.size;
        size_t correct = 0;
        volatile uint8_t* sim_ptr = ret_pack->cudamemcpy.sim_data;
        volatile uint8_t* real_ptr = ret_pack->cudamemcpy.real_data;
        if (sim_ptr && real_ptr) {
            for (size_t i = 0; i < tot; i++) {
                if (real_ptr[i] == sim_ptr[i]) {
                    correct++;
                }
            }
            stat_total_memD2H_bytes_->addData(tot);
            stat_correct_memD2H_bytes_->addData(correct);
            stat_correct_memD2H_ratio_->addData(tot ? (double)correct / (double)tot : 1.0);

            // Optional: dump the GPU result (and reference) so a host tool can
            // do a tolerance comparison for non-bit-exact (e.g. tone) inputs.
            if (enable_memcpy_dump_) {
                char buf[200];
                snprintf(buf, sizeof(buf), "cudamemcpyD2H-sim-%p-%p-size-%zu.data",
                         (void*)sim_ptr, (void*)real_ptr, tot);
                std::ofstream simDump(buf, std::ios::out | std::ios::binary);
                if (!simDump.is_open()) {
                    out.fatal(CALL_INFO, -1, "Cannot open '%s' for D2H dump\n", buf);
                }
                simDump.write((const char*)sim_ptr, tot);

                snprintf(buf, sizeof(buf), "cudamemcpyD2H-real-%p-%p-size-%zu.data",
                         (void*)sim_ptr, (void*)real_ptr, tot);
                std::ofstream realDump(buf, std::ios::out | std::ios::binary);
                if (!realDump.is_open()) {
                    out.fatal(CALL_INFO, -1, "Cannot open '%s' for D2H dump\n", buf);
                }
                realDump.write((const char*)real_ptr, tot);
            }
        }
    }
}

void DoorbellTestCPU::tryFinishSimulation()
{
    if (mode_ == DriverMode::DOORBELL_ONLY) {
        if (!packet_issue_active_ && requests_.empty() && calls_completed_ >= 1) {
            out.verbose(CALL_INFO, 1, 0, "DoorbellTestCPU: Test Completed Successfuly\n");
            primaryComponentOKToEndSim();
        }
        return;
    }
    if (!packet_issue_active_ && requests_.empty() && trace_parser_) {
        if (!trace_parser_->hasNextPacket()) {
            out.verbose(CALL_INFO, 1, 0, "DoorbellTestCPU: Test Completed Successfuly\n");
            primaryComponentOKToEndSim();
        }
    }
}

bool DoorbellTestCPU::clockTic(Cycle_t)
{
    if (mode_ == DriverMode::DOORBELL_ONLY) {
        // Issue the initial fatbin exactly once, the first time the CPU is idle.
        if (!issued_initial_packet_ && requests_.empty() && !packet_issue_active_) {
            BalarCudaCallPacket_t fatbin{};
            fatbin.cuda_call_id = CUDA_REG_FAT_BINARY;
            fatbin.isSSTmem = false;
            strncpy(fatbin.register_fatbin.file_name, cuda_executable_.c_str(), BALAR_CUDA_MAX_FILE_NAME - 1);
            fatbin.register_fatbin.file_name[BALAR_CUDA_MAX_FILE_NAME - 1] = '\0';
            issued_initial_packet_ = true;
            beginPacketIssue(fatbin);
        }
        return false;
    }

    if (!packet_issue_active_ && requests_.empty() && trace_parser_) {
        BalarCudaCallPacket_t pack{};
        if (trace_parser_->getNextPacket(pack)) {
            beginPacketIssue(pack);
        } else {
            out.verbose(CALL_INFO, 1, 0, "DoorbellTestCPU: Test Completed Successfuly\n");
            primaryComponentOKToEndSim();
            return true;
        }
    }

    tryFinishSimulation();
    return false;
}
