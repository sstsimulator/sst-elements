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

#include "cuda_runtime_api.h"
#include "balar_vanadis.h"
#include "../../balar_packet.h"

using namespace SST::BalarComponent;

// Encode version information here
#ifndef BALAR_CUDA_VERSION
#define BALAR_CUDA_VERSION "11.0"
#endif
static const char *version = BALAR_CUDA_VERSION;

#if !defined(__dv)
#if defined(__cplusplus)
#define __dv(v) = v
#else /* __cplusplus */
#define __dv(v)
#endif /* __cplusplus */
#endif /* !__dv */

extern "C" {
    #include <unistd.h>
    #include <sys/syscall.h>
    #include <sys/mman.h>
    #include <stdlib.h>

    inline __attribute__((always_inline)) void __vanadisFence() {
        asm volatile ("fence");
    }

    // Map balar's address with some memory protection bits
    // Use inline syscall to avoid unexpected speculative 
    // loads to balar's address
    inline __attribute__((always_inline)) void __vanadisMapBalar(int prot) {
        // Default to restrict access til we make a cuda call
        __vanadisFence();
        int syscall_num;
        #ifdef SYS_mmap2
            syscall_num = SYS_mmap2;
        #else
            syscall_num = SYS_mmap;
        #endif
        // #ifdef SYS_mmap2
        //     g_balarBaseAddr = (Addr_t*) syscall(SYS_mmap2, 0, 0, prot, 0, -2000, 0);
        // #else
        //     g_balarBaseAddr = (Addr_t*) syscall(SYS_mmap, 0, 0, prot, 0, -2000, 0);
        // #endif
        // mmap
        asm volatile (
            "add sp, sp, -56\n\t" // Reserve some stack space for storing ecall regs
            "sd a0, 48(sp)\n\t"   // Save regs
            "sd a1, 40(sp)\n\t"
            "sd a2, 32(sp)\n\t"
            "sd a3, 24(sp)\n\t"
            "sd a4, 16(sp)\n\t"
            "sd a5, 8(sp)\n\t"
            "sd a7, 0(sp)\n\t"
            // Begin mmap syscall
            "li a0, 0\n\t"        // addr: 0
            "li a1, 0\n\t"        // length: 0
            "mv a2, %1\n\t"       // prot: prot
            "li a3, 0\n\t"        // flags: 0
            "li a4, -2000\n\t"    // fd: balar's fd in vanadis
            "li a5, 0\n\t"        // offset: 0
            "mv a7, %2\n\t"       // syscall: SYS_mmap
            "ecall\n\t"           // perform syscall
            "sd a0, %0\n\t"       // store mmap address
            "ld a0, 48(sp)\n\t"   // Restore saved regs
            "ld a1, 40(sp)\n\t"
            "ld a2, 32(sp)\n\t"
            "ld a3, 24(sp)\n\t"
            "ld a4, 16(sp)\n\t"
            "ld a5, 8(sp)\n\t"
            "ld a7, 0(sp)\n\t"
            "add sp, sp, 56\n\t"  // Restore stack pointer
            : "=m" (g_balarBaseAddr)
            : "r" (prot), "r" (syscall_num)
            : "a0", "a1", "a2", "a3", "a4", "a5", "a7", "memory"
        );
        __vanadisFence();
    }

    // unmap balar's address
    inline __attribute__((always_inline)) void __vanadisUnmapBalar() {
        __vanadisFence();
        // munmap(g_balarBaseAddr, 4096);
        int syscall_num = SYS_munmap;
        asm volatile (
            "add sp, sp, -56\n\t" // Reserve some stack space for storing ecall regs
            "sd a0, 48(sp)\n\t"   // Save regs
            "sd a1, 40(sp)\n\t"
            "sd a2, 32(sp)\n\t"
            "sd a3, 24(sp)\n\t"
            "sd a4, 16(sp)\n\t"
            "sd a5, 8(sp)\n\t"
            "sd a7, 0(sp)\n\t"
            // Begin mmap syscall
            "mv a0, %0\n\t"       // addr: g_balarBaseAddr
            "li a1, 4096\n\t"     // length: 0
            "li a2, 0\n\t"        // unused
            "li a3, 0\n\t"        // unused
            "li a4, 0\n\t"        // unused
            "li a5, 0\n\t"        // unused
            "mv a7, %1\n\t"       // syscall: SYS_mumap
            "ecall\n\t"           // perform syscall
            "ld a0, 48(sp)\n\t"   // Restore saved regs
            "ld a1, 40(sp)\n\t"
            "ld a2, 32(sp)\n\t"
            "ld a3, 24(sp)\n\t"
            "ld a4, 16(sp)\n\t"
            "ld a5, 8(sp)\n\t"
            "ld a7, 0(sp)\n\t"
            "add sp, sp, 56\n\t"  // Restore stack pointer
            :
            : "r" (g_balarBaseAddr), "r" (syscall_num)
            : "a0", "a1", "a2", "a3", "a4", "a5", "a7", "memory"
        );
        __vanadisFence();
    }
}

