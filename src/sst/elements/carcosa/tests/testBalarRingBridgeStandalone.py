#!/usr/bin/env python3
"""Compile the real bridge/parser with a small simulated SST/CUDA transport.

Run directly with Python 3; only a C++17 compiler is required. CUDA types below
are ABI stand-ins, deliberately making return packets larger than commands.
The test models a write-back cache and Balar's command/return DMA protocol.
"""
from pathlib import Path
import os
import shlex
import subprocess
import tempfile


SUPPORT = r'''
#ifndef BALAR_BRIDGE_TEST_SUPPORT_H
#define BALAR_BRIDGE_TEST_SUPPORT_H
#include <algorithm>
#include <cassert>
#include <cinttypes>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <deque>
#include <fstream>
#include <functional>
#include <limits>
#include <map>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#define CALL_INFO 0
#define SST_ELI_REGISTER_SUBCOMPONENT(...)
#define SST_ELI_DOCUMENT_PARAMS(...)
#define SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(...)
using cudaStream_t = void*;
using cudaEvent_t = void*;
using CUdeviceptr = uint64_t;
enum cudaMemcpyKind { cudaMemcpyHostToDevice = 1, cudaMemcpyDeviceToHost = 2 };
enum cudaError_t { cudaSuccess = 0, cudaErrorNotReady = 34, cudaErrorUnknown = 999 };
enum cudaDeviceAttr { testAttribute };
struct textureReference { uint8_t unused[64]; };
struct cudaChannelFormatDesc { int unused[5]; };
struct cudaDeviceProp { uint8_t unused[800]; };
namespace SST {
using ComponentId_t = uint64_t;
using TimeConverter = int;
struct ComponentInfo { static constexpr int SHARE_NONE = 0; };
class Output {
public:
    static constexpr int STDOUT = 0;
    Output(const char*, int, int, int) {}
    [[noreturn]] void fatal(int, int, const char* format, ...) {
        char text[1024]; va_list args; va_start(args, format);
        vsnprintf(text, sizeof(text), format, args); va_end(args);
        throw std::runtime_error(text);
    }
    template<class... Args> void output(Args...) {}
};
class Event {
public:
    virtual ~Event() = default;
    template<class T, void (T::*Method)(Event*)> struct Handler {
        T* target;
        explicit Handler(T* t) : target(t) {}
        void operator()(Event* event) { (target->*Method)(event); }
    };
};
class Link {
public:
    std::deque<Event*> pending;
    std::function<void(Event*)> deliver;
    void send(Event* event) { pending.push_back(event); }
    ~Link() { for (auto* event : pending) delete event; }
};
class Params {
public:
    std::map<std::string, std::string> values;
    template<class T> T find(const std::string& key, T fallback) {
        auto it = values.find(key); if (it == values.end()) return fallback;
        if constexpr (std::is_same_v<T, std::string>) return it->second;
        else return static_cast<T>(std::stoull(it->second, nullptr, 0));
    }
    template<class T> T find(const std::string& key, bool& found) {
        found = values.count(key); return found ? T(values.at(key)) : T{};
    }
};
namespace Interfaces {
class StandardMem {
public:
    using Addr = uint64_t;
    struct ReadResp; struct WriteResp; struct FlushResp;
    struct RequestHandler {
        explicit RequestHandler(Output*) {} virtual ~RequestHandler() = default;
        virtual void handle(ReadResp*) {} virtual void handle(WriteResp*) {}
        virtual void handle(FlushResp*) {}
    };
    struct Request {
        using id_t = uint64_t;
        inline static id_t next = 0;
        id_t id; Addr pAddr; size_t size;
        Request(Addr addr, size_t n) : id(++next), pAddr(addr), size(n) {}
        virtual ~Request() = default;
        id_t getID() { return id; }
        virtual void handle(RequestHandler*) {}
    };
    struct Read : Request { Read(Addr a, size_t n) : Request(a,n) {} };
    struct Write : Request {
        std::vector<uint8_t> data;
        Write(Addr a, size_t n, const std::vector<uint8_t>& d, bool) : Request(a,n), data(d) {}
    };
    struct FlushAddr : Request {
        bool inv;
        FlushAddr(Addr a, size_t n, bool invalidate, int) : Request(a,n), inv(invalidate) {}
    };
    struct ReadResp : Request {
        std::vector<uint8_t> data;
        ReadResp(Request* r, std::vector<uint8_t> d) : Request(r->pAddr,r->size), data(std::move(d)) { id=r->id; }
        void handle(RequestHandler* h) override { h->handle(this); }
    };
    struct WriteResp : Request {
        explicit WriteResp(Request* r) : Request(r->pAddr,r->size) { id=r->id; }
        void handle(RequestHandler* h) override { h->handle(this); }
    };
    struct FlushResp : Request {
        explicit FlushResp(Request* r) : Request(r->pAddr,r->size) { id=r->id; }
        void handle(RequestHandler* h) override { h->handle(this); }
    };
    template<class T, void(T::*Method)(Request*)> struct Handler { explicit Handler(T*) {} };
    std::deque<Request*> pending;
    void send(Request* request) { pending.push_back(request); }
    void init(unsigned) {} void setup() {}
    ~StandardMem() { for (auto* request : pending) delete request; }
};
}
namespace MemHierarchy { class MemEvent {}; }
namespace Carcosa {
class HaliEvent : public Event {
    std::string text; unsigned num;
public:
    HaliEvent(std::string s, unsigned n) : text(std::move(s)), num(n) {}
    std::string getStr() { return text; } unsigned getNum() { return num; }
};
class InterceptionAgentAPI {
    std::vector<Link*> owned_links;
    std::vector<Interfaces::StandardMem*> owned_mem;
public:
    InterceptionAgentAPI() = default;
    InterceptionAgentAPI(ComponentId_t, Params&) {}
    virtual ~InterceptionAgentAPI() {
        for (auto* link : owned_links) delete link;
        for (auto* mem : owned_mem) delete mem;
    }
    virtual bool handleInterceptedEvent(MemHierarchy::MemEvent*, Link*) = 0;
    virtual void handleRingEvent(HaliEvent*) {} virtual void setRingLink(Link*) {}
    virtual void agentInit(unsigned) {} virtual void agentSetup() {}
    TimeConverter getTimeConverter(const std::string&) { return 1; }
    template<class H> Link* configureSelfLink(const char*, const char*, H* handler) {
        auto* link = new Link;
        link->deliver = [copy=*handler](Event* event) mutable { copy(event); };
        delete handler; owned_links.push_back(link); return link;
    }
    template<class T, class H> T* loadUserSubComponent(const char*, int, TimeConverter, H* handler) {
        delete handler; auto* mem = new T; owned_mem.push_back(mem); return mem;
    }
};
}
}
#endif
'''

