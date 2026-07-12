// Copyright 2009-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.

#include "sst_config.h"

#ifdef HAVE_BALAR_BRIDGE

#include "sst/elements/carcosa/components/balarTraceParser.h"
#include "sst/elements/carcosa/components/configParse.h"

#include <algorithm>
#include <climits>
#include <cstring>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <utility>

using namespace SST;
using namespace SST::BalarComponent;
using namespace SST::Carcosa;

namespace {

std::string trim(const std::string& value)
{
    size_t first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    size_t last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

std::vector<std::string> split(const std::string& value, char delimiter)
{
    std::vector<std::string> result;
    std::stringstream stream(value);
    std::string part;
    while (std::getline(stream, part, delimiter)) result.push_back(trim(part));
    return result;
}

std::map<std::string, std::string> parseParams(const std::string& value)
{
    std::map<std::string, std::string> result;
    for (const auto& part : split(value, ',')) {
        size_t delimiter = part.find(':');
        if (delimiter != std::string::npos) {
            result[trim(part.substr(0, delimiter))] = trim(part.substr(delimiter + 1));
        }
    }
    return result;
}

const std::string& requireParam(const std::map<std::string, std::string>& params,
                                const std::string& key, Output* out,
                                size_t line_number, bool allow_empty = false)
{
    auto found = params.find(key);
    if (found == params.end() || (!allow_empty && found->second.empty())) {
        out->fatal(CALL_INFO, -1,
                   "Balar trace line %zu: missing parameter '%s'\n",
                   line_number, key.c_str());
    }
    return found->second;
}

template <typename T>
T parseInteger(const std::string& value, const char* field, Output* out,
               size_t line_number)
{
    try {
        if (value.empty() || value.front() == '-') throw std::invalid_argument(value);
        size_t parsed = 0;
        unsigned long long raw = std::stoull(value, &parsed, 0);
        if (parsed != value.size() ||
            raw > static_cast<unsigned long long>(std::numeric_limits<T>::max())) {
            out->fatal(CALL_INFO, -1,
                       "Balar trace line %zu: invalid %s '%s'\n",
                       line_number, field, value.c_str());
        }
        return static_cast<T>(raw);
    } catch (...) {
        out->fatal(CALL_INFO, -1,
                   "Balar trace line %zu: invalid %s '%s'\n",
                   line_number, field, value.c_str());
    }
    return {};
}

int parseSignedInteger(const std::string& value, const char* field, Output* out,
                       size_t line_number)
{
    try {
        size_t parsed = 0;
        long long raw = std::stoll(value, &parsed, 0);
        if (parsed == value.size() && raw >= INT_MIN && raw <= INT_MAX) {
            return static_cast<int>(raw);
        }
    } catch (...) {
    }
    out->fatal(CALL_INFO, -1, "Balar trace line %zu: invalid %s '%s'\n",
               line_number, field, value.c_str());
    return 0;
}

} // namespace

BalarTraceParser::BalarTraceParser(Output* out, std::string trace_file,
                                   std::string cuda_executable)
    : out_(out), trace_file_(std::move(trace_file)),
      cuda_executable_(std::move(cuda_executable))
{
    size_t separator = trace_file_.find_last_of('/');
    trace_base_path_ = separator == std::string::npos
        ? "./" : trace_file_.substr(0, separator + 1);
    rewind();
}

void BalarTraceParser::rewind()
{
    if (trace_stream_.is_open()) trace_stream_.close();
    trace_stream_.clear();
    trace_stream_.open(trace_file_);
    if (!trace_stream_.is_open()) {
        out_->fatal(CALL_INFO, -1, "Balar trace file '%s' does not exist\n",
                    trace_file_.c_str());
    }
    line_number_ = 0;
    std::queue<BalarCudaCallPacket_t> empty;
    queued_packets_.swap(empty);
    if (!fatbin_registered_) {
        BalarCudaCallPacket_t packet{};
        packet.cuda_call_id = CUDA_REG_FAT_BINARY;
        packet.isSSTmem = false;
        strncpy(packet.register_fatbin.file_name, cuda_executable_.c_str(),
                BALAR_CUDA_MAX_FILE_NAME - 1);
        packet.register_fatbin.file_name[BALAR_CUDA_MAX_FILE_NAME - 1] = '\0';
        queued_packets_.push(packet);
    }
}

bool BalarTraceParser::next(BalarTracePacket& result)
{
    result = {};
    if (!queued_packets_.empty()) {
        result.packet = queued_packets_.front();
        queued_packets_.pop();
        return true;
    }

    std::string line;
    while (std::getline(trace_stream_, line)) {
        ++line_number_;
        line = trim(line);
        if (line.empty() || line.front() == '#') continue;
        if (parseLine(line, result)) return true;
    }
    if (!trace_stream_.eof()) {
        out_->fatal(CALL_INFO, -1, "Balar trace read failed near line %zu\n",
                    line_number_ + 1);
    }
    return false;
}

bool BalarTraceParser::parseLine(const std::string& line,
                                 BalarTracePacket& result)
{
    size_t delimiter = line.find(':');
    if (delimiter == std::string::npos) {
        out_->fatal(CALL_INFO, -1,
                    "Balar trace line %zu: expected '<call>: <params>'\n",
                    line_number_);
    }

    std::string call = trim(line.substr(0, delimiter));
    auto params = parseParams(line.substr(delimiter + 1));
    auto& packet = result.packet;
    packet = {};
    packet.isSSTmem = false;

    if (call.find("memalloc") != std::string::npos) {
        packet.cuda_call_id = CUDA_MALLOC;
        const auto& name = requireParam(params, "dptr", out_, line_number_);
        auto slot = device_ptrs_.emplace(name, CUdeviceptr{}).first;
        packet.cuda_malloc.devPtr = reinterpret_cast<void**>(&slot->second);
        packet.cuda_malloc.size = parseInteger<size_t>(
            requireParam(params, "size", out_, line_number_), "size", out_, line_number_);
        return true;
    }

    bool h2d = call.find("memcpyH2D") != std::string::npos;
    bool d2h = call.find("memcpyD2H") != std::string::npos;
    if (h2d || d2h) {
        packet.cuda_call_id = CUDA_MEMCPY;
        const auto& name = requireParam(params, "device_ptr", out_, line_number_);
        auto device = device_ptrs_.find(name);
        if (device == device_ptrs_.end()) {
            out_->fatal(CALL_INFO, -1,
                        "Balar trace line %zu: unknown device pointer '%s'\n",
                        line_number_, name.c_str());
        }
        size_t size = parseInteger<size_t>(
            requireParam(params, "size", out_, line_number_), "size", out_, line_number_);
        packet.cuda_memcpy.count = size;
        packet.cuda_memcpy.payload = 0;
        if (h2d) {
            result.is_h2d = true;
            packet.cuda_memcpy.kind = cudaMemcpyHostToDevice;
            packet.cuda_memcpy.dst = device->second;
            std::string path = trace_base_path_ +
                requireParam(params, "data_file", out_, line_number_);
            std::ifstream data(path, std::ios::binary);
            if (!data.is_open()) {
                out_->fatal(CALL_INFO, -1,
                            "Balar trace line %zu: data file '%s' does not exist\n",
                            line_number_, path.c_str());
            }
            result.h2d_payload.resize(size);
            if (size > 0) {
                data.read(reinterpret_cast<char*>(result.h2d_payload.data()), size);
                if (data.gcount() != static_cast<std::streamsize>(size)) {
                    out_->fatal(CALL_INFO, -1,
                                "Balar trace line %zu: data file '%s' is shorter than %zu bytes\n",
                                line_number_, path.c_str(), size);
                }
            }
        } else {
            packet.cuda_memcpy.kind = cudaMemcpyDeviceToHost;
            packet.cuda_memcpy.src = device->second;
            result.is_d2h = true;
            result.d2h_bytes = size;
        }
        return true;
    }

    if (call.find("kernel launch") != std::string::npos) {
        const auto& function_name = requireParam(params, "name", out_, line_number_);
        const auto& ptx_name = requireParam(params, "ptx_name", out_, line_number_);

        BalarCudaCallPacket_t configure{}, argument{}, launch{}, registration{};
        configure.cuda_call_id = CUDA_CONFIG_CALL;
        argument.cuda_call_id = CUDA_SET_ARG;
        launch.cuda_call_id = CUDA_LAUNCH;
        configure.configure_call.gdx = parseInteger<unsigned>(requireParam(params, "gdx", out_, line_number_), "gdx", out_, line_number_);
        configure.configure_call.gdy = parseInteger<unsigned>(requireParam(params, "gdy", out_, line_number_), "gdy", out_, line_number_);
        configure.configure_call.gdz = parseInteger<unsigned>(requireParam(params, "gdz", out_, line_number_), "gdz", out_, line_number_);
        configure.configure_call.bdx = parseInteger<unsigned>(requireParam(params, "bdx", out_, line_number_), "bdx", out_, line_number_);
        configure.configure_call.bdy = parseInteger<unsigned>(requireParam(params, "bdy", out_, line_number_), "bdy", out_, line_number_);
        configure.configure_call.bdz = parseInteger<unsigned>(requireParam(params, "bdz", out_, line_number_), "bdz", out_, line_number_);
        configure.configure_call.sharedMem = parseInteger<unsigned>(requireParam(params, "sharedBytes", out_, line_number_), "sharedBytes", out_, line_number_);
        configure.configure_call.stream = nullptr;
        queued_packets_.push(configure);

        auto function = functions_.find(function_name);
        if (function == functions_.end()) {
            uint64_t id = functions_.size();
            function = functions_.emplace(function_name, id).first;
            registration.cuda_call_id = CUDA_REG_FUNCTION;
            registration.register_function.fatCubinHandle = fatbin_handle_;
            registration.register_function.hostFun = id;
            strncpy(registration.register_function.deviceFun, ptx_name.c_str(),
                    BALAR_CUDA_MAX_KERNEL_NAME - 1);
            registration.register_function.deviceFun[BALAR_CUDA_MAX_KERNEL_NAME - 1] = '\0';
            packet = registration;
        } else {
            packet = queued_packets_.front();
            queued_packets_.pop();
        }

        size_t offset = 0;
        std::string arguments = requireParam(params, "args", out_, line_number_, true);
        while (!arguments.empty()) {
            size_t value_end = arguments.find('/');
            if (value_end == std::string::npos) {
                out_->fatal(CALL_INFO, -1,
                            "Balar trace line %zu: malformed kernel arguments\n",
                            line_number_);
            }
            std::string value = arguments.substr(0, value_end);
            arguments.erase(0, value_end + 1);
            size_t size_end = arguments.find('/');
            if (size_end == std::string::npos) {
                out_->fatal(CALL_INFO, -1,
                            "Balar trace line %zu: malformed kernel argument size\n",
                            line_number_);
            }
            size_t argument_size = parseInteger<size_t>(
                arguments.substr(0, size_end), "argument size", out_, line_number_);
            arguments.erase(0, size_end + 1);
            if (argument_size == 0 || argument_size > sizeof(argument.setup_argument.value)) {
                out_->fatal(CALL_INFO, -1,
                            "Balar trace line %zu: unsupported argument size %zu\n",
                            line_number_, argument_size);
            }
            offset = (offset + argument_size - 1) / argument_size * argument_size;
            argument.setup_argument.size = argument_size;
            argument.setup_argument.offset = offset;
            argument.setup_argument.arg = 0;
            memset(argument.setup_argument.value, 0,
                   sizeof(argument.setup_argument.value));
            offset += argument_size;

            if (value.find("dptr") != std::string::npos) {
                auto device = device_ptrs_.find(value);
                if (device == device_ptrs_.end()) {
                    out_->fatal(CALL_INFO, -1,
                                "Balar trace line %zu: unknown argument pointer '%s'\n",
                                line_number_, value.c_str());
                }
                argument.setup_argument.arg = device->second;
            } else if (value.find('.') != std::string::npos) {
                double parsed = 0.0;
                if (!ConfigParse::parseDouble(value, parsed)) {
                    out_->fatal(CALL_INFO, -1,
                                "Balar trace line %zu: invalid floating argument '%s'\n",
                                line_number_, value.c_str());
                }
                if (argument_size == sizeof(double)) {
                    memcpy(argument.setup_argument.value, &parsed, argument_size);
                } else if (argument_size == sizeof(float)) {
                    float narrowed = static_cast<float>(parsed);
                    memcpy(argument.setup_argument.value, &narrowed, argument_size);
                } else {
                    out_->fatal(CALL_INFO, -1,
                                "Balar trace line %zu: unsupported floating argument size %zu\n",
                                line_number_, argument_size);
                }
            } else {
                int parsed = parseSignedInteger(value, "integer argument", out_, line_number_);
                memcpy(argument.setup_argument.value, &parsed,
                       std::min(argument_size, sizeof(parsed)));
            }
            queued_packets_.push(argument);
        }
        launch.cuda_launch.func = function->second;
        queued_packets_.push(launch);
        return true;
    }

    if (call.find("free") != std::string::npos) {
        packet.cuda_call_id = CUDA_FREE;
        const auto& name = requireParam(params, "dptr", out_, line_number_);
        auto device = device_ptrs_.find(name);
        if (device == device_ptrs_.end()) {
            out_->fatal(CALL_INFO, -1,
                        "Balar trace line %zu: unknown device pointer '%s'\n",
                        line_number_, name.c_str());
        }
        packet.cuda_free.devPtr = reinterpret_cast<void*>(device->second);
        return true;
    }

    out_->fatal(CALL_INFO, -1, "Balar trace line %zu: unsupported call '%s'\n",
                line_number_, call.c_str());
    return false;
}

void BalarTraceParser::setFatbinHandle(uint64_t handle)
{
    fatbin_handle_ = handle;
    fatbin_registered_ = true;
}

#endif
