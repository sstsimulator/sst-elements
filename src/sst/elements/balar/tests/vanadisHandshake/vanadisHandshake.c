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


int main( int argc, char* argv[] ) {
    printf("Hello from Vanadis and Balar\n");

    uint8_t * d_a = 0;
    uint8_t * d_b = 0;
    uint8_t * h_a = malloc(sizeof(uint8_t) * 200);
    uint8_t * h_b = malloc(sizeof(uint8_t) * 200);
    for (int i = 0; i < 200; i++) {
        h_a[i] = i;
    }

    printf("d_a: %p\n", d_a);

    cudaMalloc(&d_a, 200);
    cudaMalloc(&d_b, 200);
    printf("sizeof: %d %d\n", sizeof(d_a), sizeof(h_a));
    printf("d_a: %p\n", d_a);
    printf("d_b: %p\n", d_b);

    cudaMemcpy(d_a, h_a, 200 * sizeof(uint8_t), cudaMemcpyHostToDevice);
    cudaMemcpy(h_b, d_a, 200 * sizeof(uint8_t), cudaMemcpyDeviceToHost);

    for (int i = 0; i < 200; i++) {
        printf("h_b[%d]: %d\n", i, h_b[i]);
    }
    return 0;
}