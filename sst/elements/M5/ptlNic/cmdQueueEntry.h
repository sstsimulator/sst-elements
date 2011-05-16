/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; -*-
 * vim:expandtab:shiftwidth=4:tabstop=4:
 */

#ifndef _cmdQueueEntry_h
#define _cmdQueueEntry_h

typedef enum {
    PtlNIInit = 1,   
    PtlNIFini,
    PtlPTAlloc,
    PtlPTFree
} cmd_t;

#define CMD_NAMES {\
    "",\
    "PtlNIInit",\
    "PtlNIFini",\
    "PtlPTAlloc",\
    "PtlPTFree" \
}

    #define NUM_ARGS 15
typedef struct {
    volatile unsigned int retval;
    volatile int cmd;
    unsigned long arg[ NUM_ARGS ];
} cmdQueueEntry_t;

#endif
