/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; -*-
 * vim:expandtab:shiftwidth=4:tabstop=4:
 */

#ifndef _cmdQueueEntry_h
#define _cmdQueueEntry_h

typedef enum {
    PtlNIInit = 1,   
    PtlNIFini,
    PtlPTAlloc,
    PtlPTFree,
    PtlMDBind,
    PtlMDRelease,
    PtlMEAppend,
    PtlMEUnlink,
    PtlGetId,
    PtlCTAlloc,
    PtlCTFree,
    PtlCTWait,
    PtlPut,
    PtlGet,
    PtlEQAlloc,
    PtlEQFree,
    ContextInit,
    ContextFini,
} cmd_t;

#define CMD_NAMES {\
    "",\
    "PtlNIInit",\
    "PtlNIFini",\
    "PtlPTAlloc",\
    "PtlPTFree",\
    "PtlMDBind",\
    "PtlMDRelease",\
    "PtlMEAppend",\
    "PtlMEUnlink",\
    "PtlGetId",\
    "PtlCTAlloc",\
    "PtlCTFree",\
    "PtlCTWait",\
    "PtlPut",\
    "PtlGet",\
    "PtlEQAlloc",\
    "PtlEQFree",\
    "ContextInit",\
    "ContextFini"\
}

typedef struct {
    #define NUM_ARGS 15
    volatile unsigned int retval;
    volatile int cmd;
    int context;
    unsigned long args[ NUM_ARGS ];
} cmdQueueEntry_t;

#endif
