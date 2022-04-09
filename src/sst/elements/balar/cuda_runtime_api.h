// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
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

extern "C"{

uint64_t cudaMallocSST(void **devPtr, size_t size);

unsigned CUDARTAPI __cudaRegisterFatBinarySST(char file_name[256]);

void CUDARTAPI __cudaRegisterFunctionSST(
    uint64_t fatCubinHandle,
    uint64_t hostFun,
    char deviceFun[256]
);


__host__ cudaError_t CUDARTAPI cudaMemcpySST(uint64_t dst, uint64_t src, size_t count, enum cudaMemcpyKind kind, uint8_t *payload);

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
