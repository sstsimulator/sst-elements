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

#include <stdio.h>
#include <time.h>

#define _GNU_SOURCE         /* See feature_test_macros(7) */
#include <unistd.h>
#include <sys/syscall.h>   /* For SYS_xxx definitions */
#include <inttypes.h>

int main( int argc, char* argv[] ) {
    clockid_t clk;
    struct timespec ts;

    printf("sizeof(tv_sec)=%zu, sizeof(tv_nsec)=%zu\n",sizeof(ts.tv_sec),sizeof(ts.tv_nsec));
    ts.tv_sec = 0;
    ts.tv_nsec = 0;

#ifdef SYS_clock_gettime64
    int r = syscall(SYS_clock_gettime64, clk, &ts);
    printf("tv_sec=%" PRIu64 ", tv_nsec=%" PRIu32 "\n",ts.tv_sec,ts.tv_nsec);
#else
    int r = syscall(SYS_clock_gettime, clk, &ts);
    printf("tv_sec=%" PRIu64 ", tv_nsec=%" PRIu64 "\n",ts.tv_sec,ts.tv_nsec);
#endif
}
