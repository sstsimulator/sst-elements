// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

enum GpuApi_t {
    GPU_REG_FAT_BINARY = 1,
    GPU_REG_FAT_BINARY_RET = 2,
    GPU_REG_FUNCTION = 3,
    GPU_REG_FUNCTION_RET = 4,
    GPU_MEMCPY = 5,
    GPU_MEMCPY_RET = 6,
    GPU_CONFIG_CALL = 7,
    GPU_CONFIG_CALL_RET = 8,
    GPU_SET_ARG = 9,
    GPU_SET_ARG_RET = 10,
    GPU_LAUNCH = 11,
    GPU_LAUNCH_RET = 12,
    GPU_FREE = 13,
    GPU_FREE_RET = 14,
    GPU_GET_LAST_ERROR = 15,
    GPU_GET_LAST_ERROR_RET = 16,
    GPU_MALLOC = 17,
    GPU_MALLOC_RET = 18,
    GPU_REG_VAR = 19,
    GPU_REG_VAR_RET = 20,
    GPU_MAX_BLOCK = 21,
    GPU_MAX_BLOCK_RET = 22,
};
