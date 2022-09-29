/**
Copyright 2009-2021 National Technology and Engineering Solutions of Sandia, 
LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S.  Government 
retains certain rights in this software.

Sandia National Laboratories is a multimission laboratory managed and operated
by National Technology and Engineering Solutions of Sandia, LLC., a wholly 
owned subsidiary of Honeywell International, Inc., for the U.S. Department of 
Energy's National Nuclear Security Administration under contract DE-NA0003525.

Copyright (c) 2009-2021, NTESS

All rights reserved.

Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.

    * Neither the name of the copyright holder nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Questions? Contact sst-macro-help@sandia.gov
*/

#ifndef sstmac_software_libraries_compute_compute_api_h
#define sstmac_software_libraries_compute_compute_api_h


#ifdef __cplusplus
#include <cstdint>
extern "C" {
#else
#include <stdint.h>
#endif

/** Block and return the time when unblocked */
double sstmac_block();

/**
 * @brief sstmac_sleep SST virtual equivalent of Linux sleep
 * @param secs
 * @return Always zero, successful return code for Linux
 */
unsigned int sstmac_sleep(unsigned int secs);

/**
 * @brief sstmac_usleep SST virtual equivalent of Linux usleep
 * @param usecs
 * @return Always zero, successful return code for Linux
 */
int sstmac_usleep(unsigned int usecs);

int sstmac_nanosleep(unsigned int nsecs);

int sstmac_msleep(unsigned int msecs);

int sstmac_fsleep(double secs);

/**
 * @brief sstmac_compute Compute for a specified number of seconds
 * @param secs
 */
void sstmac_compute(double secs);

/**
 * @brief sstmac_compute_detailed Model a specific compute block
 * @param nflops  The number of flops executed in the compute block
 * @param nintops The number of int ops executed in the compute block
 * @param bytes
 */
void sstmac_compute_detailed(uint64_t nflops, uint64_t nintops, uint64_t bytes);

void sstmac_compute_detailed_nthr(uint64_t nflops, uint64_t nintops, uint64_t bytes,
                                  int nthread);

/**
 * @brief sstmac_compute_loop
 * @param num_loops        The number of loops to execute
 * @param nflops_per_loop  The number of flops per loop in the inner loop
 * @param nintops_per_loop The number of integer ops in the inner loop (not including loop predicates like i < N)
 * @param bytes_per_loop   The average number of unique bytes read + written per loop
 */
void sstmac_computeLoop(uint64_t num_loops,
                    uint32_t nflops_per_loop,
                    uint32_t nintops_per_loop,
                    uint32_t bytes_per_loop);

/**
 * @brief sstmac_compute_loop2
 * @param isize           The number of indices in the outer loop (imax - imin)
 * @param jsize           The number of indices in the inner loop (jmax - jmin)
 * @param nflops_per_loop  The number of flops per loop in the inner loop
 * @param nintops_per_loop The number of integer ops in the inner loop (not including loop predicates like i < N)
 * @param bytes_per_loop   The average number of unique bytes read + written per loop
 */
void sstmac_compute_loop2(uint64_t isize, uint64_t jsize,
                    uint32_t nflops_per_loop,
                    uint32_t nintops_per_loop,
                    uint32_t bytes_per_loop);

/**
 * @brief sstmac_compute_loop3
 * @param isize           The number of indices in the outer loop (imax - imin)
 * @param jsize           The number of indices in the inner loop (jmax - jmin)
 * @param ksize           The number of indices in the inner loop (kmax - kmin)
 * @param nflops_per_loop  The number of flops per loop in the inner loop
 * @param nintops_per_loop The number of integer ops in the inner loop (not including loop predicates like i < N)
 * @param bytes_per_loop   The average number of unique bytes read + written per loop
 */
void sstmac_compute_loop3(uint64_t isize, uint64_t jsize,
                    uint64_t ksize,
                    uint32_t nflops_per_loop,
                    uint32_t nintops_per_loop,
                    uint32_t bytes_per_loop);

/**
 * @brief sstmac_compute_loop4
 * @param isize           The number of indices in the outer loop (imax - imin)
 * @param jsize           The number of indices in the inner loop (jmax - jmin)
 * @param ksize           The number of indices in the inner loop (kmax - kmin)
 * @param lsize           The number of indices in the inner loop (lmax - lmin)
 * @param nflops_per_loop  The number of flops per loop in the inner loop
 * @param nintops_per_loop The number of integer ops in the inner loop (not including loop predicates like i < N)
 * @param bytes_per_loop   The average number of unique bytes read + written per loop
 */
void sstmac_compute_loop4(uint64_t isize, uint64_t jsize,
                    uint64_t ksize, uint64_t lsize,
                    uint32_t nflops_per_loop,
                    uint32_t nintops_per_loop,
                    uint32_t bytes_per_loop);

void sstmac_memread(uint64_t bytes);

void sstmac_memwrite(uint64_t bytes);

void sstmac_memcopy(uint64_t bytes);

/**
This has to work from C, so we must regrettably use VA_ARGS
*/
/**
 * @brief sstmac_start_memoize
 * @param token
 * @param model
 * @return A thread tag to identify thread-local storage later
 */
int sstmac_start_memoize(const char* token, const char* model);

void sstmac_finish_memoize0(int thr_tag, const char* token);
void sstmac_finish_memoize1(int thr_tag, const char* token, double p1);
void sstmac_finish_memoize2(int thr_tag, const char* token, double p1, double p2);
void sstmac_finish_memoize3(int thr_tag, const char* token, double p1, double p2,
                            double p3);
void sstmac_finish_memoize4(int thr_tag, const char* token, double p1, double p2,
                            double p3, double p4);
void sstmac_finish_memoize5(int thr_tag, const char* token, double p1, double p2,
                            double p3, double p4, double p5);

void sstmac_compute_memoize0(const char* token);
void sstmac_compute_memoize1(const char* token, double p1);
void sstmac_compute_memoize2(const char* token, double p1, double p2);
void sstmac_compute_memoize3(const char* token, double p1, double p2,
                             double p3);
void sstmac_compute_memoize4(const char* token, double p1, double p2,
                             double p3, double p4);
void sstmac_compute_memoize5(const char* token, double p1, double p2,
                             double p3, double p4, double p5);


void sstmac_set_implicit_memoize_state1(int type0, int state0);
void sstmac_set_implicit_memoize_state2(int type0, int state0, int type1, int state1);
void sstmac_set_implicit_memoize_state3(int type0, int state0, int type1, int state1,
                                        int type2, int state2);
void sstmac_unset_implicit_memoize_state1(int type0);
void sstmac_unset_implicit_memoize_state2(int type0, int type1);
void sstmac_unset_implicit_memoize_state3(int type0, int type1, int type2);

void sstmac_set_implicit_compute_state1(int type0, int state0);
void sstmac_set_implicit_compute_state2(int type0, int state0, int type1, int state1);
void sstmac_set_implicit_compute_state3(int type0, int state0, int type1, int state1,
                                        int type2, int state2);
void sstmac_unset_implicit_compute_state1(int type0);
void sstmac_unset_implicit_compute_state2(int type0, int type1);
void sstmac_unset_implicit_compute_state3(int type0, int type1, int type2);

void* sstmac_alloc_stack(int sz, int md_sz);
void sstmac_free_stack(void* ptr);

#define SSTMAC_sleep(...) sstmac_sleep(__VA_ARGS__)
#define SSTMAC_usleep(...) sstmac_usleep(__VA_ARGS__)
#define SSTMAC_compute(...) sstmac_compute(__VA_ARGS__)
#define SSTMAC_memread(...) sstmac_memread(__VA_ARGS__)
#define SSTMAC_memwrite(...) sstmac_memwrite(__VA_ARGS__)
#define SSTMAC_memcopy(...) sstmac_memcopy(__VA_ARGS__)
#define SSTMAC_computeDetailed(...) sstmac_compute_detailed(__VA_ARGS__)
#define SSTMAC_computeLoop(...) sstmac_computeLoop(__VA_ARGS__)
#define SSTMAC_compute_loop2(...) sstmac_compute_loop2(__VA_ARGS__)
#define SSTMAC_compute_loop3(...) sstmac_compute_loop3(__VA_ARGS__)
#define SSTMAC_compute_loop4(...) sstmac_compute_loop4(__VA_ARGS__)

#ifdef __cplusplus
} //end extern c
#endif

#endif
