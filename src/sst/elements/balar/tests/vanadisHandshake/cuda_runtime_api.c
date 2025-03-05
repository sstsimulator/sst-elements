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

#include "cuda_runtime_api.h"
#include "../../balar_consts.h"
#include <sys/syscall.h>
#include <sys/mman.h>

cudaError_t cudaMalloc(void **devPtr, uint64_t size) {
    // Send request to GPU
    BalarCudaCallPacket_t *call_packet_ptr = (BalarCudaCallPacket_t *) g_scratch_mem;
    call_packet_ptr->isSSTmem = true;
    call_packet_ptr->cuda_call_id = CUDA_MALLOC;
    call_packet_ptr->cuda_malloc.size = size;
    call_packet_ptr->cuda_malloc.devPtr = devPtr;

    if (g_debug_level >= LOG_LEVEL_DEBUG) {
        printf("Malloc Packet address: %p\n", call_packet_ptr);
        printf("Malloc Packet size: %lu\n", size);
        printf("Malloc Packet devptr: %p\n", devPtr);
    }

    // Vanadis write with 4B chunk, thus use uint32_t to pass
    // the pointer to the balar packet
    __sync_synchronize();
    *g_balarBaseAddr = (uint32_t) call_packet_ptr;
    __sync_synchronize();
    
    // Read from GPU will return the address to the cuda return packet
    BalarCudaCallReturnPacket_t *response_packet_ptr = (BalarCudaCallReturnPacket_t *)*g_balarBaseAddr;
    __sync_synchronize();

    if (g_debug_level >= LOG_LEVEL_DEBUG) {
        printf("CUDA API ID: %d with error: %d\nMalloc addr: %lu Dev addr: %lu\n", 
                response_packet_ptr->cuda_call_id, response_packet_ptr->cuda_error,
                response_packet_ptr->cudamalloc.malloc_addr, response_packet_ptr->cudamalloc.devptr_addr);
    }

    *devPtr = response_packet_ptr->cudamalloc.malloc_addr;
    return response_packet_ptr->cuda_error;
}

cudaError_t cudaMemcpy(uint64_t dst, uint64_t src, uint64_t count, enum cudaMemcpyKind kind) {
    // Send request to GPU
    if (g_debug_level >= LOG_LEVEL_DEBUG) {
        printf("Memcpy dst: %llx\n", dst);
    }
    BalarCudaCallPacket_t *call_packet_ptr = (BalarCudaCallPacket_t *) g_scratch_mem;
    call_packet_ptr->isSSTmem = true;
    call_packet_ptr->cuda_call_id = CUDA_MEMCPY;

    call_packet_ptr->cuda_memcpy.kind = kind;
    call_packet_ptr->cuda_memcpy.dst = dst;
    call_packet_ptr->cuda_memcpy.src = src;
    call_packet_ptr->cuda_memcpy.count = count;
    call_packet_ptr->cuda_memcpy.payload = (uint64_t) src;

    if (g_debug_level >= LOG_LEVEL_DEBUG) {
        printf("Memcpy Packet address: %p\n", call_packet_ptr);
        printf("Memcpy kind: %d\n", (uint8_t) (call_packet_ptr->cuda_memcpy.kind));
        printf("Memcpy size: %ld\n", count);
        printf("Memcpy src: %p\n", src);
        printf("Memcpy dst: %p\n", dst);
        // printf("sizeof: %d\n", sizeof(call_packet_ptr->cuda_memcpy.kind));
    }

    // Vanadis write with 4B chunk, thus use uint32_t to pass
    // the pointer to the balar packet
    __sync_synchronize();
    *g_balarBaseAddr = (uint32_t) call_packet_ptr;
    __sync_synchronize();

    // Read from GPU will return the address to the cuda return packet
    // Make the memcpy sync with balar so that CPU will
    // issue another write/read to balar only if the previous memcpy is done
    BalarCudaCallReturnPacket_t *response_packet_ptr;

    while (1) {
        int wait = 0;
        response_packet_ptr = (BalarCudaCallReturnPacket_t *)*g_balarBaseAddr;
        if (response_packet_ptr->is_cuda_call_done)
            break;
        wait++;

        // if (wait % 10 == 0) {
        //     printf("Waited %d times\n", wait);
        // }
    }
    __sync_synchronize();


    if (g_debug_level >= LOG_LEVEL_DEBUG) {
        printf("CUDA API ID: %d with error: %d\n", 
                response_packet_ptr->cuda_call_id, response_packet_ptr->cuda_error);
    }
    
    return response_packet_ptr->cuda_error;
}

