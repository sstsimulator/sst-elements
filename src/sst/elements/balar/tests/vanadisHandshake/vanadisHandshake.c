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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#define VEC_ADD_FUNC 1

int main( int argc, char* argv[] ) {
    printf("Hello from Vanadis and Balar\n");

    // Map balar
    printf("Mapping balar...\n");
    fflush(stdout);
    __vanadisMapBalar();

    // Registering the fatbinary and function
    // Use first argument as the fatbinary path
    unsigned int fatbin_handle = __cudaRegisterFatBinary("./vectorAdd/vectorAdd");
    
    // Need the mangled PTX name here 
    __cudaRegisterFunction(fatbin_handle, VEC_ADD_FUNC, "_Z6vecAddPiS_S_i");

    // Preparing the data
    int n = 10000;
    int total_prints = 5;   // Print 5 times
    int interval = n / total_prints;
    int * d_a = 0;
    int * d_b = 0;
    int * d_result = 0;
    int * h_a = malloc(sizeof(int) * n);
    int * h_b = malloc(sizeof(int) * n);
    int * h_result = malloc(sizeof(int) * n);
    for (int i = 0; i < n; i++) {
        h_a[i] = i;
        h_b[i] = -2 * i;
        if (g_debug_level >= LOG_LEVEL_DEBUG) {
            if (i % interval == 0){
                printf("i:%d h_a[%d] = %d\th_b[%d] = %d\n", i, i, h_a[i], i, h_b[i]);
                fflush(stdout);
            }
        }
    }
    printf("Array allocation done\n");
    fflush(stdout);

    // Prepare device data
    cudaMalloc(&d_a, n * sizeof(int));
    cudaMalloc(&d_b, n * sizeof(int));
    cudaMalloc(&d_result, n * sizeof(int));

    if (g_debug_level >= LOG_LEVEL_DEBUG) {
        printf("d_a: %p\n", d_a);
        printf("d_b: %p\n", d_b);
        printf("d_result: %p\n", d_result);
    }

    // Looks like the 32 bit pointer get signed extended, use AND to force unsigned extend
    cudaMemcpy((uint64_t)d_a & 0xFFFFFFFF, (uint64_t)h_a, n * sizeof(int), cudaMemcpyHostToDevice);
    cudaMemcpy((uint64_t)d_b & 0xFFFFFFFF, (uint64_t)h_b, n * sizeof(int), cudaMemcpyHostToDevice);

    // DEBUG: Check if copy is correct
    if (g_debug_level >= LOG_LEVEL_DEBUG) {
        cudaMemcpy(h_result, (uint64_t)d_a & 0xFFFFFFFF, n * sizeof(int), cudaMemcpyDeviceToHost);
        printf("Checking d_a memcpy content\n");
        for (int i = 0; i < n; i++) {
            if (i % interval == 0) {
                printf("h_result[%d]: %d\n", i, h_result[i]);
                fflush(stdout);
            }
        }
        cudaMemcpy(h_result, (uint64_t)d_b & 0xFFFFFFFF, n * sizeof(int), cudaMemcpyDeviceToHost);
        printf("Checking d_b memcpy content\n");
        for (int i = 0; i < n; i++) {
            if (i % interval == 0) {
                printf("h_result[%d]: %d\n", i, h_result[i]);
                fflush(stdout);
            }
        }
    }
    // DEBUG: END

    // Configure the kernel and arguments
    dim3 gridSize, blockSize;
    blockSize.x = 256;
    blockSize.y = 1;
    blockSize.z = 1;
    gridSize.x = (int) ceil((float) n/blockSize.x);
    gridSize.y = 1;
    gridSize.z = 1;
    uint8_t local_value[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    cudaConfigureCall(gridSize, blockSize, 0);
    cudaSetupArgument((uint64_t)d_a & 0xFFFFFFFF, NULL, 8, 0);
    cudaSetupArgument((uint64_t)d_b & 0xFFFFFFFF, NULL, 8, 8);
    cudaSetupArgument((uint64_t)d_result & 0xFFFFFFFF, NULL, 8, 16);
    memcpy(local_value, &n, 4);
    cudaSetupArgument(NULL, local_value, 4, 24);

    // Calling kernel
    cudaLaunch(VEC_ADD_FUNC);

    // Copy data back
    cudaMemcpy(h_result, (uint64_t)d_result & 0xFFFFFFFF, n * sizeof(int), cudaMemcpyDeviceToHost);

    for (int i = 0; i < n; i++) {
        if (i % interval == 0) {
            printf("h_result[%d]: %d\n", i, h_result[i]);
            fflush(stdout);
        }
    }

    // Weili: CPU validation against GPU results
    if (g_debug_level >= LOG_LEVEL_DEBUG) {
        int * h_cpu = malloc(sizeof(int) * n);
        int err_count = 0;
        for (int i = 0; i < n; i++) {
            h_cpu[i] = h_a[i] + h_b[i];
            if (h_cpu[i] != h_result[i])
                err_count++;
        }
        printf("Error count against cpu result: %d\n", err_count);
        fflush(stdout);
    }
    
    return 0;
}
