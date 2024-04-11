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

#pragma once

#ifdef __cplusplus
#include <cstdint>
extern "C" {
#else
#include <stdint.h>
#endif

/** Block and return the time when unblocked */
double sst_hg_block();

/**
 * @brief sst_hg_sleep SST virtual equivalent of Linux sleep
 * @param secs
 * @return Always zero, successful return code for Linux
 */
unsigned int sst_hg_sleep(unsigned int secs);

/**
 * @brief sst_hg_usleep SST virtual equivalent of Linux usleep
 * @param usecs
 * @return Always zero, successful return code for Linux
 */
int sst_hg_usleep(unsigned int usecs);

int sst_hg_nanosleep(unsigned int nsecs);

int sst_hg_msleep(unsigned int msecs);

int sst_hg_fsleep(double secs);

/**
 * @brief sst_hg_compute Compute for a specified number of seconds
 * @param secs
 */
void sst_hg_compute(double secs);

/**
 * @brief sst_hg_compute_detailed Model a specific compute block
 * @param nflops  The number of flops executed in the compute block
 * @param nintops The number of int ops executed in the compute block
 * @param bytes
 */
void sst_hg_compute_detailed(uint64_t nflops, uint64_t nintops, uint64_t bytes);

void sst_hg_compute_detailed_nthr(uint64_t nflops, uint64_t nintops, uint64_t bytes,
                                  int nthread);

/**
 * @brief sst_hg_compute_loop
 * @param num_loops        The number of loops to execute
 * @param nflops_per_loop  The number of flops per loop in the inner loop
 * @param nintops_per_loop The number of integer ops in the inner loop (not including loop predicates like i < N)
 * @param bytes_per_loop   The average number of unique bytes read + written per loop
 */
void sst_hg_computeLoop(uint64_t num_loops,
                    uint32_t nflops_per_loop,
                    uint32_t nintops_per_loop,
                    uint32_t bytes_per_loop);

/**
 * @brief sst_hg_compute_loop2
 * @param isize           The number of indices in the outer loop (imax - imin)
 * @param jsize           The number of indices in the inner loop (jmax - jmin)
 * @param nflops_per_loop  The number of flops per loop in the inner loop
 * @param nintops_per_loop The number of integer ops in the inner loop (not including loop predicates like i < N)
 * @param bytes_per_loop   The average number of unique bytes read + written per loop
 */
void sst_hg_compute_loop2(uint64_t isize, uint64_t jsize,
                    uint32_t nflops_per_loop,
                    uint32_t nintops_per_loop,
                    uint32_t bytes_per_loop);

/**
 * @brief sst_hg_compute_loop3
 * @param isize           The number of indices in the outer loop (imax - imin)
 * @param jsize           The number of indices in the inner loop (jmax - jmin)
 * @param ksize           The number of indices in the inner loop (kmax - kmin)
 * @param nflops_per_loop  The number of flops per loop in the inner loop
 * @param nintops_per_loop The number of integer ops in the inner loop (not including loop predicates like i < N)
 * @param bytes_per_loop   The average number of unique bytes read + written per loop
 */
void sst_hg_compute_loop3(uint64_t isize, uint64_t jsize,
                    uint64_t ksize,
                    uint32_t nflops_per_loop,
                    uint32_t nintops_per_loop,
                    uint32_t bytes_per_loop);

/**
 * @brief sst_hg_compute_loop4
 * @param isize           The number of indices in the outer loop (imax - imin)
 * @param jsize           The number of indices in the inner loop (jmax - jmin)
 * @param ksize           The number of indices in the inner loop (kmax - kmin)
 * @param lsize           The number of indices in the inner loop (lmax - lmin)
 * @param nflops_per_loop  The number of flops per loop in the inner loop
 * @param nintops_per_loop The number of integer ops in the inner loop (not including loop predicates like i < N)
 * @param bytes_per_loop   The average number of unique bytes read + written per loop
 */
void sst_hg_compute_loop4(uint64_t isize, uint64_t jsize,
                    uint64_t ksize, uint64_t lsize,
                    uint32_t nflops_per_loop,
                    uint32_t nintops_per_loop,
                    uint32_t bytes_per_loop);

void sst_hg_memread(uint64_t bytes);

void sst_hg_memwrite(uint64_t bytes);

void sst_hg_memcopy(uint64_t bytes);

/**
This has to work from C, so we must regrettably use VA_ARGS
*/
/**
 * @brief sst_hg_start_memoize
 * @param token
 * @param model
 * @return A thread tag to identify thread-local storage later
 */
int sst_hg_start_memoize(const char* token, const char* model);

void sst_hg_finish_memoize0(int thr_tag, const char* token);
void sst_hg_finish_memoize1(int thr_tag, const char* token, double p1);
void sst_hg_finish_memoize2(int thr_tag, const char* token, double p1, double p2);
void sst_hg_finish_memoize3(int thr_tag, const char* token, double p1, double p2,
                            double p3);
void sst_hg_finish_memoize4(int thr_tag, const char* token, double p1, double p2,
                            double p3, double p4);
void sst_hg_finish_memoize5(int thr_tag, const char* token, double p1, double p2,
                            double p3, double p4, double p5);

void sst_hg_compute_memoize0(const char* token);
void sst_hg_compute_memoize1(const char* token, double p1);
void sst_hg_compute_memoize2(const char* token, double p1, double p2);
void sst_hg_compute_memoize3(const char* token, double p1, double p2,
                             double p3);
void sst_hg_compute_memoize4(const char* token, double p1, double p2,
                             double p3, double p4);
void sst_hg_compute_memoize5(const char* token, double p1, double p2,
                             double p3, double p4, double p5);


void sst_hg_set_implicit_memoize_state1(int type0, int state0);
void sst_hg_set_implicit_memoize_state2(int type0, int state0, int type1, int state1);
void sst_hg_set_implicit_memoize_state3(int type0, int state0, int type1, int state1,
                                        int type2, int state2);
void sst_hg_unset_implicit_memoize_state1(int type0);
void sst_hg_unset_implicit_memoize_state2(int type0, int type1);
void sst_hg_unset_implicit_memoize_state3(int type0, int type1, int type2);

void sst_hg_set_implicit_compute_state1(int type0, int state0);
void sst_hg_set_implicit_compute_state2(int type0, int state0, int type1, int state1);
void sst_hg_set_implicit_compute_state3(int type0, int state0, int type1, int state1,
                                        int type2, int state2);
void sst_hg_unset_implicit_compute_state1(int type0);
void sst_hg_unset_implicit_compute_state2(int type0, int type1);
void sst_hg_unset_implicit_compute_state3(int type0, int type1, int type2);

void* sst_hg_alloc_stack(int sz, int md_sz);
void sst_hg_free_stack(void* ptr);

#define SST_HG_sleep(...) sst_hg_sleep(__VA_ARGS__)
#define SST_HG_usleep(...) sst_hg_usleep(__VA_ARGS__)
#define SST_HG_compute(...) sst_hg_compute(__VA_ARGS__)
#define SST_HG_memread(...) sst_hg_memread(__VA_ARGS__)
#define SST_HG_memwrite(...) sst_hg_memwrite(__VA_ARGS__)
#define SST_HG_memcopy(...) sst_hg_memcopy(__VA_ARGS__)
#define SST_HG_computeDetailed(...) sst_hg_compute_detailed(__VA_ARGS__)
#define SST_HG_computeLoop(...) sst_hg_computeLoop(__VA_ARGS__)
#define SST_HG_compute_loop2(...) sst_hg_compute_loop2(__VA_ARGS__)
#define SST_HG_compute_loop3(...) sst_hg_compute_loop3(__VA_ARGS__)
#define SST_HG_compute_loop4(...) sst_hg_compute_loop4(__VA_ARGS__)

#ifdef __cplusplus
} //end extern c
#endif
