/*
 * Hyades - Vanadis control abstraction
 *
 * Single header that encapsulates the MMIO coordination protocol used when
 * running under SST/Vanadis with a Hali in the data path. Hali intercepts
 * accesses to a small MMIO region and coordinates multiple cores (e.g. ping-pong).
 *
 * -----------------------------------------------------------------------------
 * BUILD-TIME PARAMETERS (define before #include "hyades.h" if not using default)
 * -----------------------------------------------------------------------------
 *
 *   HYADES_MMIO_BASE
 *     Optional. Base virtual address of the MMIO control region. Must match
 *     the Hali parameter "control_addr_base" in the SST config.
 *     Default: 0xBEEF0000
 *     Example: #define HYADES_MMIO_BASE 0xBEEF0000
 *
 * -----------------------------------------------------------------------------
 * PROTOCOL (must match Hali MMIO handling)
 * -----------------------------------------------------------------------------
 *
 *   - Base + 0x0 (command register, read):
 *       The CPU reads the next action index. Value < 0 means "exit" (end the
 *       run loop). The read may block until Hali has a command ready.
 *
 *   - Base + 0x4 (status register, write):
 *       The CPU writes the completed action index to signal done. Hali
 *       coordinates with the partner core, then advances and may unblock
 *       the next command read.
 *
 * -----------------------------------------------------------------------------
 * RUNTIME API
 * -----------------------------------------------------------------------------
 *
 *   hyades_command_read(void)
 *     Returns the next command index (>= 0) or exit sentinel (< 0).
 *     Parameters: none.
 *
 *   hyades_status_write(int idx)
 *     Signals completion of the action for index idx.
 *     Parameters: idx - the command index that was just executed.
 *
 *   hyades_run(hyades_handler_t *handlers, int n_handlers)
 *     Runs the control loop: read command -> call handler -> write status,
 *     until the command is exit. Required for the standard use pattern.
 *     Parameters:
 *       handlers   - array of function pointers (jump table); may be NULL for
 *                    unused indices.
 *       n_handlers - number of valid entries in handlers (e.g. 2 for ping/pong).
 *
 *   hyades_role_from_argv(int argc, char *argv[])
 *     Optional helper. Parses role from argv[1] (e.g. "0" or "1"). Used to
 *     select which action map this process uses (e.g. role 0: [ping, pong],
 *     role 1: [pong, ping]).
 *     Parameters: argc, argv - standard main() arguments.
 *     Returns: role as int, or 0 if argv[1] is missing or invalid.
 *
 *   hyades_handler_t
 *     Type: void (*)(void). Each entry in the jump table must have this type.
 *
 * -----------------------------------------------------------------------------
 * EXAMPLE: pingpong
 * -----------------------------------------------------------------------------
 *
 *   Two processes run the same binary; each gets a role via argv[1]. Role
 *   selects the order of actions (ping then pong, or pong then ping). The
 *   run loop is entirely inside hyades_run().
 *
 *   Build (from the tests/ directory, so hyades.h is found):
 *     riscv64-unknown-linux-gnu-gcc -static -I.. -o pingpong pingpong.c
 *
 *   Example code:
 *
 *     #include "hyades.h"
 *     #include <unistd.h>
 *
 *     static void ping(void) { write(1, "PING\n", 5); }
 *     static void pong(void) { write(1, "PONG\n", 5); }
 *
 *     int main(int argc, char *argv[]) {
 *         int role = hyades_role_from_argv(argc, argv);   // optional helper
 *
 *         hyades_handler_t jump_table[2];
 *         if (role == 0) {
 *             jump_table[0] = ping;
 *             jump_table[1] = pong;
 *         } else {
 *             jump_table[0] = pong;
 *             jump_table[1] = ping;
 *         }
 *
 *         hyades_run(jump_table, 2);   // required: run until Hali sends exit
 *         return 0;
 *     }
 *
 *   Required usage: same MMIO base as Hali (default 0xBEEF0000), jump table
 *   populated for indices 0..n_handlers-1, and exactly one call to
 *   hyades_run(handlers, n_handlers) so the process participates in the
 *   coordinated loop until exit.
 */
#ifndef HYADES_H
#define HYADES_H

#include <stdlib.h>

#ifndef HYADES_MMIO_BASE
#define HYADES_MMIO_BASE  0xBEEF0000
#endif

#define HYADES_COMMAND_OFFSET  0
#define HYADES_STATUS_OFFSET   4

#define HYADES_COMMAND  ((volatile int *)(HYADES_MMIO_BASE + HYADES_COMMAND_OFFSET))
#define HYADES_STATUS   ((volatile int *)(HYADES_MMIO_BASE + HYADES_STATUS_OFFSET))

/**
 * Read next command index from Hali. Value < 0 means exit.
 */
static inline int hyades_command_read(void) {
    return *HYADES_COMMAND;
}

/**
 * Signal completion of the given action index to Hali.
 */
static inline void hyades_status_write(int idx) {
    *HYADES_STATUS = idx;
}

/**
 * Handler type: no args, no return.
 */
typedef void (*hyades_handler_t)(void);

/**
 * Run the Hyades control loop: repeatedly read command, run handlers[cmd], write status.
 * Exits when command is < 0. n_handlers is the number of valid entries in handlers[].
 * If cmd is in [0, n_handlers), handlers[cmd] is called; otherwise only status is written.
 */
static inline void hyades_run(hyades_handler_t *handlers, int n_handlers) {
    for (;;) {
        int idx = hyades_command_read();
        if (idx < 0)
            break;
        if (idx >= 0 && idx < n_handlers && handlers[idx] != 0)
            handlers[idx]();
        hyades_status_write(idx);
    }
}

/**
 * Parse role from argv (e.g. argv[1] "0" or "1"). Returns 0 if missing/invalid.
 */
static inline int hyades_role_from_argv(int argc, char *argv[]) {
    return (argc > 1) ? atoi(argv[1]) : 0;
}

#endif /* HYADES_H */
