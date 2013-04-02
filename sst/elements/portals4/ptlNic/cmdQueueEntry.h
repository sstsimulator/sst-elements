/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; -*-
 * vim:expandtab:shiftwidth=4:tabstop=4:
 */

#ifndef _cmdQueueEntry_h
#define _cmdQueueEntry_h
#include "portals4.h"
#include "ptlEvent.h"

namespace SST {
namespace Portals4 {

typedef enum {
    PtlNIInitCmd = 1,   
    PtlNIFiniCmd,
    PtlPTAllocCmd,
    PtlPTFreeCmd,
    PtlMDBindCmd,
    PtlMDReleaseCmd,
    PtlMEAppendCmd,
    PtlMEUnlinkCmd,
    PtlGetIdCmd,
    PtlCTAllocCmd,
    PtlCTFreeCmd,
    PtlCTWaitCmd,
    PtlPutCmd,
    PtlGetCmd,
    PtlTrigGetCmd,
    PtlEQAllocCmd,
    PtlEQFreeCmd,
    ContextInitCmd,
    ContextFiniCmd
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
    "PtlTrigGet",\
    "PtlEQAlloc",\
    "PtlEQFree",\
    "ContextInit",\
    "ContextFini"\
}

typedef int cmdHandle_t;
typedef unsigned long cmdAddr_t;

typedef struct {
    int uid;
    cmdAddr_t nidPtr; 
    cmdAddr_t limitsPtr; 
    cmdAddr_t meUnlinkedPtr;
} cmdContextInit_t;

typedef struct {
} cmdContextFini_t;

typedef struct {
    ptl_pid_t pid; 
    unsigned int options;
} cmdPtlNIInit_t;

typedef struct {
} cmdPtlNIFini_t;

typedef struct {
    cmdHandle_t handle; 
    ptl_ct_event_t*   addr;
} cmdPtlCTAlloc_t;

typedef struct {
    cmdHandle_t handle; 
} cmdPtlCTFree_t;

typedef struct {
    cmdHandle_t handle; 
    PtlEventInternal*   addr;
    ptl_size_t          size;
} cmdPtlEQAlloc_t;

typedef struct {
    cmdHandle_t handle; 
} cmdPtlEQFree_t;

typedef struct {
    cmdHandle_t handle; 
    ptl_md_t md;
} cmdPtlMDBind_t;

typedef struct {
    cmdHandle_t handle; 
} cmdPtlMDRelease_t;

typedef struct {
    unsigned int options;
    cmdHandle_t eq_handle;
    ptl_pt_index_t pt_index;
} cmdPtlPTAlloc_t;

typedef struct {
    ptl_pt_index_t pt_index;
} cmdPtlPTFree_t;


typedef struct {
    cmdHandle_t md_handle;    
    ptl_size_t   local_offset;
    ptl_size_t   length;
    ptl_ack_req_t         ack_req;
    ptl_process_t    target_id;
    ptl_pt_index_t         pt_index;
    ptl_match_bits_t match_bits; 
    ptl_size_t       remote_offset;
    void*           user_ptr;
    ptl_hdr_data_t  hdr_data;
} cmdPtlPut_t;

typedef struct {
    cmdHandle_t md_handle;    
    ptl_size_t   local_offset;
    ptl_size_t   length;
    ptl_process_t    target_id;
    ptl_pt_index_t         pt_index;
    ptl_match_bits_t match_bits; 
    ptl_size_t       remote_offset;
    void*           user_ptr;
    ptl_hdr_data_t  hdr_data;
    ptl_handle_ct_t trig_ct_handle;
    ptl_size_t      threshold;
} cmdPtlGet_t;

typedef cmdPtlGet_t cmdPtlTrigGet_t;


typedef struct {
    cmdHandle_t handle;
    ptl_pt_index_t pt_index;
    ptl_me_t    me;
    ptl_list_t  list;
    void* user_ptr;
} cmdPtlMEAppend_t ;

typedef struct {
    cmdHandle_t handle;
} cmdPtlMEUnlink_t ;

typedef union {
    cmdContextInit_t    ctxInit;
    cmdContextFini_t    ctxFini;
    cmdPtlNIInit_t      niInit;
    cmdPtlNIFini_t      niFini;
    cmdPtlCTAlloc_t     ctAlloc;
    cmdPtlCTFree_t      ctFree;
    cmdPtlEQAlloc_t     eqAlloc;
    cmdPtlEQFree_t      eqFree;
    cmdPtlMDBind_t      mdBind;
    cmdPtlMDRelease_t   mdRelease;
    cmdPtlPut_t         ptlPut;
    cmdPtlGet_t         ptlGet;
    cmdPtlGet_t         ptlTrigGet;
    cmdPtlPTAlloc_t     ptAlloc;
    cmdPtlPTFree_t      ptFree;
    cmdPtlMEAppend_t    meAppend;
    cmdPtlMEUnlink_t    meUnlink;
} cmdUnion_t;

typedef struct {
    int type; 
    int ctx_id;
    cmdUnion_t u;
} cmdQueueEntry_t;

}
}

#endif
