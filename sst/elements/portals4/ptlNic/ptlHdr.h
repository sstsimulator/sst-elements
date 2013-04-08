// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2013, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _ptlHdr_t
#define _ptlHdr_t

#include "portals4.h"

namespace SST {
namespace Portals4 {

union CtrlFlit {
    struct {
        unsigned nid : 32; 
        unsigned head : 1;  
        unsigned tail : 1;  
    } s;
    unsigned long all;
};

typedef enum {
    Put, Get, Atomic, FetchAtomic, Swap, Ack, Ack2, Reply  
} op_t;

struct PtlHdr {
    ptl_match_bits_t    match_bits;
    ptl_hdr_data_t      hdr_data;
    ptl_size_t          length;
    ptl_size_t          offset;
    uint16_t            dest_pid;
    uint16_t            src_pid;
    uint16_t            uid;
    uint8_t             key;
    uint8_t             ni       : 2;
    ptl_ack_req_t       ack_req  : 2;
    op_t                op       : 5;
    ptl_pt_index_t      pt_index : 6;
};

}
}

#endif