extern "C" BalarCudaCallReturnPacket_t * makeCudaCall(BalarCudaCallPacket_t * call_packet_ptr) {
    // Make cuda call
    if (g_debug_level >= LOG_LEVEL_DEBUG) {
        printf("Making CUDA API Call ID: %d\n", 
                call_packet_ptr->cuda_call_id);
    }
    __vanadisMapBalar(PROT_READ|PROT_WRITE);
    *g_balarBaseAddr = (Addr_t) call_packet_ptr;
    __vanadisFence();

    // Read from GPU will return the address to the cuda return packet
    BalarCudaCallReturnPacket_t *response_packet_ptr = (BalarCudaCallReturnPacket_t *)*g_balarBaseAddr;
    __vanadisUnmapBalar();

    return response_packet_ptr;
}

extern "C" BalarCudaCallReturnPacket_t * readLastCudaStatus() {
    __vanadisMapBalar(PROT_READ|PROT_WRITE);
    BalarCudaCallReturnPacket_t * response_packet_ptr = (BalarCudaCallReturnPacket_t *)*g_balarBaseAddr;
    __vanadisUnmapBalar();
    return response_packet_ptr;
}

cudaError_t cudaMalloc(void **devPtr, uint64_t size) {
    // Send request to GPU
    BalarCudaCallPacket_t *call_packet_ptr = (BalarCudaCallPacket_t *) g_scratch_mem;
    call_packet_ptr->isSSTmem = true;
    call_packet_ptr->cuda_call_id = GPU_MALLOC;
    call_packet_ptr->cuda_malloc.size = size;
    call_packet_ptr->cuda_malloc.devPtr = devPtr;

    if (g_debug_level >= LOG_LEVEL_DEBUG) {
        printf("Malloc Packet address: %p\n", call_packet_ptr);
        printf("Malloc Packet size: %lu\n", size);
        printf("Malloc Packet devptr: %p\n", devPtr);
    }
    
    // Make cuda call
    BalarCudaCallReturnPacket_t *response_packet_ptr = makeCudaCall(call_packet_ptr);

    if (g_debug_level >= LOG_LEVEL_DEBUG) {
        printf("CUDA API ID: %d with error: %d\nMalloc addr: %lx Dev addr: %lx\n", 
                response_packet_ptr->cuda_call_id, response_packet_ptr->cuda_error,
                response_packet_ptr->cudamalloc.malloc_addr, response_packet_ptr->cudamalloc.devptr_addr);
    }

    *devPtr = (void *)response_packet_ptr->cudamalloc.malloc_addr;

    return response_packet_ptr->cuda_error;
}