// Cuda Configure call
cudaError_t cudaConfigureCall(dim3 gridDim, dim3 blockDim, uint64_t sharedMem) {
    BalarCudaCallPacket_t *call_packet_ptr = (BalarCudaCallPacket_t *) g_scratch_mem;
    call_packet_ptr->isSSTmem = true;
    call_packet_ptr->cuda_call_id = CUDA_CONFIG_CALL;

    // Prepare packet
    call_packet_ptr->configure_call.gdx = gridDim.x;
    call_packet_ptr->configure_call.gdy = gridDim.y;
    call_packet_ptr->configure_call.gdz = gridDim.z;
    call_packet_ptr->configure_call.bdx = blockDim.x;
    call_packet_ptr->configure_call.bdy = blockDim.y;
    call_packet_ptr->configure_call.bdz = blockDim.z;
    call_packet_ptr->configure_call.sharedMem = sharedMem;
    call_packet_ptr->configure_call.stream = NULL;

    if (g_debug_level >= LOG_LEVEL_DEBUG) {
        printf("Start Configure Call:\n");
        printf("Grid Dim: x(%d) y(%d) z(%d)\n", gridDim.x, gridDim.y, gridDim.z);
        printf("Block Dim: x(%d) y(%d) z(%d)\n", blockDim.x, blockDim.y, blockDim.z);
    }

    // Vanadis write with 4B chunk, thus use uint32_t to pass
    // the pointer to the balar packet
    __sync_synchronize();
    *g_balarBaseAddr = (uint32_t) call_packet_ptr;
    __sync_synchronize();
    
    // Read from GPU will return the address to the cuda return packet
    BalarCudaCallReturnPacket_t *response_packet_ptr = (BalarCudaCallReturnPacket_t *)*g_balarBaseAddr;
    __sync_synchronize();
    
    if (g_debug_level >= LOG_LEVEL_DEBUG) {
        printf("CUDA API ID: %d with error: %d\n", 
                response_packet_ptr->cuda_call_id, response_packet_ptr->cuda_error);
    }
    
    return response_packet_ptr->cuda_error;
}

// Cuda Setup argument
cudaError_t cudaSetupArgument(uint64_t arg, uint8_t value[BALAR_CUDA_MAX_ARG_SIZE], uint64_t size, uint64_t offset) {
    if (g_debug_level >= LOG_LEVEL_DEBUG) {
        printf("Start setup argument:\n");
        printf("Size: %d offset: %d\n", size, offset);
    }

    BalarCudaCallPacket_t *call_packet_ptr = (BalarCudaCallPacket_t *) g_scratch_mem;
    call_packet_ptr->isSSTmem = true;
    call_packet_ptr->cuda_call_id = CUDA_SET_ARG;
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
        call_packet_ptr->setup_argument.arg = NULL;
        for (int i = 0; i < 8; i++) {
            call_packet_ptr->setup_argument.value[i] = value[i];
        }
    }

    // Vanadis write with 4B chunk, thus use uint32_t to pass
    // the pointer to the balar packet
    __sync_synchronize();
    *g_balarBaseAddr = (uint32_t) call_packet_ptr;
    __sync_synchronize();

    // Read from GPU will return the address to the cuda return packet
    BalarCudaCallReturnPacket_t *response_packet_ptr = (BalarCudaCallReturnPacket_t *)*g_balarBaseAddr;
    __sync_synchronize();

    if (g_debug_level >= LOG_LEVEL_DEBUG) {
        printf("CUDA API ID: %d with error: %d\n", 
                response_packet_ptr->cuda_call_id, response_packet_ptr->cuda_error);
    }
    
    return response_packet_ptr->cuda_error;
}

