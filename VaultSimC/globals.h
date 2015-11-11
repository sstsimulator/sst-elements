// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef GLOBALS_H
#define GLOBALS_H

//Debug Levels (output class)
enum {ERROR, WARNING, INFO, L3, L4, L5, L6, L7, L8, L9, L10};
#define _ERROR_ CALL_INFO,ERROR,0           //
#define _WARNING_ CALL_INFO,WARNING,0       //
#define _INFO_ CALL_INFO,INFO,0             // Init, Finish, Parameters
#define _L3_ CALL_INFO,L3,0                 //
#define _L4_ CALL_INFO,L4,0                 // Logic Layer Requests
#define _L5_ CALL_INFO,L5,0                 // VautlSimC answers
#define _L6_ CALL_INFO,L6,0                 // VaultSimC internals
#define _L7_ CALL_INFO,L7,0                 // Vault Answers
#define _L8_ CALL_INFO,L8,0                 // Vault Internal
#define _L9_ CALL_INFO,L9,0                 //
#define _L10_ CALL_INFO,L10,0               //


enum AddressMappingScheme {
    CB_CH_RW_RK_VL_BK_CL_BY,    // Cube - Column High - Row - Rank - Vault - Bank - Column Low - Byte
    CB_CH_RW_VL_BK_RK_CL_BY,    // Cube - Column High - Row - Vault - Bank - Rank - Vault - Column Low - Byte
    CB_CH_RW_BK_VL_RK_CL_BY,    // Cube - Column High - Row - Bank - Vault - Rank - Column Low - Byte
    CB_CH_RW_BK_RK_VL_CL_BY,    // Cube - Column High - Row - Bank - Rank - Vault - Column Low - Byte
    CB_CH_RW_RK_BK_VL_CL_BY,    // Cube - Column High - Row - Rank - Bank - Vault - Column Low - Byte
};


static const unsigned int LL_SHIFT = 12; // min hash between cubes in 1024
static const unsigned int VAULT_SHIFT = 7; // min hash for vaults in 16


unsigned inline log2(unsigned value) {
    unsigned logbase2 = 0;
    unsigned orig = value;
    value >>= 1;
    while (value > 0) {
        value >>= 1;
        logbase2++;
    }
    if (1u << logbase2 < orig) logbase2++;
    return logbase2;
} 

#endif
