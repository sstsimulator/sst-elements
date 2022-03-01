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

#include "cuda_runtime_api.h"


/**
 * Save argument on mmio addr in sequence and put result on start addr
 * */

extern "C" {

unsigned CUDARTAPI __cudaRegisterFatBinary(char file_name[256]) {
    uint64_t *start_mmio_addr = (uint64_t*)BALAR_MMIO_ADDR;
    uint64_t *mmio_ptr = (uint64_t*)BALAR_MMIO_ADDR;
	*mmio_ptr       = GPU_REG_FAT_BINARY;
    *(++mmio_ptr)   = (uint64_t)file_name;
	return *start_mmio_addr;
}

void CUDARTAPI __cudaRegisterFunction(
    uint64_t fatCubinHandle,
    uint64_t hostFun,
    char deviceFun[256]
) {
    // uint64_t *start_mmio_addr = (uint64_t*)BALAR_MMIO_ADDR;
    uint64_t *mmio_ptr = (uint64_t*)BALAR_MMIO_ADDR;
	*mmio_ptr       = GPU_REG_FUNCTION;
    *(++mmio_ptr)   = fatCubinHandle;
    *(++mmio_ptr)   = hostFun;
    *(++mmio_ptr)   = (uint64_t)deviceFun;
    return;
}

__host__ cudaError_t CUDARTAPI  cudaMalloc(void **devPtr, size_t size) {
	uint64_t *start_mmio_addr = (uint64_t*)BALAR_MMIO_ADDR;
    uint64_t *mmio_ptr = (uint64_t*)BALAR_MMIO_ADDR;
	*mmio_ptr       = GPU_MALLOC;
    *(++mmio_ptr)   = (uint64_t)devPtr;
	*(++mmio_ptr)   = size;
	return (cudaError_t)*start_mmio_addr;
}

__host__ cudaError_t CUDARTAPI cudaMemcpy(void * dst, const void * src, size_t count, enum cudaMemcpyKind kind) {
    uint64_t *start_mmio_addr = (uint64_t*)BALAR_MMIO_ADDR;
    uint64_t *mmio_ptr = (uint64_t*)BALAR_MMIO_ADDR;
	*mmio_ptr       = GPU_MEMCPY;
    *(++mmio_ptr)   = (uint64_t)dst;
	*(++mmio_ptr)   = (uint64_t)src;
	*(++mmio_ptr)   = count;
	*(++mmio_ptr)   = kind;
	return (cudaError_t)*start_mmio_addr;
}

__host__ cudaError_t CUDARTAPI cudaConfigureCall(dim3 gridDim, dim3 blockDim, size_t sharedMem, cudaStream_t stream ) {
    uint64_t *start_mmio_addr = (uint64_t*)BALAR_MMIO_ADDR;
    uint64_t *mmio_ptr = (uint64_t*)BALAR_MMIO_ADDR;
	*mmio_ptr       = GPU_CONFIG_CALL;
    *(++mmio_ptr)   = gridDim.x;
    *(++mmio_ptr)   = gridDim.y;
    *(++mmio_ptr)   = gridDim.z;
	*(++mmio_ptr)   = blockDim.x;
	*(++mmio_ptr)   = blockDim.y;
	*(++mmio_ptr)   = blockDim.z;
	*(++mmio_ptr)   = sharedMem;
	*(++mmio_ptr)   = (uint64_t)stream;
	return (cudaError_t)*start_mmio_addr;
}


__host__ cudaError_t CUDARTAPI cudaSetupArgument(uint64_t arg, uint8_t value[8], size_t size, size_t offset) {
    uint64_t *start_mmio_addr = (uint64_t*)BALAR_MMIO_ADDR;
    uint64_t *mmio_ptr = (uint64_t*)BALAR_MMIO_ADDR;
	*mmio_ptr       = GPU_SET_ARG;
    *(++mmio_ptr)   = arg;
    *(++mmio_ptr)   = (uint64_t)value;
    *(++mmio_ptr)   = size;
	*(++mmio_ptr)   = offset;
	return (cudaError_t)*start_mmio_addr;
}

__host__ cudaError_t CUDARTAPI cudaLaunch(uint64_t func) {
    uint64_t *start_mmio_addr = (uint64_t*)BALAR_MMIO_ADDR;
    uint64_t *mmio_ptr = (uint64_t*)BALAR_MMIO_ADDR;
	*mmio_ptr       = GPU_LAUNCH;
    *(++mmio_ptr)   = func;
	return (cudaError_t)*start_mmio_addr;
}

__host__ cudaError_t CUDARTAPI cudaFree(void *devPtr) {
    uint64_t *start_mmio_addr = (uint64_t*)BALAR_MMIO_ADDR;
    uint64_t *mmio_ptr = (uint64_t*)BALAR_MMIO_ADDR;
	*mmio_ptr       = GPU_FREE;
    *(++mmio_ptr)   = (uint64_t)devPtr;
	return (cudaError_t)*start_mmio_addr;
}


__host__ __cudart_builtin__ cudaError_t CUDARTAPI cudaGetLastError(void) {
    uint64_t *start_mmio_addr = (uint64_t*)BALAR_MMIO_ADDR;
    uint64_t *mmio_ptr = (uint64_t*)BALAR_MMIO_ADDR;
	*mmio_ptr       = GPU_GET_LAST_ERROR;
	return (cudaError_t)*start_mmio_addr;
}

void __cudaRegisterVar(
		void **fatCubinHandle,
		char *hostVar, //pointer to...something
		char *deviceAddress, //name of variable
		const char *deviceName, //name of variable (same as above)
		int ext,
		int size,
		int constant,
		int global ) {
    // uint64_t *start_mmio_addr = (uint64_t*)BALAR_MMIO_ADDR;
    uint64_t *mmio_ptr = (uint64_t*)BALAR_MMIO_ADDR;
	*mmio_ptr       = GPU_REG_VAR;
    *(++mmio_ptr)   = (uint64_t)fatCubinHandle;
    *(++mmio_ptr)   = (uint64_t)hostVar;
    *(++mmio_ptr)   = (uint64_t)deviceAddress;
    *(++mmio_ptr)   = (uint64_t)deviceName;
    *(++mmio_ptr)   = ext;
    *(++mmio_ptr)   = size;
    *(++mmio_ptr)   = constant;
    *(++mmio_ptr)   = global;

	return;
}

cudaError_t CUDARTAPI cudaOccupancyMaxActiveBlocksPerMultiprocessorWithFlags(
        int* numBlocks,
        const char *hostFunc,
        int blockSize,
        size_t dynamicSMemSize,
        unsigned int flags) {
    uint64_t *start_mmio_addr = (uint64_t*)BALAR_MMIO_ADDR;
    uint64_t *mmio_ptr = (uint64_t*)BALAR_MMIO_ADDR;
	*mmio_ptr       = GPU_MAX_BLOCK;
    *(++mmio_ptr)   = (uint64_t)numBlocks;
    *(++mmio_ptr)   = (uint64_t)hostFunc;
    *(++mmio_ptr)   = blockSize;
    *(++mmio_ptr)   = dynamicSMemSize;
    *(++mmio_ptr)   = flags;

	return (cudaError_t)*start_mmio_addr;
}

}
