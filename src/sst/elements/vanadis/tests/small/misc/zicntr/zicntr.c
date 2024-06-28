#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

uint64_t read_cycles() {
    uint64_t cycles;
    asm volatile ("rdcycle %0" : "=r" (cycles));
    return cycles;
}

uint64_t read_time() {
    uint64_t time;
    asm volatile ("rdtime %0" : "=r" (time));
    return time;
}

uint64_t read_instructions() {
    uint64_t instructions;
    asm volatile ("rdinstret %0" : "=r" (instructions));
    return instructions;
}

int main()
{
    uint64_t cycles = read_cycles();
    uint64_t time = read_time();
    uint64_t instructions = read_instructions();
    printf("cycles: %" PRIu64 " time: %" PRIu64 " instructions: %" PRIu64 "\n", cycles, time, instructions);
}
