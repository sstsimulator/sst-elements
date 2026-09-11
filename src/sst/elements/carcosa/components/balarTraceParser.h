// Copyright 2009-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.

#ifndef CARCOSA_BALAR_TRACE_PARSER_H
#define CARCOSA_BALAR_TRACE_PARSER_H

#ifdef HAVE_BALAR_BRIDGE

#include <sst/core/output.h>
#include <sst/elements/balar/balar_packet.h>

#include "cuda.h"

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <map>
#include <queue>
#include <string>
#include <vector>

namespace SST::Carcosa {

struct BalarTracePacket {
    SST::BalarComponent::BalarCudaCallPacket_t packet{};
    std::vector<uint8_t> h2d_payload;
    size_t d2h_bytes = 0;
    bool is_h2d = false;
    bool is_d2h = false;
};

class BalarTraceParser {
public:
    BalarTraceParser(SST::Output* out, std::string trace_file,
                     std::string cuda_executable);

    void rewind();
    bool next(BalarTracePacket& result);
    void setFatbinHandle(uint64_t handle);

private:
    bool parseLine(const std::string& line, BalarTracePacket& result);

    SST::Output* out_;
    std::string trace_file_;
    std::string trace_base_path_;
    std::string cuda_executable_;
    std::ifstream trace_stream_;
    std::queue<SST::BalarComponent::BalarCudaCallPacket_t> queued_packets_;
    std::map<std::string, CUdeviceptr> device_ptrs_;
    std::map<std::string, uint64_t> functions_;
    uint64_t fatbin_handle_ = 0;
    size_t line_number_ = 0;
    bool fatbin_registered_ = false;
};

} // namespace SST::Carcosa

#endif
#endif
