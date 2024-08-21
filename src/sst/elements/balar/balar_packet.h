// Copyright 2009-2023 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2023, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef BALAR_PACKET_H
#define BALAR_PACKET_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

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
        GPU_PARAM_CONFIG,
        GPU_PARAM_CONFIG_RET,
        GPU_THREAD_SYNC,
        GPU_THREAD_SYNC_RET,
        GPU_GET_ERROR_STRING,
        GPU_GET_ERROR_STRING_RET,
        GPU_MEMSET,
        GPU_MEMSET_RET,
        GPU_MEMCPY_TO_SYMBOL,
        GPU_MEMCPY_TO_SYMBOL_RET,
        GPU_SET_DEVICE,
        GPU_SET_DEVICE_RET,
        GPU_CREATE_CHANNEL_DESC,
        GPU_CREATE_CHANNEL_DESC_RET,
        GPU_BIND_TEXTURE,
        GPU_BIND_TEXTURE_RET,
        GPU_REG_TEXTURE,
        GPU_REG_TEXTURE_RET,
        GPU_GET_DEVICE_COUNT,
        GPU_GET_DEVICE_COUNT_RET,
        GPU_FREE_HOST,
        GPU_FREE_HOST_RET,
        GPU_MALLOC_HOST,
        GPU_MALLOC_HOST_RET,
    };

    // Future: Make this into a class with additional serialization methods?
    // Future: Make this into subclass of standardmem::request? and override the makeResponse function?
    typedef struct BalarCudaCallPacket {
        enum GpuApi_t cuda_call_id;
        // 0: means pointer data are not in SST mem space
        // 1: means data are in SST mem space, which is the
        //    case for Vanadis as all data are in SST mem space
        bool isSSTmem;
        union {
            struct {
                // Use uint64_t to match with the 8 byte ptr width
                // in balar
                void** devPtr;
                uint64_t size;
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
                uint64_t count;
                uint64_t payload;   // A pointer, but need to be 64-bit
                enum cudaMemcpyKind kind;
            } cuda_memcpy;

            struct {
                uint64_t sharedMem;
                uint64_t stream;
                uint32_t gdx;
                uint32_t gdy;
                uint32_t gdz;
                uint32_t bdx;
                uint32_t bdy;
                uint32_t bdz;
            } configure_call;

            struct {
                uint64_t arg;
                uint64_t size;
                uint64_t offset;
                uint8_t value[200];
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
                int32_t ext;
                int32_t size;
                int32_t constant;
                int32_t global;
            } register_var;

            struct {
                size_t dynamicSMemSize;
                uint64_t numBlock;
                uint64_t hostFunc;
                int32_t blockSize;
                uint32_t flags;
            } max_active_block;
            struct {
                uint64_t hostFun;
                unsigned index; // Argument index
            } cudaparamconfig;
            struct {
                void *mem;
                int c;
                size_t count;
            } cudamemset;
            struct {
                int device;
            } cudasetdevice;

            struct {
                void **fatCubinHandle;
                const struct textureReference *hostVar;
                const void **deviceAddress;
                const char *deviceName;
                int dim;
                int norm;
                int ext;
            } cudaregtexture;

            struct {
                size_t *offset;
                const struct textureReference *texref;
                const void *devPtr;
                const struct cudaChannelFormatDesc *desc;
                size_t size;
            } cudabindtexture;

            struct {
                void *addr;
                size_t size;
            } cudamallochost;
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
                volatile uint8_t *sim_data;
                volatile uint8_t *real_data;
                uint64_t      size;
                enum cudaMemcpyKind kind;
            } cudamemcpy;
            struct {
                size_t size;
                unsigned alignment;
            } cudaparamconfig;
            struct {
                int count;
            } cudagetdevicecount;
        };
    } BalarCudaCallReturnPacket_t;
}
}
#endif