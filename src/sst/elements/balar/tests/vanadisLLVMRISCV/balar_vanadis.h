// Copyright 2009-2023 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2023, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef __BALAR_VANADIS_H__
#define __BALAR_VANADIS_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#define LOG_LEVEL_INFO 20
#define LOG_LEVEL_DEBUG 15
#define LOG_LEVEL_WARNING 10
#define LOG_LEVEL_ERROR 0

// Depends on arch, mips use uint32_t, riscv get uint64_t
typedef uint64_t Addr_t;

// Global mmio gpu address
static Addr_t* g_balarBaseAddr = (Addr_t*) -1;
static uint8_t g_scratch_mem[1024];
static int32_t g_debug_level = LOG_LEVEL_INFO;

#endif
