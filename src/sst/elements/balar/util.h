// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef BALAR_UTIL_H
#define BALAR_UTIL_H

#include <stdint.h>
#include <vector>
#include <string>
#include <map>
#include "host_defines.h"
#include "builtin_types.h"
#include "driver_types.h"
#include "balar_packet.h"

using namespace std;

namespace SST {
namespace BalarComponent {
    // Need to move the template instantiation to header file
    // See: https://stackoverflow.com/questions/495021/why-can-templates-only-be-implemented-in-the-header-file
    template <typename T> 
    vector<uint8_t>* encode_balar_packet(T *pack_ptr) {
        vector<uint8_t> *buffer = new vector<uint8_t>();

        // Treat pack ptr as uint8_t ptr
        uint8_t* ptr = reinterpret_cast<uint8_t*>(pack_ptr);

        // Get buffer initial size
        size_t len = buffer->size();

        // Allocate enough space
        buffer->resize(len + sizeof(T));

        // Copy the packet to buffer end
        std::copy(ptr, ptr + sizeof(T), buffer->data() + len);
        return buffer;
    }

    template <typename T>
    T* decode_balar_packet(vector<uint8_t> *buffer) {
        T* pack_ptr = new T();
        size_t len = sizeof(T);

        // Match with type of buffer
        uint8_t* ptr = reinterpret_cast<uint8_t*>(pack_ptr);
        std::copy(buffer->data(), buffer->data() + len, ptr);
        
        return pack_ptr;
    }

    string* gpu_api_to_string(enum GpuApi_t api);
    std::string& trim(std::string& s);
    std::vector<std::string> split(std::string& s, const std::string& delim);
    std::map<std::string, std::string> map_from_vec(std::vector<std::string> vec, const std::string& delim);

}
}

#endif // !BALAR_UTIL_H