cudaError_t cudaMemcpy(void *dst, const void *src, size_t count, enum cudaMemcpyKind kind) {
    // Send request to GPU
    if (g_debug_level >= LOG_LEVEL_DEBUG) {
        printf("Memcpy dst: %llx\n", dst);
    }
    BalarCudaCallPacket_t *call_packet_ptr = (BalarCudaCallPacket_t *) g_scratch_mem;
    call_packet_ptr->isSSTmem = true;
    call_packet_ptr->cuda_call_id = GPU_MEMCPY;

    call_packet_ptr->cuda_memcpy.kind = kind;
    call_packet_ptr->cuda_memcpy.dst = (uint64_t) dst;
    call_packet_ptr->cuda_memcpy.src = (uint64_t) src;
    call_packet_ptr->cuda_memcpy.count = count;
    call_packet_ptr->cuda_memcpy.payload = (uint64_t) src;

    if (g_debug_level >= LOG_LEVEL_DEBUG) {
        printf("Memcpy Packet address: %p\n", call_packet_ptr);
        printf("Memcpy kind: %d\n", (uint8_t) (call_packet_ptr->cuda_memcpy.kind));
        printf("Memcpy size: %ld\n", count);
        printf("Memcpy src: %p\n", src);
        printf("Memcpy dst: %p\n", dst);
        // printf("sizeof: %d\n", sizeof(call_packet_ptr->cuda_memcpy.kind));
        fflush(stdout);
    }

    // Make cuda call
    BalarCudaCallReturnPacket_t *response_packet_ptr = makeCudaCall(call_packet_ptr);

    // Make the memcpy sync with balar so that CPU will
    // issue another write/read to balar only if the previous memcpy is done
    while (!(response_packet_ptr->is_cuda_call_done)) {
        int wait = 0;
        response_packet_ptr = readLastCudaStatus();
        wait++;

        // if (wait % 10 == 0) {
        //     printf("Waited %d times\n", wait);
        // }
    }

    if (g_debug_level >= LOG_LEVEL_DEBUG) {
        printf("CUDA API ID: %d with error: %d\n", 
                response_packet_ptr->cuda_call_id, response_packet_ptr->cuda_error);
    }
    
    return response_packet_ptr->cuda_error;
}

// TODO How to handle this memcpy?
__host__ cudaError_t CUDARTAPI cudaMemcpyToSymbol(
    const char *symbol, const void *src, size_t count, size_t offset __dv(0),
    enum cudaMemcpyKind kind __dv(cudaMemcpyHostToDevice)) {

}

