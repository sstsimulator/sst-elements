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

/**
 * @file cuda_runtime_api.h
 * @author Weili An (an107@purdue.edu)
 * @brief Test lib header for vanadis calling CUDA api
 * @version 0.1
 * @date 2022-09-22
 * 
 */

#ifndef __CUDA_RUNTIME_API_H__
#define __CUDA_RUNTIME_API_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#define LOG_LEVEL_INFO 20
#define LOG_LEVEL_DEBUG 15
#define LOG_LEVEL_WARNING 10
#define LOG_LEVEL_ERROR 0

// Global mmio gpu address
static uint32_t* g_gpu = (uint32_t*) 0xFFFF1000;
static uint8_t g_scratch_mem[512];
static int32_t g_debug_level = LOG_LEVEL_WARNING;

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

enum cudaMemcpyKind
{
    cudaMemcpyHostToHost          =   0,      /**< Host   -> Host */
    cudaMemcpyHostToDevice        =   1,      /**< Host   -> Device */
    cudaMemcpyDeviceToHost        =   2,      /**< Device -> Host */
    cudaMemcpyDeviceToDevice      =   3,      /**< Device -> Device */
    cudaMemcpyDefault             =   4       /**< Direction of the transfer is inferred from the pointer values. Requires unified virtual addressing */
};

enum cudaError
{
  cudaSuccess = 0,
  cudaErrorMissingConfiguration = 1,
  cudaErrorMemoryAllocation = 2,
  cudaErrorInitializationError = 3,
  cudaErrorLaunchFailure = 4,
  cudaErrorPriorLaunchFailure = 5,
  cudaErrorLaunchTimeout = 6,
  cudaErrorLaunchOutOfResources = 7,
  cudaErrorInvalidDeviceFunction = 8,
  cudaErrorInvalidConfiguration = 9,
  cudaErrorInvalidDevice = 10,
  cudaErrorInvalidValue = 11,
  cudaErrorInvalidPitchValue = 12,
  cudaErrorInvalidSymbol = 13,
  cudaErrorMapBufferObjectFailed = 14,
  cudaErrorUnmapBufferObjectFailed = 15,
  cudaErrorInvalidHostPointer = 16,
  cudaErrorInvalidDevicePointer = 17,
  cudaErrorInvalidTexture = 18,
  cudaErrorInvalidTextureBinding = 19,
  cudaErrorInvalidChannelDescriptor = 20,
  cudaErrorInvalidMemcpyDirection = 21,
  cudaErrorAddressOfConstant = 22,
  cudaErrorTextureFetchFailed = 23,
  cudaErrorTextureNotBound = 24,
  cudaErrorSynchronizationError = 25,
  cudaErrorInvalidFilterSetting = 26,
  cudaErrorInvalidNormSetting = 27,
  cudaErrorMixedDeviceExecution = 28,
  cudaErrorCudartUnloading = 29,
  cudaErrorUnknown = 30,
  cudaErrorNotYetImplemented = 31,
  cudaErrorMemoryValueTooLarge = 32,
  cudaErrorInvalidResourceHandle = 33,
  cudaErrorNotReady = 34,
  cudaErrorInsufficientDriver = 35,
  cudaErrorSetOnActiveProcess = 36,
  cudaErrorNoDevice = 38,
  cudaErrorStartupFailure = 0x7f,
  cudaErrorApiFailureBase = 10000
};

struct dim3
{
    unsigned int x, y, z;
};

typedef enum cudaError cudaError_t;
typedef struct dim3 dim3;

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
            uint64_t devPtr;
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
            enum cudaMemcpyKind kind;
            uint64_t payload;   // A pointer, but need to be 64-bit
        } cuda_memcpy;

        struct {
            uint32_t gdx;
            uint32_t gdy;
            uint32_t gdz;
            uint32_t bdx;
            uint32_t bdy;
            uint32_t bdz;
            uint64_t sharedMem;
            uint64_t stream;
        } configure_call;

        struct {
            uint64_t arg;
            uint8_t value[8];
            uint64_t size;
            uint64_t offset;
        } setup_argument;

        struct {
            uint64_t func;
        } cuda_launch;

        struct {
            uint64_t devPtr;
        } cuda_free;

        struct {
            uint64_t fatCubinHandle;
            uint64_t hostVar; //pointer to...something
            uint64_t deviceAddress; //name of variable
            uint64_t deviceName; //name of variable
            int32_t ext;
            int32_t size;
            int32_t constant;
            int32_t global;
        } register_var;

        struct {
            uint64_t numBlock;
            uint64_t hostFunc;
            int32_t blockSize;
            size_t dynamicSMemSize;
            uint32_t flags;
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
            volatile uint64_t    sim_data;
            volatile uint64_t    real_data;
            uint64_t      size;
            enum cudaMemcpyKind kind;
        } cudamemcpy;
    };
} BalarCudaCallReturnPacket_t;

cudaError_t cudaMalloc(void **devPtr, uint64_t size);

cudaError_t cudaMemcpy(uint64_t dst, uint64_t src, uint64_t count, enum cudaMemcpyKind kind);

// Cuda Configure call
cudaError_t cudaConfigureCall(dim3 gridDim, dim3 blockDim, uint64_t sharedMem);

// Cuda Setup argument
cudaError_t cudaSetupArgument(uint64_t arg, uint8_t value[8], uint64_t size, uint64_t offset);

cudaError_t cudaLaunch(uint64_t func);

unsigned int __cudaRegisterFatBinary(char file_name[256]);

// TODO: How to get the deviceFun name automatically?
// TODO: Requires parsing the binary?
void __cudaRegisterFunction(
    uint64_t fatCubinHandle,
    uint64_t hostFun,
    char deviceFun[256]
);


#endif 