cudaError_t cudaLaunch(uint64_t func) {
    if (g_debug_level >= LOG_LEVEL_DEBUG) {
        printf("Start kernel launch with id: %d\n", func);
    }

    BalarCudaCallPacket_t *call_packet_ptr = (BalarCudaCallPacket_t *) g_scratch_mem;
    call_packet_ptr->isSSTmem = true;
    call_packet_ptr->cuda_call_id = CUDA_LAUNCH;
    call_packet_ptr->cuda_launch.func = func;

    // Vanadis write with 4B chunk, thus use uint32_t to pass
    // the pointer to the balar packet
    __sync_synchronize();
    *g_balarBaseAddr = (uint32_t) call_packet_ptr;
    __sync_synchronize();

    // Read from GPU will return the address to the cuda return packet
    BalarCudaCallReturnPacket_t *response_packet_ptr = (BalarCudaCallReturnPacket_t *)*g_balarBaseAddr;
    __sync_synchronize();

    if (g_debug_level >= LOG_LEVEL_DEBUG) {
        printf("CUDA API ID: %d with error: %d\n", 
                response_packet_ptr->cuda_call_id, response_packet_ptr->cuda_error);
    }
    
    return response_packet_ptr->cuda_error;
}

void __vanadisMapBalar() {
    #ifdef SYS_mmap2
        g_balarBaseAddr = (Addr_t*) syscall(SYS_mmap2, 0, 0, PROT_WRITE|PROT_READ, 0, -2000, 0);
    #else
        g_balarBaseAddr = (Addr_t*) syscall(SYS_mmap, 0, 0, PROT_WRITE|PROT_READ, 0, -2000, 0);
    #endif

    if (g_debug_level >= LOG_LEVEL_DEBUG) {
        printf("Mapping balar to address: %x\n", g_balarBaseAddr);
        fflush(stdout);
    }
}

unsigned int __cudaRegisterFatBinary(char file_name[BALAR_CUDA_MAX_FILE_NAME]) {
    if (g_debug_level >= LOG_LEVEL_DEBUG) {
        printf("Registering fat binary: %s\n", file_name);
        fflush(stdout);
    }

    BalarCudaCallPacket_t *call_packet_ptr = (BalarCudaCallPacket_t *) g_scratch_mem;
    call_packet_ptr->isSSTmem = true;
    call_packet_ptr->cuda_call_id = CUDA_REG_FAT_BINARY;
    memcpy(call_packet_ptr->register_fatbin.file_name, file_name, 256);

    // Vanadis write with 4B chunk, thus use uint32_t to pass
    // the pointer to the balar packet
    __sync_synchronize();
    *g_balarBaseAddr = (uint32_t) call_packet_ptr;
    __sync_synchronize();

    // Read from GPU will return the address to the cuda return packet
    BalarCudaCallReturnPacket_t *response_packet_ptr = (BalarCudaCallReturnPacket_t *)*g_balarBaseAddr;
    __sync_synchronize();

    if (g_debug_level >= LOG_LEVEL_DEBUG) {
        printf("CUDA API ID: %d with error: %d and fatbin handle %d\n", 
                response_packet_ptr->cuda_call_id, response_packet_ptr->cuda_error,
                response_packet_ptr->fat_cubin_handle);
        fflush(stdout);
    }
    
    return response_packet_ptr->fat_cubin_handle;
}

// Future: How to get the deviceFun name automatically?
// Future: Requires parsing the binary?
void __cudaRegisterFunction(
    uint64_t fatCubinHandle,
    uint64_t hostFun,
    char deviceFun[BALAR_CUDA_MAX_KERNEL_NAME]
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
    call_packet_ptr->cuda_call_id = CUDA_REG_FUNCTION;
    call_packet_ptr->register_function.fatCubinHandle = fatCubinHandle;
    call_packet_ptr->register_function.hostFun = hostFun;
    memcpy(call_packet_ptr->register_function.deviceFun, deviceFun, 256);

    // Vanadis write with 4B chunk, thus use uint32_t to pass
    // the pointer to the balar packet
    __sync_synchronize();
    *g_balarBaseAddr = (uint32_t) call_packet_ptr;
    __sync_synchronize();

    // Read from GPU will return the address to the cuda return packet
    BalarCudaCallReturnPacket_t *response_packet_ptr = (BalarCudaCallReturnPacket_t *)*g_balarBaseAddr;
    __sync_synchronize();

    if (g_debug_level >= LOG_LEVEL_DEBUG) {
        printf("CUDA API ID: %d with error: %d\n", 
                response_packet_ptr->cuda_call_id, response_packet_ptr->cuda_error);
        fflush(stdout);
    }
    
    return;
}
