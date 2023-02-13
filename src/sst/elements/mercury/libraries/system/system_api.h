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

#pragma once

#include <sst/core/params.h>
#include <sst/core/eli/elementinfo.h>
#include <operating_system/libraries/api.h>

namespace SST {
namespace Hg {

class systemAPI : public SST::Hg::API {

public:

  SST_ELI_REGISTER_DERIVED(
    API,
    systemAPI,
    "hg",
    "system",
    SST_ELI_ELEMENT_VERSION(1,0,0),
    "provides the Hg system API")

systemAPI(SST::Params& params, App* app, SST::Component* comp);

virtual ~systemAPI() { }

/** Block and return the time when unblocked */
double ssthg_block();

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
