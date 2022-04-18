// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

/* This file can be included by applications simulated by Ariel.
 * These functions will be replaced in the Pintool and provide a hook
 * for applications to interact with the simulator
 */

#ifndef _H_ARIEL_CLIENT_API
#define _H_ARIEL_CLIENT_API

#include <stdint.h>

#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif

/* Start simulating when this function is encountered
 * Must set ariel's 'arielmode' parameter to 0 or 2
 */
void ariel_enable();

/* Execute a fence */
void ariel_fence();

/* Return the number of simulated cycles since simulation begin */
uint64_t ariel_cycles();

/* Trigger the simulation to output statistics */
void ariel_output_stats();

/* Control which memory pool (level) the next 'count' allocations encountered should map to
 *
 */
void ariel_malloc_flag(int64_t id, int count, int level);

#if defined(c_plusplus) || defined(__cplusplus)
}
#endif

#endif
