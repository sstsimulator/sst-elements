// Copyright 2009-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2026, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

/*
 * Hyades MMIO ABI for Vanadis+Hali (cmd@+0, status@+4). Default base 0xBEEF0000.
 */
#ifndef HYADES_H
#define HYADES_H

#include <stdlib.h>

#ifndef HYADES_MMIO_BASE
#define HYADES_MMIO_BASE  0xBEEF0000
#endif

#define HYADES_COMMAND_OFFSET       0
#define HYADES_STATUS_OFFSET        4
#define HYADES_SEQ_LEN_OFFSET       8
/* Region-publish ABI: write base_lo/hi, size, then COMMIT; agent latches VAs. */
#define HYADES_REGION_BASE_LO_OFFSET  0x40
#define HYADES_REGION_BASE_HI_OFFSET  0x80
#define HYADES_REGION_SIZE_OFFSET     0xC0
#define HYADES_REGION_COMMIT_OFFSET   0x100
/* Action checksum LO (commit). Publish HI then LO after ACTUATE through EccGuard. */
#define HYADES_ACTION_CHECKSUM_OFFSET 0x140
/* Decoded-action token: quantized fingerprint; ActionScorer headline metric. */
#define HYADES_ACTION_TOKEN_OFFSET    0x180
/* Action-checksum HI; write before LO so latch-on-LO agents see a coherent 64-bit. */
#define HYADES_ACTION_CHECKSUM_HI_OFFSET 0x1C0

#define HYADES_COMMAND  ((volatile int *)(HYADES_MMIO_BASE + HYADES_COMMAND_OFFSET))
#define HYADES_STATUS   ((volatile int *)(HYADES_MMIO_BASE + HYADES_STATUS_OFFSET))
#define HYADES_SEQ_LEN  ((volatile int *)(HYADES_MMIO_BASE + HYADES_SEQ_LEN_OFFSET))

#define HYADES_REGION_BASE_LO  ((volatile unsigned int *)(HYADES_MMIO_BASE + HYADES_REGION_BASE_LO_OFFSET))
#define HYADES_REGION_BASE_HI  ((volatile unsigned int *)(HYADES_MMIO_BASE + HYADES_REGION_BASE_HI_OFFSET))
#define HYADES_REGION_SIZE     ((volatile unsigned int *)(HYADES_MMIO_BASE + HYADES_REGION_SIZE_OFFSET))
#define HYADES_REGION_COMMIT   ((volatile int          *)(HYADES_MMIO_BASE + HYADES_REGION_COMMIT_OFFSET))
#define HYADES_ACTION_CHECKSUM ((volatile unsigned int *)(HYADES_MMIO_BASE + HYADES_ACTION_CHECKSUM_OFFSET))
#define HYADES_ACTION_CHECKSUM_HI ((volatile unsigned int *)(HYADES_MMIO_BASE + HYADES_ACTION_CHECKSUM_HI_OFFSET))
#define HYADES_ACTION_TOKEN    ((volatile unsigned int *)(HYADES_MMIO_BASE + HYADES_ACTION_TOKEN_OFFSET))

/**
 * Read next command index from Hali. Value < 0 means exit.
 */
static inline int hyades_command_read(void) {
    return *HYADES_COMMAND;
}

/**
 * Signal completion of the given action index to Hali.
 */
static inline void hyades_status_write(int idx) {
    *HYADES_STATUS = idx;
}

/* Read the VLA agent's current sequence length (decoder-only helper; KV-cache write index). */
static inline int hyades_seq_len_read(void) {
    return *HYADES_SEQ_LEN;
}

/*
 * Register labeled region into regions[slot] on COMMIT; base is the touched VA.
 */
static inline void hyades_register_region(int slot,
                                          unsigned long base,
                                          unsigned long size) {
    *HYADES_REGION_BASE_LO = (unsigned int)(base & 0xFFFFFFFFul);
    *HYADES_REGION_BASE_HI = (unsigned int)((base >> 32) & 0xFFFFFFFFul);
    *HYADES_REGION_SIZE    = (unsigned int)size;
    *HYADES_REGION_COMMIT  = slot;
    /* Vanadis store buffer does not drain early-startup stores without a fence */
    __asm__ volatile ("fence iorw, iorw" ::: "memory");
}

/*
 * Publish ACTUATE action checksum; unpublished => synthetic cycle^seqLen hash.
 */
static inline void hyades_action_checksum_write(unsigned long long checksum) {
    /* Publish 64-bit as HI then LO (LO commits); fences order HI/LO/status.
     * Legacy 32-bit callers use HI = 0. */
    *HYADES_ACTION_CHECKSUM_HI = (unsigned int)((checksum >> 32) & 0xFFFFFFFFull);
    __asm__ volatile ("fence iorw, iorw" ::: "memory");
    *HYADES_ACTION_CHECKSUM    = (unsigned int)(checksum & 0xFFFFFFFFull);
    __asm__ volatile ("fence iorw, iorw" ::: "memory");
}

/*
 * Publish decoded-action token before checksum; 0 => scorer uses checksum.
 */
static inline void hyades_action_token_write(unsigned int token) {
    *HYADES_ACTION_TOKEN = token;
}

/**
 * Handler type: no args, no return.
 */
typedef void (*hyades_handler_t)(void);

/** Control loop: read cmd, run handlers[cmd], write status until cmd < 0. */
static inline void hyades_run(hyades_handler_t *handlers, int n_handlers) {
    for (;;) {
        int idx = hyades_command_read();
        if (idx < 0)
            break;
        if (idx >= 0 && idx < n_handlers && handlers[idx] != 0)
            handlers[idx]();
        hyades_status_write(idx);
    }
}

/** Index-passing hyades_run for single-dispatcher workloads. */
typedef void (*hyades_handler_idx_t)(int idx);

static inline void hyades_run_idx(hyades_handler_idx_t handler) {
    for (;;) {
        int idx = hyades_command_read();
        if (idx < 0)
            break;
        if (handler)
            handler(idx);
        hyades_status_write(idx);
    }
}

/**
 * Parse role from argv (e.g. argv[1] "0" or "1"). Returns 0 if missing/invalid.
 */
static inline int hyades_role_from_argv(int argc, char *argv[]) {
    return (argc > 1) ? atoi(argv[1]) : 0;
}

#endif /* HYADES_H */
