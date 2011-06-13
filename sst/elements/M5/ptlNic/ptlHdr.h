#ifndef _ptlHdr_t
#define _ptlHdr_t

#include "portals4_types.h"

union CtrlFlit {
    struct {
        unsigned nid : 32; 
        unsigned head : 1;  
        unsigned tail : 1;  
    } s;
    unsigned long all;
};

struct PtlHdr {
    ptl_size_t          length;
    ptl_ack_req_t       ack_req;
    ptl_pt_index_t      pt_index;
    ptl_match_bits_t    match_bits;
    ptl_size_t          offset;
    ptl_hdr_data_t      hdr_data;
    ptl_pid_t           dest_pid;
    ptl_pid_t           src_pid;
};
#endif
