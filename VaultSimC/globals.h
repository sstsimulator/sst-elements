#ifndef GLOBALS_H
#define GLOBALS_H

enum AddressMappingScheme {
    CB_CH_RW_RK_VL_BK_CL_BY,    // Cube - Column High - Row - Rank - Vault - Bank - Column Low - Byte
    CB_CH_RW_VL_BK_RK_CL_BY,    // Cube - Column High - Row - Vault - Bank - Rank - Vault - Column Low - Byte
    CB_CH_RW_BK_VL_RK_CL_BY,    // Cube - Column High - Row - Bank - Vault - Rank - Column Low - Byte
    CB_CH_RW_BK_RK_VL_CL_BY,    // Cube - Column High - Row - Bank - Rank - Vault - Column Low - Byte
    CB_CH_RW_RK_BK_VL_CL_BY,    // Cube - Column High - Row - Rank - Bank - Vault - Column Low - Byte
};

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
