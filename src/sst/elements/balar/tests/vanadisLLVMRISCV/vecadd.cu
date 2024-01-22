#include <stdio.h>
#include <stdlib.h>

__global__
void helloWorld(int* src, int N) {
    for (int i = 0; i < N; i++) {
        src[i] = i + i;
    }
}

int main() {
    int* src;
    int* src_device;
    int N = 32;

    src = (int*)malloc(N * sizeof(int));
    cudaMalloc(&src_device, N*sizeof(int));

    for(int i = 0; i < N; i++) {
        src[i] = i;
    }

    cudaMemcpy(src_device, src, N*sizeof(int), cudaMemcpyHostToDevice);

        helloWorld<<<1, 1, 1>>>(src_device, N);

    cudaMemcpy(src, src_device, N*sizeof(int), cudaMemcpyDeviceToHost);

    for(int i = 0; i < N; i++) {
        printf("src[%d] = %d\n", i, src[i]);
    }

    return 0;
}
