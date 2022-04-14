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

#include "arielapi.h"
#include <stdio.h>
#include <inttypes.h>

/* These definitions are replaced during simulation */

void ariel_enable() {
    printf("ARIEL: ENABLE called in Ariel API.\n");
}

void ariel_fence() {
    printf("ARIEL: FENCE called in Ariel API.\n");
}

uint64_t ariel_cycles() {
    return 0;
}

void ariel_output_stats() {
    printf("ARIEL: Request to print statistics.\n");
}

void ariel_malloc_flag(int64_t id, int count, int level) {
    printf("ARIEL: flagging next %d mallocs at id %" PRId64 "\n", count, id);
}
