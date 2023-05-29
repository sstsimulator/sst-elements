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

#include <stdio.h>
#include <stdint.h>

void* allocate_memory_at_address(uint32_t size, void*  address) {
    void* ptr;

#ifdef MIPSEL
    asm volatile (
        "move $a0, %[size] \n"
        "move $a1, %[address] \n"
        "li $v0, 4210 \n"
        "syscall \n"
        "move %[ptr], $v0 \n"
        : [ptr] "=r" (ptr)
        : [size] "r" (size), [address] "r" (address)
        : "$v0", "$a0", "$a1"
    );
#else
    asm volatile (
        "mv a0, %[size] \n"
        "mv a1, %[address] \n"
        "li a7, 222 \n"
        "ecall \n"
        "mv %[ptr], a0 \n"
        : [ptr] "=r" (ptr)
        : [size] "r" (size), [address] "r" (address)
        : "a0", "a1", "a7"
    );
#endif
    return ptr;
}

int main() {
    void* address = (void*)0xC000;
    uint32_t size = 4096;

    void* ptr = allocate_memory_at_address(size, address);

    if (ptr != NULL) {
        printf("Memory allocated at address: %p\n", ptr);
    } else {
        printf("Memory allocation failed\n");
    }

    return 0;
}