extern "C" {
// Function declarations
cudaError_t cudaLaunch(uint64_t func);

// Cuda Configure call
cudaError_t cudaConfigureCall(dim3 gridDim, dim3 blockDim, uint64_t sharedMem) {
    BalarCudaCallPacket_t *call_packet_ptr = (BalarCudaCallPacket_t *) g_scratch_mem;
    call_packet_ptr->isSSTmem = true;
    call_packet_ptr->cuda_call_id = GPU_CONFIG_CALL;

    // Prepare packet
    call_packet_ptr->configure_call.gdx = gridDim.x;
    call_packet_ptr->configure_call.gdy = gridDim.y;
    call_packet_ptr->configure_call.gdz = gridDim.z;
    call_packet_ptr->configure_call.bdx = blockDim.x;
    call_packet_ptr->configure_call.bdy = blockDim.y;
    call_packet_ptr->configure_call.bdz = blockDim.z;
    call_packet_ptr->configure_call.sharedMem = sharedMem;
    call_packet_ptr->configure_call.stream = (uint64_t) NULL;

    if (g_debug_level >= LOG_LEVEL_DEBUG) {
        printf("Start Configure Call:\n");
        printf("Grid Dim: x(%d) y(%d) z(%d)\n", gridDim.x, gridDim.y, gridDim.z);
        printf("Block Dim: x(%d) y(%d) z(%d)\n", blockDim.x, blockDim.y, blockDim.z);
    }

    // Make cuda call
    BalarCudaCallReturnPacket_t *response_packet_ptr = makeCudaCall(call_packet_ptr);
    
    if (g_debug_level >= LOG_LEVEL_DEBUG) {
        printf("CUDA API ID: %d with error: %d\n", 
                response_packet_ptr->cuda_call_id, response_packet_ptr->cuda_error);
    }
    
    return response_packet_ptr->cuda_error;
}

// Weili: Need to refer to GPGPU-Sim libcuda for these 
// https://github.com/accel-sim/gpgpu-sim_distribution/blob/a0c12f5d63504c67c8bdfb1a6cc689b4ab7867a6/libcuda/cuda_runtime_api.cc#L577
cudaError_t __cudaPopCallConfiguration(dim3 *gridDim, dim3 *blockDim, size_t *sharedMem, void *stream) {
	return cudaSuccess;
}
unsigned CUDARTAPI __cudaPushCallConfiguration(dim3 gridDim, dim3 blockDim,
                                               size_t sharedMem,
                                               void *stream) {
	cudaConfigureCall(gridDim, blockDim, sharedMem);
    return 0;
}

// Cuda Setup argument
cudaError_t cudaSetupArgument(uint64_t arg, uint8_t value[200], uint64_t size, uint64_t offset) {
    if (g_debug_level >= LOG_LEVEL_DEBUG) {
        printf("Start setup argument:\n");
        printf("Size: %d offset: %d\n", size, offset);
    }

    BalarCudaCallPacket_t *call_packet_ptr = (BalarCudaCallPacket_t *) g_scratch_mem;
    call_packet_ptr->isSSTmem = true;
    call_packet_ptr->cuda_call_id = GPU_SET_ARG;
    call_packet_ptr->setup_argument.size = size;
    call_packet_ptr->setup_argument.offset = offset;


    if (value == NULL) {
        // The argument is a device pointer
        if (g_debug_level >= LOG_LEVEL_DEBUG) {
            printf("Argument is a device pointer\n");
        }
        call_packet_ptr->setup_argument.arg = arg;
    } else {
        // The argument is a constant value
        if (g_debug_level >= LOG_LEVEL_DEBUG) {
            printf("Argument is a constant\n");
        }
        call_packet_ptr->setup_argument.arg =  (uint64_t) NULL;
        for (int i = 0; i < 8; i++) {
            call_packet_ptr->setup_argument.value[i] = value[i];
        }
    }

    // Make cuda call
    BalarCudaCallReturnPacket_t *response_packet_ptr = makeCudaCall(call_packet_ptr);

    if (g_debug_level >= LOG_LEVEL_DEBUG) {
        printf("CUDA API ID: %d with error: %d\n", 
                response_packet_ptr->cuda_call_id, response_packet_ptr->cuda_error);
    }

    return response_packet_ptr->cuda_error;
}

BalarCudaCallReturnPacket_t __balarGetParamInfo(uint64_t hostFunc, unsigned index) {
    if (g_debug_level >= LOG_LEVEL_DEBUG) {
        printf("Query param %d of function %d\n", index, hostFunc);
    }

    BalarCudaCallPacket_t *call_packet_ptr = (BalarCudaCallPacket_t *) g_scratch_mem;
    call_packet_ptr->isSSTmem = true;
    call_packet_ptr->cuda_call_id = GPU_PARAM_CONFIG;
    call_packet_ptr->cudaparamconfig.hostFun = hostFunc;
    call_packet_ptr->cudaparamconfig.index = index;

    // Make cuda call
    BalarCudaCallReturnPacket_t *response_packet_ptr = makeCudaCall(call_packet_ptr);

    if (g_debug_level >= LOG_LEVEL_DEBUG) {
        printf("CUDA API ID: %d with error: %d\n", 
                response_packet_ptr->cuda_call_id, response_packet_ptr->cuda_error);
    }
    
    return *response_packet_ptr;
}

__host__ cudaError_t CUDARTAPI cudaLaunchKernel(const void *hostFun,
                                                dim3 gridDim, dim3 blockDim,
                                                void **args,
                                                size_t sharedMem,
                                                cudaStream_t stream) {
    if (g_debug_level >= LOG_LEVEL_DEBUG) {
        printf("Prepare to launch kernel with id: %d\n", hostFun);
    }
    #if CUDART_VERSION < 10000
        cudaConfigureCall(gridDim, blockDim, sharedMem);
    #endif
    BalarCudaCallReturnPacket_t ret;
    for (unsigned index;;index++) {
        // Get param info
        ret = __balarGetParamInfo((uint64_t) hostFun, index);
        if (ret.cuda_error != cudaSuccess) {
            // Get param info return error value if the index is invalid
            break;
        } else {
            // How to pass argument? All use the value[8]
            // as cudaSetupArgument will make a copy of its content
            // so that GPGPU-Sim will know both the constant and pointer pass to the kernel
            uint8_t value[200];
            if (ret.cudaparamconfig.size > 200) {
                printf("CUDA function argument size(%d) exceeds %d bytes limit!\n", ret.cudaparamconfig.size, 200);
            }
            memcpy(value, args[index], ret.cudaparamconfig.size);
            cudaSetupArgument((uint64_t) NULL, value, ret.cudaparamconfig.size, ret.cudaparamconfig.alignment);
        }
    }

    return cudaLaunch((uint64_t) hostFun);
}

cudaError_t cudaLaunch(uint64_t func) {
    if (g_debug_level >= LOG_LEVEL_DEBUG) {
        printf("Start kernel launch with id: %d\n", func);
    }

    BalarCudaCallPacket_t *call_packet_ptr = (BalarCudaCallPacket_t *) g_scratch_mem;
    call_packet_ptr->isSSTmem = true;
    call_packet_ptr->cuda_call_id = GPU_LAUNCH;
    call_packet_ptr->cuda_launch.func = func;

    // Make cuda call
    BalarCudaCallReturnPacket_t *response_packet_ptr = makeCudaCall(call_packet_ptr);

    if (g_debug_level >= LOG_LEVEL_DEBUG) {
        printf("CUDA API ID: %d with error: %d\n", 
                response_packet_ptr->cuda_call_id, response_packet_ptr->cuda_error);
    }

    return response_packet_ptr->cuda_error;
}

// With LLVM compiler, need to pass the executable path in config script by setting 
// BALAR_CUDA_EXE_PATH env var, which will overwrite file_name in Balar packet
unsigned int __cudaRegisterFatBinary(void *fatCubin) {
    if (g_debug_level >= LOG_LEVEL_DEBUG) {
        printf("Registering fat binary\n");
        fflush(stdout);
    }

    BalarCudaCallPacket_t *call_packet_ptr = (BalarCudaCallPacket_t *) g_scratch_mem;
    call_packet_ptr->isSSTmem = true;
    call_packet_ptr->cuda_call_id = GPU_REG_FAT_BINARY;

    // Make cuda call
    BalarCudaCallReturnPacket_t *response_packet_ptr = makeCudaCall(call_packet_ptr);

    if (g_debug_level >= LOG_LEVEL_DEBUG) {
        printf("CUDA API ID: %d with error: %d and fatbin handle %d\n", 
                response_packet_ptr->cuda_call_id, response_packet_ptr->cuda_error,
                response_packet_ptr->fat_cubin_handle);
        fflush(stdout);
    }

    return response_packet_ptr->fat_cubin_handle;
}

void __cudaRegisterFunction(
    uint64_t fatCubinHandle,
    uint64_t hostFun,
    char deviceFun[256]
) {
    if (g_debug_level >= LOG_LEVEL_DEBUG) {
        printf("Registering kernel function:\n");
        printf("Fatbin handle: %d, hostFun: %lu, deviceFunc: %s\n",
            fatCubinHandle,
            hostFun,
            deviceFun);
        fflush(stdout);
    }

    BalarCudaCallPacket_t *call_packet_ptr = (BalarCudaCallPacket_t *) g_scratch_mem;
    call_packet_ptr->isSSTmem = true;
    call_packet_ptr->cuda_call_id = GPU_REG_FUNCTION;
    call_packet_ptr->register_function.fatCubinHandle = fatCubinHandle;
    call_packet_ptr->register_function.hostFun = hostFun;
    memcpy(call_packet_ptr->register_function.deviceFun, deviceFun, 256);

    if (g_debug_level >= LOG_LEVEL_DEBUG) {
        printf("Packet's deviceFunc: %s; Addr: %x %x\nScratch mem addr: %x\n",
            call_packet_ptr->register_function.deviceFun,
            &(call_packet_ptr->register_function.deviceFun),
            &deviceFun,
            g_scratch_mem);
        fflush(stdout);
    }

    // Make cuda call
    BalarCudaCallReturnPacket_t *response_packet_ptr = makeCudaCall(call_packet_ptr);

    if (g_debug_level >= LOG_LEVEL_DEBUG) {
        printf("CUDA API ID: %d with error: %d\n", 
                response_packet_ptr->cuda_call_id, response_packet_ptr->cuda_error);
        fflush(stdout);
    }

    return;
}

__host__ cudaError_t CUDARTAPI cudaThreadSynchronize(void) {
    if (g_debug_level >= LOG_LEVEL_DEBUG) {
        printf("Start thread sync\n");
    }

    BalarCudaCallPacket_t *call_packet_ptr = (BalarCudaCallPacket_t *) g_scratch_mem;
    call_packet_ptr->isSSTmem = true;
    call_packet_ptr->cuda_call_id = GPU_THREAD_SYNC;

    // Make cuda call
    BalarCudaCallReturnPacket_t *response_packet_ptr = makeCudaCall(call_packet_ptr);

    // Wait for threadsync to complete
    while (!(response_packet_ptr->is_cuda_call_done)) {
        int wait = 0;
        response_packet_ptr = readLastCudaStatus();
        wait++;
    }

    if (g_debug_level >= LOG_LEVEL_DEBUG) {
        printf("CUDA API ID: %d with error: %d\n", 
                response_packet_ptr->cuda_call_id, response_packet_ptr->cuda_error);
    }

    return response_packet_ptr->cuda_error;
}

__host__ cudaError_t CUDARTAPI cudaGetLastError(void) {
    BalarCudaCallReturnPacket_t *last = readLastCudaStatus();
    return last->cuda_error;
}

__host__ const char *CUDARTAPI cudaGetErrorString(cudaError_t error) {
  if (error == cudaSuccess) return "no error";
  char buf[1024];
  snprintf(buf, 1024, "<<GPGPU-Sim PTX: there was an error (code = %d)>>",
           error);
  return strdup(buf);
}

__host__ cudaError_t CUDARTAPI cudaMemset(void *mem, int c, size_t count) {
    if (g_debug_level >= LOG_LEVEL_DEBUG) {
        printf("Start cudaMemset, setting 0x%llx to %c for %lld bytes\n", mem, c, count);
    }

    BalarCudaCallPacket_t *call_packet_ptr = (BalarCudaCallPacket_t *) g_scratch_mem;
    call_packet_ptr->isSSTmem = true;
    call_packet_ptr->cuda_call_id = GPU_MEMSET;
    call_packet_ptr->cudamemset.mem = mem;
    call_packet_ptr->cudamemset.c = c;
    call_packet_ptr->cudamemset.count = count;

    // Make cuda call
    BalarCudaCallReturnPacket_t *response_packet_ptr = makeCudaCall(call_packet_ptr);

    if (g_debug_level >= LOG_LEVEL_DEBUG) {
        printf("CUDA API ID: %d with error: %d\n", 
                response_packet_ptr->cuda_call_id, response_packet_ptr->cuda_error);
    }

    return response_packet_ptr->cuda_error;
}

// TODO Will there be a problem with the deviceName passed as a pointer?
void __cudaRegisterVar(
    void **fatCubinHandle,
    char *hostVar,           // pointer to...something
    char *deviceAddress,     // name of variable
    const char *deviceName,  // name of variable (same as above)
    int ext, int size, int constant, int global) {
    if (g_debug_level >= LOG_LEVEL_DEBUG) {
        printf("Start registering var\n");
    }

    BalarCudaCallPacket_t *call_packet_ptr = (BalarCudaCallPacket_t *) g_scratch_mem;
    call_packet_ptr->isSSTmem = true;
    call_packet_ptr->cuda_call_id = GPU_REG_VAR;
    call_packet_ptr->register_var.fatCubinHandle = fatCubinHandle; 
    call_packet_ptr->register_var.hostVar = hostVar;
    call_packet_ptr->register_var.deviceAddress = deviceAddress;
    call_packet_ptr->register_var.deviceName = deviceName;
    call_packet_ptr->register_var.ext = ext;
    call_packet_ptr->register_var.size = size;
    call_packet_ptr->register_var.constant = constant;
    call_packet_ptr->register_var.global = global;

    // Make cuda call
    BalarCudaCallReturnPacket_t *response_packet_ptr = makeCudaCall(call_packet_ptr);

    if (g_debug_level >= LOG_LEVEL_DEBUG) {
        printf("CUDA API ID: %d with error: %d\n", 
                response_packet_ptr->cuda_call_id, response_packet_ptr->cuda_error);
    }
}

__host__ cudaError_t CUDARTAPI cudaGetDeviceCount(int *count) {
    if (g_debug_level >= LOG_LEVEL_DEBUG) {
        printf("Start get device count\n");
    }

    BalarCudaCallPacket_t *call_packet_ptr = (BalarCudaCallPacket_t *) g_scratch_mem;
    call_packet_ptr->isSSTmem = true;
    call_packet_ptr->cuda_call_id = GPU_GET_DEVICE_COUNT;

    // Make cuda call
    BalarCudaCallReturnPacket_t *response_packet_ptr = makeCudaCall(call_packet_ptr);

    // Store count back
    *count = response_packet_ptr->cudagetdevicecount.count;

    if (g_debug_level >= LOG_LEVEL_DEBUG) {
        printf("CUDA API ID: %d with error: %d\n", 
                response_packet_ptr->cuda_call_id, response_packet_ptr->cuda_error);
    }

    return response_packet_ptr->cuda_error;
}

__host__ cudaError_t CUDARTAPI cudaSetDevice(int device) {
    if (g_debug_level >= LOG_LEVEL_DEBUG) {
        printf("Start get device count\n");
    }

    BalarCudaCallPacket_t *call_packet_ptr = (BalarCudaCallPacket_t *) g_scratch_mem;
    call_packet_ptr->isSSTmem = true;
    call_packet_ptr->cuda_call_id = GPU_SET_DEVICE;
    call_packet_ptr->cudasetdevice.device = device;

    // Make cuda call
    BalarCudaCallReturnPacket_t *response_packet_ptr = makeCudaCall(call_packet_ptr);

    if (g_debug_level >= LOG_LEVEL_DEBUG) {
        printf("CUDA API ID: %d with error: %d\n", 
                response_packet_ptr->cuda_call_id, response_packet_ptr->cuda_error);
    }

    return response_packet_ptr->cuda_error;
}

__host__ struct cudaChannelFormatDesc CUDARTAPI cudaCreateChannelDesc(
    int x, int y, int z, int w, enum cudaChannelFormatKind f) {
  struct cudaChannelFormatDesc dummy;
  dummy.x = x;
  dummy.y = y;
  dummy.z = z;
  dummy.w = w;
  dummy.f = f;
  return dummy;
}

// TODO Will there be a problem with deviceName passed as a pointer?
// TODO Will there be a problem with the textureReference pointer? Should pass by value?
void __cudaRegisterTexture(
    void **fatCubinHandle, const struct textureReference *hostVar,
    const void **deviceAddress, const char *deviceName, int dim, int norm,
    int ext) {

    if (g_debug_level >= LOG_LEVEL_DEBUG) {
        printf("Start register texture\n");
    }

    BalarCudaCallPacket_t *call_packet_ptr = (BalarCudaCallPacket_t *) g_scratch_mem;
    call_packet_ptr->isSSTmem = true;
    call_packet_ptr->cuda_call_id = GPU_REG_TEXTURE;
    call_packet_ptr->cudaregtexture.fatCubinHandle = fatCubinHandle;
    call_packet_ptr->cudaregtexture.hostVar = hostVar;
    call_packet_ptr->cudaregtexture.deviceAddress = deviceAddress;
    call_packet_ptr->cudaregtexture.deviceName = deviceName;
    call_packet_ptr->cudaregtexture.dim = dim;
    call_packet_ptr->cudaregtexture.norm = norm;
    call_packet_ptr->cudaregtexture.ext = ext;

    // Make cuda call
    BalarCudaCallReturnPacket_t *response_packet_ptr = makeCudaCall(call_packet_ptr);

    if (g_debug_level >= LOG_LEVEL_DEBUG) {
        printf("CUDA API ID: %d with error: %d\n", 
                response_packet_ptr->cuda_call_id, response_packet_ptr->cuda_error);
    }
}

// TODO Will there be a problem with the textureReference pointer? Should pass by value?
__host__ cudaError_t CUDARTAPI cudaBindTexture(
    size_t *offset, const struct textureReference *texref, const void *devPtr,
    const struct cudaChannelFormatDesc *desc, size_t size) {
    if (g_debug_level >= LOG_LEVEL_DEBUG) {
        printf("Start bind texture\n");
    }

    BalarCudaCallPacket_t *call_packet_ptr = (BalarCudaCallPacket_t *) g_scratch_mem;
    call_packet_ptr->isSSTmem = true;
    call_packet_ptr->cuda_call_id = GPU_SET_DEVICE;
    call_packet_ptr->cudabindtexture.offset = offset;
    call_packet_ptr->cudabindtexture.texref = texref;
    call_packet_ptr->cudabindtexture.devPtr = devPtr;
    call_packet_ptr->cudabindtexture.desc = desc;
    call_packet_ptr->cudabindtexture.size = size;

    // Make cuda call
    BalarCudaCallReturnPacket_t *response_packet_ptr = makeCudaCall(call_packet_ptr);

    if (g_debug_level >= LOG_LEVEL_DEBUG) {
        printf("CUDA API ID: %d with error: %d\n", 
                response_packet_ptr->cuda_call_id, response_packet_ptr->cuda_error);
    }

    return response_packet_ptr->cuda_error;
}

__host__ cudaError_t CUDARTAPI cudaFreeHost(void *ptr) {
  free(ptr);
  return cudaSuccess;
}

// A bit tricky as GPGPUSim needs to track the malloc size as well
__host__ cudaError_t CUDARTAPI cudaMallocHost(void **ptr, size_t size) {
    if (g_debug_level >= LOG_LEVEL_DEBUG) {
        printf("Start cuda malloc host with size %lld\n", size);
    }

    // Actual malloc done in vanadis, just need to tell GPGPU-Sim about the
    // malloc addr and size info
    *ptr = malloc(size);
    
    BalarCudaCallPacket_t *call_packet_ptr = (BalarCudaCallPacket_t *) g_scratch_mem;
    call_packet_ptr->isSSTmem = true;
    call_packet_ptr->cuda_call_id = GPU_MALLOC_HOST;
    call_packet_ptr->cudamallochost.addr = *ptr;
    call_packet_ptr->cudamallochost.size = size;

    // Make cuda call
    BalarCudaCallReturnPacket_t *response_packet_ptr = makeCudaCall(call_packet_ptr);

    if (g_debug_level >= LOG_LEVEL_DEBUG) {
        printf("CUDA API ID: %d with error: %d\n", 
                response_packet_ptr->cuda_call_id, response_packet_ptr->cuda_error);
    }
    return response_packet_ptr->cuda_error;
}

// Added these CUDA calls that do nothing, just to make clang happy
void CUDARTAPI __cudaRegisterFatBinaryEnd( void **fatCubinHandle ) {
	return;
}

void __cudaUnregisterFatBinary(void **fatCubinHandle) {
	return;
}

__host__ cudaError_t CUDARTAPI cudaFree(void *devPtr) {
	return cudaSuccess;
}
}