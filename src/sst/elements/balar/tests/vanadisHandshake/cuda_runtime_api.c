#include "cuda_runtime_api.h"

// TODO: DMA engine timing issue when removing the debug statements
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

    // Vanadis write with 4B chunk, thus use uint32_t to pass
    // the pointer to the balar packet
    *g_gpu = (uint32_t) call_packet_ptr;

    // Read from GPU will return the address to the cuda return packet
    BalarCudaCallReturnPacket_t *response_packet_ptr = (BalarCudaCallReturnPacket_t *)*g_gpu;

    if (g_debug_level >= LOG_LEVEL_DEBUG) {
        printf("CUDA API ID: %d with error: %d\nMalloc addr: %lu Dev addr: %lu\n", 
                response_packet_ptr->cuda_call_id, response_packet_ptr->cuda_error,
                response_packet_ptr->cudamalloc.malloc_addr, response_packet_ptr->cudamalloc.devptr_addr);
    }

    // TODO: Malloc address incorrect? malloc_addr or devptr_addr?
    *devPtr = response_packet_ptr->cudamalloc.malloc_addr;
    return response_packet_ptr->cuda_error;
}

cudaError_t cudaMemcpy(uint64_t dst, uint64_t src, uint64_t count, enum cudaMemcpyKind kind) {
    // Send request to GPU
    BalarCudaCallPacket_t *call_packet_ptr = (BalarCudaCallPacket_t *) g_scratch_mem;
    call_packet_ptr->isSSTmem = true;
    call_packet_ptr->cuda_call_id = GPU_MEMCPY;

    call_packet_ptr->cuda_memcpy.kind = kind;
    call_packet_ptr->cuda_memcpy.dst = dst;
    call_packet_ptr->cuda_memcpy.src = src;
    call_packet_ptr->cuda_memcpy.count = count;
    call_packet_ptr->cuda_memcpy.payload = (uint64_t) src;

    if (g_debug_level >= LOG_LEVEL_DEBUG) {
        printf("Memcpy Packet address: %p\n", call_packet_ptr);
        printf("Memcpy kind: %d\n", (uint8_t) (call_packet_ptr->cuda_memcpy.kind));
        printf("Memcpy size: %ld\n", count);
        printf("Memcpy src: %d\n", src);
        printf("Memcpy dst: %d\n", dst);
        // printf("sizeof: %d\n", sizeof(call_packet_ptr->cuda_memcpy.kind));
    }

    // Vanadis write with 4B chunk, thus use uint32_t to pass
    // the pointer to the balar packet
    *g_gpu = (uint32_t) call_packet_ptr;

    // Read from GPU will return the address to the cuda return packet
    // Make the memcpy sync with balar so that CPU will
    // issue another write/read to balar only if the previous memcpy is done
    BalarCudaCallReturnPacket_t *response_packet_ptr;
    while (1) {
        int wait = 0;
        response_packet_ptr = (BalarCudaCallReturnPacket_t *)*g_gpu;
        if (response_packet_ptr->is_cuda_call_done)
            break;
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

// TODO Need to register fatbin now
// TODO Need to register function
// TODO Also need to handle kernel arguments

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
    call_packet_ptr->configure_call.stream = NULL;

    if (g_debug_level >= LOG_LEVEL_DEBUG) {
        printf("Start Configure Call:\n");
        printf("Grid Dim: x(%d) y(%d) z(%d)\n", gridDim.x, gridDim.y, gridDim.z);
        printf("Block Dim: x(%d) y(%d) z(%d)\n", blockDim.x, blockDim.y, blockDim.z);
    }

    // Vanadis write with 4B chunk, thus use uint32_t to pass
    // the pointer to the balar packet
    *g_gpu = (uint32_t) call_packet_ptr;

    // Read from GPU will return the address to the cuda return packet
    BalarCudaCallReturnPacket_t *response_packet_ptr = (BalarCudaCallReturnPacket_t *)*g_gpu;

    if (g_debug_level >= LOG_LEVEL_DEBUG) {
        printf("CUDA API ID: %d with error: %d\n", 
                response_packet_ptr->cuda_call_id, response_packet_ptr->cuda_error);
    }
    
    return response_packet_ptr->cuda_error;
}

// Cuda Setup argument
cudaError_t cudaSetupArgument(uint64_t arg, uint8_t value[8], uint64_t size, uint64_t offset) {
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
        call_packet_ptr->setup_argument.arg = NULL;
        for (int i = 0; i < 8; i++) {
            call_packet_ptr->setup_argument.value[i] = value[i];
        }
    }

    // Vanadis write with 4B chunk, thus use uint32_t to pass
    // the pointer to the balar packet
    *g_gpu = (uint32_t) call_packet_ptr;

    // Read from GPU will return the address to the cuda return packet
    BalarCudaCallReturnPacket_t *response_packet_ptr = (BalarCudaCallReturnPacket_t *)*g_gpu;

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
    call_packet_ptr->cuda_call_id = GPU_LAUNCH;
    call_packet_ptr->cuda_launch.func = func;

    // Vanadis write with 4B chunk, thus use uint32_t to pass
    // the pointer to the balar packet
    *g_gpu = (uint32_t) call_packet_ptr;

    // Read from GPU will return the address to the cuda return packet
    BalarCudaCallReturnPacket_t *response_packet_ptr = (BalarCudaCallReturnPacket_t *)*g_gpu;

    if (g_debug_level >= LOG_LEVEL_DEBUG) {
        printf("CUDA API ID: %d with error: %d\n", 
                response_packet_ptr->cuda_call_id, response_packet_ptr->cuda_error);
    }
    
    return response_packet_ptr->cuda_error;
}

