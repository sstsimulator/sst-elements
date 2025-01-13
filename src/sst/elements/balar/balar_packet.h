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
#include "balar_consts.h"

namespace SST {
namespace BalarComponent {
    enum GpuApi_t {
        GPU_REG_FAT_BINARY = 1,
        GPU_REG_FUNCTION,
        GPU_MEMCPY,
        GPU_CONFIG_CALL,
        GPU_SET_ARG,
        GPU_LAUNCH,
        GPU_FREE,
        GPU_GET_LAST_ERROR,
        GPU_MALLOC,
        GPU_REG_VAR,
        GPU_MAX_BLOCK,
        GPU_PARAM_CONFIG,
        GPU_THREAD_SYNC,
        GPU_GET_ERROR_STRING,
        GPU_MEMSET,
        GPU_MEMCPY_TO_SYMBOL,
        GPU_MEMCPY_FROM_SYMBOL,
        GPU_SET_DEVICE,
        GPU_CREATE_CHANNEL_DESC,
        GPU_BIND_TEXTURE,
        GPU_REG_TEXTURE,
        GPU_GET_DEVICE_COUNT,
        GPU_FREE_HOST,
        GPU_MALLOC_HOST,
        GPU_MEMCPY_ASYNC,
        GPU_GET_DEVICE_PROPERTIES,
        GPU_SET_DEVICE_FLAGS,
        GPU_STREAM_CREATE,
        GPU_STREAM_DESTROY,
        GPU_EVENT_CREATE,
        GPU_EVENT_CREATE_WITH_FLAGS,
        GPU_EVENT_RECORD,
        GPU_EVENT_SYNCHRONIZE,
        GPU_EVENT_ELAPSED_TIME,
        GPU_EVENT_DESTROY,
        GPU_DEVICE_GET_ATTRIBUTE,
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
                char file_name[BALAR_CUDA_MAX_FILE_NAME];
            } register_fatbin;

            struct {
                uint64_t fatCubinHandle;
                uint64_t hostFun;
                char deviceFun[BALAR_CUDA_MAX_KERNEL_NAME];
            } register_function;
            
            struct {
                uint64_t dst;
                uint64_t src;
                uint64_t count;
                uint64_t payload;   // A pointer, but need to be 64-bit
                enum cudaMemcpyKind kind;
            } cuda_memcpy;

            struct {
                uint64_t dst;
                uint64_t src;
                uint64_t count;
                uint64_t payload;   // A pointer, but need to be 64-bit
                enum cudaMemcpyKind kind;
                cudaStream_t stream;
            } cudaMemcpyAsync;

            struct {
                uint64_t symbol;
                uint64_t src;
                uint64_t count;
                uint64_t offset;
                enum cudaMemcpyKind kind;
            } cuda_memcpy_to_symbol;

            struct {
                uint64_t symbol;
                uint64_t dst;
                uint64_t count;
                uint64_t offset;
                enum cudaMemcpyKind kind;
            } cuda_memcpy_from_symbol;

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
                uint8_t value[BALAR_CUDA_MAX_ARG_SIZE];
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
                char deviceName[BALAR_CUDA_MAX_DEV_VAR_NAME]; //name of variable
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

            // Pass struct directly so we can create a copy
            struct {
                void **fatCubinHandle;
                uint64_t hostVar_ptr;
                struct textureReference texRef;
                const void **deviceAddress;
                char deviceName[BALAR_CUDA_MAX_DEV_VAR_NAME];
                int dim;
                int norm;
                int ext;
            } cudaregtexture;

            struct {
                size_t *offset;
                uint64_t hostVar_ptr;
                struct textureReference texRef;
                const void *devPtr;
                struct cudaChannelFormatDesc desc_struct;
                size_t size;
            } cudabindtexture;

            struct {
                void *addr;
                size_t size;
            } cudamallochost;

            struct {
                int device;
            } cudaGetDeviceProperties;

            struct {
                unsigned int flags;
            } cudaSetDeviceFlags;

            struct {
                cudaStream_t stream;
            } cudaStreamDestroy;

            struct {
                unsigned int flags;
            } cudaEventCreateWithFlags;

            struct {
                cudaEvent_t event;
                cudaStream_t stream;
            } cudaEventRecord;

            struct {
                cudaEvent_t event;
            } cudaEventSynchronize;

            struct {
                cudaEvent_t start;
                cudaEvent_t end;
            } cudaEventElapsedTime;

            struct {
                cudaEvent_t event;
            } cudaEventDestroy;

            struct {
                cudaDeviceAttr attr;
                int device;
            } cudaDeviceGetAttribute;
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
            struct {
                struct cudaDeviceProp prop;
            } cudaGetDeviceProperties;
            struct {
                cudaStream_t stream;
            } cudaStreamCreate;
            struct {
                cudaStream_t event;
            } cudaEventCreate;
            struct {
                cudaEvent_t event;
            } cudaEventCreateWithFlags;
            struct {
                float ms;
            } cudaEventElapsedTime;
            struct {
                int value;
            } cudaDeviceGetAttribute;
        };
    } BalarCudaCallReturnPacket_t;
}
}
#endif