// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
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

namespace SST {
namespace Hg {

class SystemAPI {

public:

/** Block and return the time when unblocked */
virtual double ssthg_block() = 0;

/**
 * @brief sleep SST virtual equivalent of Linux sleep
 * @param secs
 * @return Always zero, successful return code for Linux
 */
static unsigned int ssthg_sleep(unsigned int secs);

///**
// * @brief ssthg_usleep SST virtual equivalent of Linux usleep
// * @param usecs
// * @return Always zero, successful return code for Linux
// */
//int ssthg_usleep(unsigned int usecs);

//int ssthg_nanosleep(unsigned int nsecs);

//int ssthg_msleep(unsigned int msecs);

//int ssthg_fsleep(double secs);

//void ssthg_memread(uint64_t bytes);

//void ssthg_memwrite(uint64_t bytes);

//void ssthg_memcopy(uint64_t bytes);

//void* ssthg_alloc_stack(int sz, int md_sz);

//void ssthg_free_stack(void* ptr);

};

} // end namepace Hg
} // end namespace SST