unsigned int __cudaRegisterFatBinary(char file_name[256]) {
    if (g_debug_level >= LOG_LEVEL_DEBUG) {
        printf("Registering fat binary: %s\n", file_name);
    }

    BalarCudaCallPacket_t *call_packet_ptr = (BalarCudaCallPacket_t *) g_scratch_mem;
    call_packet_ptr->isSSTmem = true;
    call_packet_ptr->cuda_call_id = GPU_REG_FAT_BINARY;
    memcpy(call_packet_ptr->register_fatbin.file_name, file_name, 256);

    // Vanadis write with 4B chunk, thus use uint32_t to pass
    // the pointer to the balar packet
    *g_gpu = (uint32_t) call_packet_ptr;

    // Read from GPU will return the address to the cuda return packet
    BalarCudaCallReturnPacket_t *response_packet_ptr = (BalarCudaCallReturnPacket_t *)*g_gpu;

    if (g_debug_level >= LOG_LEVEL_DEBUG) {
        printf("CUDA API ID: %d with error: %d and fatbin handle %d\n", 
                response_packet_ptr->cuda_call_id, response_packet_ptr->cuda_error,
                response_packet_ptr->fat_cubin_handle);
    }
    
    return response_packet_ptr->fat_cubin_handle;
}

// TODO: How to get the deviceFun name?
// TODO: Requires parsing the binary?
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
    }

    BalarCudaCallPacket_t *call_packet_ptr = (BalarCudaCallPacket_t *) g_scratch_mem;
    call_packet_ptr->isSSTmem = true;
    call_packet_ptr->cuda_call_id = GPU_REG_FUNCTION;
    call_packet_ptr->register_function.fatCubinHandle = fatCubinHandle;
    call_packet_ptr->register_function.hostFun = hostFun;
    memcpy(call_packet_ptr->register_function.deviceFun, deviceFun, 256);

    // Vanadis write with 4B chunk, thus use uint32_t to pass
    // the pointer to the balar packet
    *g_gpu = (uint32_t) call_packet_ptr;

    // Read from GPU will return the address to the cuda return packet
    BalarCudaCallReturnPacket_t *response_packet_ptr = (BalarCudaCallReturnPacket_t *)*g_gpu;

    if (g_debug_level >= LOG_LEVEL_DEBUG) {
        printf("CUDA API ID: %d with error: %d\n", 
                response_packet_ptr->cuda_call_id, response_packet_ptr->cuda_error);
    }
    
    return;
}