/**
 * @file vanadisHandshake.c
 * @author Weili An (an107@purdue.edu)
 * @brief A simple program to test balar and vanadis handshake
 *        by writing packet to the MMIO address of balar
 * @version 0.1
 * @date 2022-06-01
 * 
 */

#include "cuda_runtime_api.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>

#define VEC_ADD_FUNC 1

int main( int argc, char* argv[] ) {
    printf("Hello from Vanadis and Balar\n");

    // Registering the fatbinary and function
    // Use first argument as the fatbinary path
    unsigned int fatbin_handle = __cudaRegisterFatBinary("./vectorAdd/vectorAdd");
    __cudaRegisterFunction(fatbin_handle, VEC_ADD_FUNC, "_Z6vecAddPiS_S_i");

    // Preparing the data
    int n = 200;
    int * d_a = 0;
    int * d_b = 0;
    int * d_result = 0;
    int * h_a = malloc(sizeof(int) * n);
    int * h_b = malloc(sizeof(int) * n);
    int * h_result = malloc(sizeof(int) * n);
    for (int i = 0; i < n; i++) {
        h_a[i] = i;
        h_b[i] = -2 * i;
        if (i % 30 == 0){
            printf("i:%d h_a[%d] = %d\th_b[%d] = %d\n", i, i, h_a[i], i, h_b[i]);
            fflush(stdout);
        }
    }

    // Prepare device data
    cudaMalloc(&d_a, n * sizeof(int));
    cudaMalloc(&d_b, n * sizeof(int));
    cudaMalloc(&d_result, n * sizeof(int));

    cudaMemcpy(d_a, h_a, n * sizeof(int), cudaMemcpyHostToDevice);
    cudaMemcpy(d_b, h_b, n * sizeof(int), cudaMemcpyHostToDevice);

    // DEBUG: Check if copy is correct
    cudaMemcpy(h_result, d_a, n * sizeof(int), cudaMemcpyDeviceToHost);
    printf("Checking d_a memcpy content\n");
    for (int i = 0; i < n; i++) {
        if (i % 30 == 0) {
            printf("h_result[%d]: %d\n", i, h_result[i]);
            fflush(stdout);
        }
    }
    cudaMemcpy(h_result, d_b, n * sizeof(int), cudaMemcpyDeviceToHost);
    printf("Checking d_b memcpy content\n");
    for (int i = 0; i < n; i++) {
        if (i % 30 == 0) {
            printf("h_result[%d]: %d\n", i, h_result[i]);
            fflush(stdout);
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
    uint8_t local_value[8];
    cudaConfigureCall(gridSize, blockSize, 0);
    cudaSetupArgument(d_a, NULL, 8, 0);
    cudaSetupArgument(d_b, NULL, 8, 8);
    cudaSetupArgument(d_result, NULL, 8, 16);
    memcpy(local_value, &n, 4);
    cudaSetupArgument(NULL, local_value, 4, 24);

    // Calling kernel
    cudaLaunch(VEC_ADD_FUNC);

    // Copy data back
    cudaMemcpy(h_result, d_result, n * sizeof(int), cudaMemcpyDeviceToHost);

    for (int i = 0; i < n; i++) {
        if (i % 30 == 0) {
            printf("h_result[%d]: %d\n", i, h_result[i]);
            fflush(stdout);
        }
    }
    
    return 0;
}