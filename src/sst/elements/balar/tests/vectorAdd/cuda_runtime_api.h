// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#if !defined(__dv)
#if defined(__cplusplus)
#define __dv(v) \
		= v
#else /* __cplusplus */
#define __dv(v)
#endif /* __cplusplus */
#endif /* !__dv */

#include "host_defines.h"
#include "builtin_types.h"
#include "driver_types.h"
#include <inttypes.h>

// Assign MMIO space must be large enough to store argument
#ifndef BALAR_MMIO_ADDR
	#define BALAR_MMIO_ADDR 0x1000
#endif

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


extern "C"{


unsigned CUDARTAPI __cudaRegisterFatBinary(char file_name[256]);

void CUDARTAPI __cudaRegisterFunction(
    uint64_t fatCubinHandle,
    uint64_t hostFun,
    char deviceFun[256]
);


// __host__ cudaError_t CUDARTAPI cudaMemcpySST(uint64_t dst, uint64_t src, size_t count, enum cudaMemcpyKind kind, uint8_t *payload);

/**
 * Save argument on mmio addr in sequence and put result on start addr
 * */
__host__ cudaError_t CUDARTAPI  cudaMalloc(void **devPtr, size_t size);

__host__ cudaError_t CUDARTAPI cudaMemcpy(void * dst, const void * src, size_t count, enum cudaMemcpyKind kind);

__host__ cudaError_t CUDARTAPI cudaConfigureCallSST(dim3 gridDim, dim3 blockDim, size_t sharedMem, cudaStream_t stream );


__host__ cudaError_t CUDARTAPI cudaSetupArgumentSST(uint64_t arg, uint8_t value[8], size_t size, size_t offset);

__host__ cudaError_t CUDARTAPI cudaLaunchSST(uint64_t func);

__host__ cudaError_t CUDARTAPI cudaFree(void *devPtr);


__host__ __cudart_builtin__ cudaError_t CUDARTAPI cudaGetLastError(void);

void __cudaRegisterVar(
		void **fatCubinHandle,
		char *hostVar, //pointer to...something
		char *deviceAddress, //name of variable
		const char *deviceName, //name of variable (same as above)
		int ext,
		int size,
		int constant,
		int global );

cudaError_t CUDARTAPI cudaOccupancyMaxActiveBlocksPerMultiprocessorWithFlags(
        int* numBlocks,
        const char *hostFunc,
        int blockSize,
        size_t dynamicSMemSize,
        unsigned int flags);

 void SST_receive_mem_reply(unsigned core_id,  void* mem_req);

 bool SST_gpu_core_cycle();

 void SST_gpgpusim_numcores_equal_check(unsigned sst_numcores);

}