HARNESS = r'''
#include "support.h"
// Expose state only to this standalone harness; compile the production code as is.
#define private public
#include <sst/elements/carcosa/components/balarRingBridge.h>
#undef private
#include <sst/elements/carcosa/components/balarRingBridge.cc>

using namespace SST;
using namespace SST::Carcosa;
using namespace SST::BalarComponent;
using Mem = SST::Interfaces::StandardMem;

void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }

struct Transport {
    BalarRingBridge& bridge;
    std::vector<uint8_t> memory = std::vector<uint8_t>(1 << 16);
    struct Line { std::vector<uint8_t> data; bool dirty = false; };
    std::map<uint64_t, Line> cache;
    BalarCudaCallReturnPacket_t result{};
    std::map<int, unsigned> rejected, calls;
    std::map<int, std::vector<uint8_t>> rejected_packets;
    std::vector<uint8_t> gpu, output;
    size_t weight_bytes_written = 0, assembled_returns = 0, retries = 0;
    bool saw_partial_line = false;

    explicit Transport(BalarRingBridge& b) : bridge(b) {}
    Line& line(uint64_t address) {
        uint64_t base = address - address % bridge.cache_line_size_;
        auto [it, fresh] = cache.try_emplace(base);
        if (fresh) it->second.data.assign(memory.begin()+base, memory.begin()+base+bridge.cache_line_size_);
        return it->second;
    }
    void stepCache() {
        auto* req = bridge.cache_link_->pending.front(); bridge.cache_link_->pending.pop_front();
        size_t width = bridge.cache_line_size_, offset = req->pAddr % width;
        require(req->size && offset + req->size <= width, "cache request crosses a line");
        if (auto* write = dynamic_cast<Mem::Write*>(req)) {
            auto& entry = line(req->pAddr);
            std::copy(write->data.begin(),write->data.end(),entry.data.begin()+offset);
            entry.dirty = true;
            if (req->pAddr >= bridge.weight_stage_addr_) weight_bytes_written += req->size;
            bridge.handleCacheEvent(new Mem::WriteResp(req));
        } else if (auto* flush = dynamic_cast<Mem::FlushAddr*>(req)) {
            require(offset == 0 && req->size == width && flush->inv, "flush is not an aligned invalidation");
            auto found = cache.find(req->pAddr);
            if (found != cache.end()) {
                if (found->second.dirty)
                    std::copy(found->second.data.begin(),found->second.data.end(),memory.begin()+req->pAddr);
                cache.erase(found);
            }
            bridge.handleCacheEvent(new Mem::FlushResp(req));
        } else if (dynamic_cast<Mem::Read*>(req)) {
            std::string tag = bridge.requests_.at(req->id).first;
            auto& entry = line(req->pAddr);
            std::vector<uint8_t> bytes(entry.data.begin()+offset,entry.data.begin()+offset+req->size);
            bool final_return = tag == "Read_CUDA_ret_packet" &&
                bridge.ret_packet_data_.size()+req->size == sizeof(result);
            if (offset) saw_partial_line = true;
            bridge.handleCacheEvent(new Mem::ReadResp(req, std::move(bytes)));
            if (final_return) {
                require(bridge.ret_packet_data_.size() == sizeof(result), "incomplete return assembly");
                require(memcmp(bridge.ret_packet_data_.data(), &result, sizeof(result)) == 0,
                        "return packet contains stale cache bytes");
                ++assembled_returns;
            }
        } else throw std::runtime_error("unknown cache request");
        delete req;
    }
    void stepMmio() {
        auto* req = bridge.mmio_link_->pending.front(); bridge.mmio_link_->pending.pop_front();
        if (dynamic_cast<Mem::Write*>(req)) {
            BalarCudaCallPacket_t packet;
            memcpy(&packet, memory.data()+bridge.scratch_mem_addr_, sizeof(packet));
            int key = packet.cuda_call_id == CUDA_MEMCPY ? 100 + packet.cuda_memcpy.kind : packet.cuda_call_id;
            ++calls[key];
            auto bytes = std::vector<uint8_t>(memory.begin()+bridge.scratch_mem_addr_,
                                             memory.begin()+bridge.scratch_mem_addr_+sizeof(packet));
            auto pending = rejected_packets.find(key);
            if (pending != rejected_packets.end()) require(pending->second == bytes, "retry changed the active packet");
            // Poison even the inactive union bytes: failed calls must not consume
            // them, and full return assembly must invalidate them on every call.
            memset(&result, static_cast<int>(assembled_returns + 1), sizeof(result));
            result.cuda_call_id = packet.cuda_call_id;
            result.cuda_error = cudaSuccess; result.is_cuda_call_done = true;
            bool rejectable = key == CUDA_LAUNCH || key == 101 || key == 102;
            if (rejectable && rejected[key] < 2) {
                ++rejected[key]; rejected_packets[key] = bytes;
                result.cuda_error = cudaErrorNotReady;
                result.is_cuda_call_done = false;
            } else {
                rejected_packets.erase(key);
                if (packet.cuda_call_id == CUDA_REG_FAT_BINARY) result.fat_cubin_handle = 123;
                else if (packet.cuda_call_id == CUDA_MALLOC) {
                    result.cudamalloc.devptr_addr = reinterpret_cast<uint64_t>(packet.cuda_malloc.devPtr);
                    result.cudamalloc.malloc_addr = 0x100000;
                } else if (key == 101) {
                    gpu.assign(memory.begin()+packet.cuda_memcpy.src,
                               memory.begin()+packet.cuda_memcpy.src+packet.cuda_memcpy.count);
                    result.cudamemcpy.kind = cudaMemcpyHostToDevice;
                } else if (key == 102) {
                    require(packet.isSSTmem, "expected SST D2H");
                    size_t reserved = std::max(sizeof(packet), sizeof(result));
                    require(packet.cuda_memcpy.dst >= bridge.scratch_mem_addr_ + reserved,
                            "D2H overlaps command/return scratch");
                    require(packet.cuda_memcpy.dst % bridge.cache_line_size_ == 0, "unaligned D2H");
                    output = gpu; for (auto& byte : output) byte ^= 0xa5;
                    std::copy(output.begin(),output.end(),memory.begin()+packet.cuda_memcpy.dst);
                    result.cudamemcpy.sim_data = nullptr;
                    result.cudamemcpy.kind = cudaMemcpyDeviceToHost;
                    result.cudamemcpy.size = output.size();
                }
            }
            bridge.handleMmioEvent(new Mem::WriteResp(req));
        } else if (dynamic_cast<Mem::Read*>(req)) {
            // Balar completes D2H DMA first, then writes its return packet to scratch.
            memcpy(memory.data()+bridge.scratch_mem_addr_, &result, sizeof(result));
            std::vector<uint8_t> address;
            BalarRingBridge::uint64ToData(bridge.scratch_mem_addr_, &address);
            bridge.handleMmioEvent(new Mem::ReadResp(req, std::move(address)));
        } else throw std::runtime_error("unknown MMIO request");
        delete req;
    }
    void drain() {
        size_t steps = 0;
        while (!bridge.cache_link_->pending.empty() || !bridge.mmio_link_->pending.empty() ||
               !bridge.retry_link_->pending.empty()) {
            require(++steps < 10000, "bridge never completed");
            if (!bridge.cache_link_->pending.empty()) stepCache();
            else if (!bridge.mmio_link_->pending.empty()) stepMmio();
            else {
                require(bridge.packet_issue_active_ && bridge.replay_active_, "rejected call advanced the trace");
                require(bridge.pending_d2h_read_bytes_ == 0, "rejected D2H started readback");
                auto* event = bridge.retry_link_->pending.front(); bridge.retry_link_->pending.pop_front();
                ++retries; bridge.retry_link_->deliver(event);
            }
        }
    }
};

int main() {
    try {
        static_assert(sizeof(BalarCudaCallReturnPacket_t) > sizeof(BalarCudaCallPacket_t));
        std::ofstream trace("calls.trace");
        trace << "memalloc: dptr:dptrA, size:64\n"
              << "memcpyH2D: device_ptr:dptrA, size:64, data_file:weights.bin\n"
              << "kernel launch: name:gemm, ptx_name:gemm, gdx:1, gdy:1, gdz:1, bdx:1, bdy:1, bdz:1, sharedBytes:0, args:dptrA/8/\n"
              << "memcpyD2H: device_ptr:dptrA, size:64\n"
              << "free: dptr:dptrA\n";
        trace.close();
        std::ofstream weights("weights.bin", std::ios::binary);
        for (unsigned char i=0; i<64; ++i) weights.put(i);
        weights.close();
        Params params;
        params.values = {{"cuda_executable","test"}, {"trace_file","calls.trace"},
                         {"scratch_mem_addr","3"}, {"weight_stage_addr","16387"},
                         {"replay_each_cmd","1"}};
        BalarRingBridge bridge(1, params);
        Link done; bridge.setRingLink(&done); bridge.agentSetup();
        Transport transport(bridge);
        std::vector<uint8_t> expected(64);
        for (size_t i=0; i<expected.size(); ++i) expected[i] = static_cast<uint8_t>(i) ^ 0xa5;
        HaliEvent cmd(RingTag::Cmd, 0);
        for (size_t replay=1; replay<=2; ++replay) {
            bridge.handleRingEvent(&cmd); transport.drain();
            require(!bridge.replay_active_ && bridge.replays_ == replay, "replay not completed");
            require(done.pending.size() == replay, "wrong Done count");
            require(bridge.requests_.empty(), "unhandled requests remain");
            require(transport.weight_bytes_written == 64*replay, "retry restaged H2D weights");
            require(transport.output == expected, "GPU copy was skipped or corrupted");
            require(bridge.checksum_ == fnv1a64(expected.data(),expected.size()),
                    "checksum did not fold the complete GPU result");
        }
        require(transport.retries == 6 && transport.saw_partial_line, "retry/unaligned coverage missing");
        require(transport.rejected_packets.empty(), "a rejected call was skipped");

        // Fatal CUDA errors must be checked before dereferencing union pointers.
        bridge.active_packet_.cuda_call_id = CUDA_MALLOC;
        BalarCudaCallReturnPacket_t failure{};
        failure.cuda_call_id = CUDA_MALLOC; failure.cuda_error = cudaErrorUnknown;
        failure.cudamalloc.devptr_addr = 1;
        bool failed = false;
        try { bridge.completeCudaCall(&failure); }
        catch (const std::runtime_error& e) { failed = std::string(e.what()).find("CUDA error 999") != std::string::npos; }
        require(failed, "CUDA failure was not reported before union access");
        std::printf("PASS: packet assembly, nonoverlapping D2H, deferred retries, cache invalidation, CUDA failures (%zu returns)\n",
                    transport.assembled_returns);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "FAIL: %s\n", e.what()); return 1;
    }
}
'''


def main():
    source_root = Path(__file__).resolve().parents[4]
    with tempfile.TemporaryDirectory(prefix="carcosa-balar-test-") as directory:
        root = Path(directory)
        (root / "support.h").write_text(SUPPORT)
        for name in (
            "sst_config.h", "builtin_types.h", "driver_types.h", "cuda.h",
            "sst/core/output.h", "sst/core/link.h", "sst/core/interfaces/stdMem.h",
            "sst/elements/carcosa/components/interceptionAgentAPI.h",
            "sst/elements/carcosa/components/haliEvent.h",
            "sst/elements/memHierarchy/memEvent.h",
        ):
            path = root / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text('#include "support.h"\n')
        (root / "test.cc").write_text(HARNESS)
        compiler = shlex.split(os.environ.get("CXX", "c++"))
        subprocess.run(compiler + ["-std=c++17", "-DHAVE_BALAR_BRIDGE=1", "-I" + str(root),
                                  "-I" + str(source_root), str(root / "test.cc"),
                                  str(source_root / "sst/elements/carcosa/components/balarTraceParser.cc"),
                                  "-o", str(root / "test")], check=True)
        subprocess.run([str(root / "test")], cwd=root, check=True)


if __name__ == "__main__":
    main()
