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

#ifndef PTL_INTERNAL_HANDLES_H
#define PTL_INTERNAL_HANDLES_H

#define HANDLE_SELECTOR_BITS 3
#define HANDLE_IFACE_BITS    3 
#define HANDLE_NI_BITS       2
#define HANDLE_CODE_BITS     24

typedef union {
    uint32_t i;
    struct {
#ifdef BITFIELD_ORDER_FORWARD
        unsigned int  selector : HANDLE_SELECTOR_BITS;
        unsigned char iface    : HANDLE_IFACE_BITS;
        unsigned char ni       : HANDLE_NI_BITS;
        unsigned int  code     : HANDLE_CODE_BITS;
#else
        unsigned int  code     : HANDLE_CODE_BITS;
        unsigned char ni       : HANDLE_NI_BITS;
        unsigned char iface    : HANDLE_IFACE_BITS;
        unsigned int  selector : HANDLE_SELECTOR_BITS;
#endif
    } s;
    ptl_handle_any_t a;
} ptl_internal_handle_converter_t;


#include <stdio.h>
static inline void print_handle (ptl_handle_any_t h )
{
    ptl_internal_handle_converter_t tmp = {h};
    printf("handle sel=%d iface=%d ni=%d code=%d\n",tmp.s.selector,
        tmp.s.iface, tmp.s.ni, tmp.s.code );
}

#define HANDLE_NI_CODE 0
#define HANDLE_EQ_CODE 1
#define HANDLE_CT_CODE 2
#define HANDLE_MD_CODE 3
#define HANDLE_LE_CODE 4
#define HANDLE_ME_CODE 5

#endif /* ifndef PTL_INTERNAL_HANDLES_H */
/* vim:set expandtab: */
