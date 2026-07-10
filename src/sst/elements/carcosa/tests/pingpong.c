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

/* Ping-pong workload for Vanadis + Hali MMIO (via hyades.h). */
#include "hyades.h"
#include <unistd.h>

static void ping(void) { write(1, "PING\n", 5); }
static void pong(void) { write(1, "PONG\n", 5); }

int main(int argc, char *argv[]) {
    int role = hyades_role_from_argv(argc, argv);

    hyades_handler_t jump_table[2];
    if (role == 0) {
        jump_table[0] = ping;
        jump_table[1] = pong;
    } else {
        jump_table[0] = pong;
        jump_table[1] = ping;
    }

    hyades_run(jump_table, 2);
    return 0;
}
