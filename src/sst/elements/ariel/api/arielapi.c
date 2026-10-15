// Copyright 2009-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2026, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "arielapi.h"
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

/* These definitions are replaced during simulation */

void ariel_enable() {
    printf("ARIEL: ENABLE called in Ariel API.\n");
}

void ariel_enable_() { ariel_enable(); }

void ariel_disable() {
    printf("ARIEL: DISABLE called in Ariel API.\n");
}

void ariel_disable_() { ariel_disable(); }

void ariel_fence() {
    printf("ARIEL: FENCE called in Ariel API.\n");
}

uint64_t ariel_cycles() {
    printf("ARIEL: ariel_cycles called in Ariel API.\n");
    return 0;
}

void ariel_output_stats() {
    printf("ARIEL: Request to print statistics.\n");
}

void ariel_malloc_flag(int64_t id, int count, int level) {
    printf("ARIEL: flagging next %d mallocs at id %" PRId64 "\n", count, id);
}

void ariel_output_stats_begin_region(const char *name) {
    printf("ARIEL: Request to print statistics and begin region:%s\n", name);
}

void ariel_output_stats_end_region(const char *name) {
    printf("ARIEL: Request to print statistics and end region:%s\n", name);
}
