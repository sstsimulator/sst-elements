#ifndef MVM_H
#define MVM_H

#include <stdint.h>

/*#define MUL_FACTOR 256

void normalize(float* a, int* b, float* max_a) {
    float current_max = 0;
    // find max value in a
    for(int i = 0; i < SIZE; i++) {
        if(a[i] > current_max) {
            current_max = a[i];
        }
    }
    *max_a = current_max;
    // normalize
    for(int i = 0; i < SIZE; i++) {
        b[i] = (int)(a[i] / current_max * MUL_FACTOR);
    }
}

void denormalize(int* b, float* a, float max_a) {
    // denormalize
    for(int i = 0; i < SIZE; i++) {
        a[i] = (float)b[i] / MUL_FACTOR * max_a;
    }
}

*/

uint16_t mvm_set(float *matrix, uint16_t array_id) {
    uint16_t success;
    __asm__ __volatile__ ("mvm.set %0, %1, %2" : "=r"(success) : "r"(matrix), "r"(array_id) : "memory");
    return success;
}

uint16_t mvm_load(float *vector, uint16_t array_id) {
    uint16_t success;
    __asm__ __volatile__ ("mvm.l %0, %1, %2" : "=r"(success) : "r"(vector), "r"(array_id) : "memory");
    return success;
}

uint16_t mvm_compute(uint16_t array_id) {
    uint16_t success;
    __asm__ __volatile__ ("mvm %0, %1, x0" : "=r"(success) : "r"(array_id));
    return success;
}

uint16_t mvm_store(float *vector, uint16_t array_id) {
    uint16_t success;
    __asm__ __volatile__ ("mvm.s %0, %1, %2" : "=r"(success) : "r"(vector), "r"(array_id) : "memory");
    return success;
}

uint16_t mvm_move(uint16_t array_id_a, uint16_t array_id_b) {
    uint16_t success;
    __asm__ __volatile__ ("mvm.mv %0, %1, %2" : "=r"(success) : "r"(array_id_a), "r"(array_id_b));
    return success;
}
#endif
