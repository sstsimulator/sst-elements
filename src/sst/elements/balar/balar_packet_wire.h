// Copyright 2009-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2026, NTESS
// All rights reserved.

#ifndef BALAR_PACKET_WIRE_H
#define BALAR_PACKET_WIRE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "balar_consts.h"

#ifdef __cplusplus
namespace SST {
namespace BalarComponent {
#endif

enum CudaAPI_t {
    CUDA_REG_FAT_BINARY = 1,
    CUDA_REG_FUNCTION,
    CUDA_MEMCPY,
    CUDA_CONFIG_CALL,
    CUDA_SET_ARG,
    CUDA_LAUNCH,
    CUDA_FREE,
    CUDA_GET_LAST_ERROR,
    CUDA_MALLOC,
    CUDA_REG_VAR,
    CUDA_MAX_BLOCK,
    CUDA_PARAM_CONFIG,
    CUDA_THREAD_SYNC,
    CUDA_GET_ERROR_STRING,
    CUDA_MEMSET,
    CUDA_MEMCPY_TO_SYMBOL,
    CUDA_MEMCPY_FROM_SYMBOL,
    CUDA_SET_DEVICE,
    CUDA_CREATE_CHANNEL_DESC,
    CUDA_BIND_TEXTURE,
    CUDA_REG_TEXTURE,
    CUDA_GET_DEVICE_COUNT,
    CUDA_FREE_HOST,
    CUDA_MALLOC_HOST,
    CUDA_MEMCPY_ASYNC,
    CUDA_GET_DEVICE_PROPERTIES,
    CUDA_SET_DEVICE_FLAGS,
    CUDA_STREAM_CREATE,
    CUDA_STREAM_DESTROY,
    CUDA_STREAM_SYNC,
    CUDA_EVENT_CREATE,
    CUDA_EVENT_CREATE_WITH_FLAGS,
    CUDA_EVENT_RECORD,
    CUDA_EVENT_SYNCHRONIZE,
    CUDA_EVENT_ELAPSED_TIME,
    CUDA_EVENT_DESTROY,
    CUDA_DEVICE_GET_ATTRIBUTE,
};

typedef struct BalarCudaCallPacket {
    enum CudaAPI_t cuda_call_id;
    bool isSSTmem;
    union {
        struct {
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
            uint64_t payload;
            uint8_t *dst_buf;
            uint8_t *src_buf;
            enum cudaMemcpyKind kind;
        } cuda_memcpy;

        struct {
            uint64_t dst;
            uint64_t src;
            uint64_t count;
            uint64_t payload;
            uint8_t *dst_buf;
            uint8_t *src_buf;
            enum cudaMemcpyKind kind;
            cudaStream_t stream;
        } cudaMemcpyAsync;

        struct {
            uint64_t symbol;
            uint64_t src;
            uint64_t count;
            uint64_t offset;
            uint8_t *dst_buf;
            uint8_t *src_buf;
            enum cudaMemcpyKind kind;
        } cuda_memcpy_to_symbol;

        struct {
            uint64_t symbol;
            uint64_t dst;
            uint64_t count;
            uint64_t offset;
            uint8_t *dst_buf;
            uint8_t *src_buf;
            enum cudaMemcpyKind kind;
        } cuda_memcpy_from_symbol;

        struct {
            uint64_t sharedMem;
            cudaStream_t stream;
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
            char *hostVar;
            char *deviceAddress;
            char deviceName[BALAR_CUDA_MAX_DEV_VAR_NAME];
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
            unsigned index;
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
        } cudaStreamSynchronize;

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
    enum CudaAPI_t cuda_call_id;
    cudaError_t cuda_error;
    bool is_cuda_call_done;
    union {
        uint64_t fat_cubin_handle;
        struct {
            uint64_t malloc_addr;
            uint64_t devptr_addr;
        } cudamalloc;
        struct {
            volatile uint8_t *sim_data;
            volatile uint8_t *real_data;
            uint64_t size;
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
            cudaEvent_t event;
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

#ifdef __cplusplus
static_assert(sizeof(BalarCudaCallPacket_t) > 0, "Balar packet wire ABI is empty");
static_assert(sizeof(BalarCudaCallReturnPacket_t) > 0, "Balar return packet wire ABI is empty");
} // namespace BalarComponent
} // namespace SST
#endif

#endif // BALAR_PACKET_WIRE_H
