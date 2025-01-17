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

using namespace std;

namespace SST {
namespace BalarComponent {
    enum GpuApi_t {
        GPU_REG_FAT_BINARY = 1,
        GPU_REG_FAT_BINARY_RET = 2,
        GPU_REG_FUNCTION = 3,
        GPU_REG_FUNCTION_RET = 4,
        GPU_MEMCPY = 5,
        GPU_MEMCPY_RET = 6,
        GPU_CONFIG_CALL = 7,
        GPU_CONFIG_CALL_RET = 8,
        GPU_SET_ARG = 9,
        GPU_SET_ARG_RET = 10,
        GPU_LAUNCH = 11,
        GPU_LAUNCH_RET = 12,
        GPU_FREE = 13,
        GPU_FREE_RET = 14,
        GPU_GET_LAST_ERROR = 15,
        GPU_GET_LAST_ERROR_RET = 16,
        GPU_MALLOC = 17,
        GPU_MALLOC_RET = 18,
        GPU_REG_VAR = 19,
        GPU_REG_VAR_RET = 20,
        GPU_MAX_BLOCK = 21,
        GPU_MAX_BLOCK_RET = 22,
    };

    
    // Future: Make this into a class with additional serialization methods?
    // Future: Make this into subclass of standardmem::request? and override the makeResponse function?
    typedef struct BalarCudaCallPacket {
        enum GpuApi_t cuda_call_id;
        // Whether the pointer data are in SST memory space
        // 0: no, same space as the simulator
        // 1: yes, the data are in SST mem space and we
        //    need to query and write to them via standard 
        //    mem interface
        bool isSSTmem;
        union {
            struct {
                void** devPtr;
                size_t size;
            } cuda_malloc;

            struct {
                char file_name[256];
            } register_fatbin;

            struct {
                uint64_t fatCubinHandle;
                uint64_t hostFun;
                char deviceFun[256];
            } register_function;
            
            struct {
                uint64_t dst;
                uint64_t src;
                size_t count;
                enum cudaMemcpyKind kind;
                volatile uint8_t *payload;
            } cuda_memcpy;

            struct {
                unsigned int gdx;
                unsigned int gdy;
                unsigned int gdz;
                unsigned int bdx;
                unsigned int bdy;
                unsigned int bdz;
                size_t sharedMem;
                cudaStream_t stream;
            } configure_call;

            struct {
                uint64_t arg;
                uint8_t value[8];
                size_t size;
                size_t offset;
            } setup_argument;

            struct {
                uint64_t func;
            } cuda_launch;

            struct {
                void *devPtr;
            } cuda_free;

            struct {
                void **fatCubinHandle;
                char *hostVar; //pointer to...something
                char *deviceAddress; //name of variable
                const char *deviceName; //name of variable
                int ext;
                int size;
                int constant;
                int global;
            } register_var;

            struct {
                int *numBlock;
                const char *hostFunc;
                int blockSize;
                size_t dynamicSMemSize;
                unsigned int flags;
            } max_active_block;
        };
    } BalarCudaCallPacket_t;

    typedef struct BalarCudaCallReturnPacket {
        enum GpuApi_t cuda_call_id;
        cudaError_t cuda_error;
        bool is_cuda_call_done; 
        union {
            uint64_t    fat_cubin_handle;
            struct {
                uint64_t    malloc_addr;
                uint64_t    devptr_addr;
            } cudamalloc;
            struct {
                volatile uint8_t*    sim_data;
                volatile uint8_t*    real_data;
                size_t      size;
                enum cudaMemcpyKind kind;
            } cudamemcpy;
        };
    } BalarCudaCallReturnPacket_t;

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
        buffer->resize(len + sizeof(*pack_ptr));

        // Copy the packet to buffer end
        std::copy(ptr, ptr + sizeof(*pack_ptr), buffer->data() + len);
        return buffer;
    }

    template <typename T>
    T* decode_balar_packet(vector<uint8_t> *buffer) {
        T* pack_ptr = new T();
        size_t len = sizeof(*pack_ptr);

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
