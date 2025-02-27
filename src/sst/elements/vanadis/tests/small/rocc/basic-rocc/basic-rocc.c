
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

#include <stdio.h>
#include <stdlib.h> 

int main() {

    // The custom RoCC ADD is an R‑type instruction:
    //   funct7 = 0, rs2 = a1 (11), rs1 = a0 (10), funct3 = 0, rd = a0 (10), opcode = 0x0B.
    // Its 32‑bit encoding is:
    //   (0<<25) | (11<<20) | (10<<15) | (0<<12) | (10<<7) | 0x0B = 0x00B5050B.
    // (Note: a0 is register 10 (0xA) and a1 is register 11 (0xB).)
    int a0_val = 1725049359; // intended to go into register a0 (x10)
    int a1_val = 2010879695;  // intended to go into register a1 (x11)
    asm volatile (
        // Move your C variables into the desired registers.
        "mv a0, %0\n\t"          // a0 <- a0_val
        "mv a1, %1\n\t"          // a1 <- a1_val
        // Insert the custom instruction word.
        ".word 0x00B5050B\n\t"    // This performs: a0 = a0 + a1
        // Move the result from register a0 back to a0_val.
        "mv %0, a0\n\t"
        : "+r" (a0_val)          // a0_val is both input and output.
        : "r" (a1_val)           // a1_val is an input.
        : "a0", "a1", "memory"   // Clobbers: we use a0 and a1.
    );
    printf("Result from RoCC 0: %d\n", a0_val);


    // The custom RoCC SRAI is an R‑type instruction:
    //   funct7 = 1, rs2 = a1 (11), rs1 = a0 (10), funct3 = 0, rd = a0 (10), opcode = 0x0B.
    // Its 32‑bit encoding is:
    //   (1<<25) | (11<<20) | (10<<15) | (0<<12) | (10<<7) | 0x0B = 0x02B5050B.
    // (Note: a0 is register 10 (0xA) and a1 is register 11 (0xB).)
    a0_val = 3435203;
    a1_val = 4;
    asm volatile (
        // Move your C variables into the desired registers.
        "mv a0, %0\n\t"          // a0 <- a0_val
        "mv a1, %1\n\t"          // a1 <- a1_val
        // Insert the custom instruction word.
        ".word 0x02B5050B\n\t"    // This performs: RoCC SRAI 
        // Move the result from register a0 back to a0_val.
        "mv %0, a0\n\t"
        : "+r" (a0_val)          // a0_val is both input and output.
        : "r" (a1_val)           // a1_val is an input.
        : "a0", "a1", "memory"   // Clobbers: we use a0 and a1.
    );
    printf("Result from RoCC 0: %d\n", a0_val);


    // The custom RoCC Load is an R‑type instruction:
    //   funct7 = 2, rs2 = a1 (11), rs1 = a0 (10), funct3 = 0, rd = a0 (10), opcode = 0x0B.
    // Its 32‑bit encoding is:
    //   (2<<25) | (0<<20) | (10<<15) | (0<<12) | (0<<7) | 0x0B = 0x405000B
    // (Note: a0 is register 10 (0xA) and a1 is register 11 (0xB).)
    int *load_value_a = (int *)malloc(sizeof(int));
    load_value_a[0] = 1234;
    asm volatile (
        "mv a0, %0\n\t"          // a0 <- a0_val
        ".word 0x405000B\n\t"    // This performs: RoCC Load
        : 
        : "r" (load_value_a)          // only 1 input.
        : "a0", "memory"   // Clobbers: we use a0
    );
    printf("Loaded value %d in RoCC 0.\n", load_value_a[0]);


    // The custom RoCC Store is an R‑type instruction:
    //   funct7 = 3, rs2 = a1 (11), rs1 = a0 (10), funct3 = 0, rd = a0 (10), opcode = 0x0B.
    // Its 32‑bit encoding is:
    //   (3<<25) | (0<<20) | (10<<15) | (0<<12) | (0<<7) | 0x0B = 0x405000B
    // (Note: a0 is register 10 (0xA) and a1 is register 11 (0xB).)
    int *store_value_a = (int *)malloc(sizeof(int));
    asm volatile (
        "mv a0, %0\n\t"          // a0 <- a0_val
        ".word 0x605000B\n\t"    // This performs: RoCC Load
        : 
        : "r" (store_value_a)          // only 1 input.
        : "a0", "memory"   // Clobbers: we use a0
    );
    printf("Stored value %d from RoCC 0.\n", store_value_a[0]);


    // The custom RoCC ADD is an R‑type instruction:
    //   funct7 = 1, rs2 = a1 (11), rs1 = a0 (10), funct3 = 0, rd = a0 (10), opcode = 0x2B.
    // Its 32‑bit encoding is:
    //   (0<<25) | (11<<20) | (10<<15) | (0<<12) | (10<<7) | 0x2B = 0x00B5052B.
    // (Note: a0 is register 10 (0xA) and a1 is register 11 (0xB).)
    a0_val = 1735928559; // intended to go into register a0 (x10)
    a1_val = 2000000000;  // intended to go into register a1 (x11)
    asm volatile (
        // Move your C variables into the desired registers.
        "mv a0, %0\n\t"          // a0 <- a0_val
        "mv a1, %1\n\t"          // a1 <- a1_val
        // Insert the custom instruction word.
        ".word 0xB5052B\n\t"    // This performs: a0 = a0 >> a1
        // Move the result from register a0 back to a0_val.
        "mv %0, a0\n\t"
        : "+r" (a0_val)          // a0_val is both input and output.
        : "r" (a1_val)           // a1_val is an input.
        : "a0", "a1", "memory"   // Clobbers: we use a0 and a1.
    );
    printf("Result from RoCC 1: %d\n", a0_val);


    // The custom RoCC SRAI is an R‑type instruction:
    //   funct7 = 1, rs2 = a1 (11), rs1 = a0 (10), funct3 = 0, rd = a0 (10), opcode = 0x2B.
    // Its 32‑bit encoding is:
    //   (1<<25) | (11<<20) | (10<<15) | (0<<12) | (10<<7) | 0x2B = 0x02B5052B.
    // (Note: a0 is register 10 (0xA) and a1 is register 11 (0xB).)
    a0_val = 6325892;
    a1_val = 4;
    asm volatile (
        // Move your C variables into the desired registers.
        "mv a0, %0\n\t"          // a0 <- a0_val
        "mv a1, %1\n\t"          // a1 <- a1_val
        // Insert the custom instruction word.
        ".word 0x02B5052B\n\t"    // This performs: a0 = a0 + a1
        // Move the result from register a0 back to a0_val.
        "mv %0, a0\n\t"
        : "+r" (a0_val)          // a0_val is both input and output.
        : "r" (a1_val)           // a1_val is an input.
        : "a0", "a1", "memory"   // Clobbers: we use a0 and a1.
    );
    printf("Result from RoCC 1: %d\n", a0_val);


    // The custom RoCC Load is an R‑type instruction:
    //   funct7 = 2, rs2 = a1 (11), rs1 = a0 (10), funct3 = 0, rd = a0 (10), opcode = 0x2B.
    // Its 32‑bit encoding is:
    //   (2<<25) | (0<<20) | (10<<15) | (0<<12) | (0<<7) | 0x2B = 0x405002B
    // (Note: a0 is register 10 (0xA) and a1 is register 11 (0xB).)
    int *load_value_b = (int *)malloc(sizeof(int));
    load_value_b[0] = 4321;
    asm volatile (
        "mv a0, %0\n\t"          // a0 <- a0_val
        ".word 0x405002B\n\t"    // This performs: RoCC Load
        : 
        : "r" (load_value_b)          // only 1 input.
        : "a0", "memory"   // Clobbers: we use a0
    );
    printf("Loaded value %d in RoCC 1.\n", load_value_b[0]);


    // The custom RoCC Store is an R‑type instruction:
    //   funct7 = 3, rs2 = a1 (11), rs1 = a0 (10), funct3 = 0, rd = a0 (10), opcode = 0x2B.
    // Its 32‑bit encoding is:
    //   (3<<25) | (0<<20) | (10<<15) | (0<<12) | (0<<7) | 0x2B = 0x405002B
    // (Note: a0 is register 10 (0xA) and a1 is register 11 (0xB).)
    int *store_value_b = (int *)malloc(sizeof(int));
    asm volatile (
        "mv a0, %0\n\t"          // a0 <- a0_val
        ".word 0x605002B\n\t"    // This performs: RoCC Load
        : 
        : "r" (store_value_b)          // only 1 input.
        : "a0", "memory"   // Clobbers: we use a0
    );
    printf("Stored value %d from RoCC 1.\n", store_value_b[0]);
}
