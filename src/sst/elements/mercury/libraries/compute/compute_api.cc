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

#include <components/operating_system.h>
#include <operating_system/process/thread.h>
#include <operating_system/process/app.h>
#include <common/timestamp.h>
#include <libraries/compute/compute_api.h>
#include <common/thread_safe_new.h>
#include <sstmac/skeleton.h>


using SST::Hg::TimeDelta;
using SST::Hg::Timestamp;
using os = SST::Hg::OperatingSystem;

extern "C" double sstmac_block()
{
  os::currentOs()->block();
  return os::currentOs()->now().sec();
}

extern "C" unsigned int sstmac_sleep(unsigned int secs){
  os::currentOs()->sleep(TimeDelta(secs, TimeDelta::one_second));
  return 0;
}

extern "C" unsigned sstmac_sleepUntil(double t){
  os::currentOs()->sleepUntil(Timestamp(t));
  return 0;
}

extern "C" int sstmac_usleep(unsigned int usecs){
  os::currentOs()->sleep(TimeDelta(usecs, TimeDelta::one_microsecond));
  return 0;
}

extern "C" int sstmac_nanosleep(unsigned int nanosecs){
  os::currentOs()->sleep(TimeDelta(nanosecs, TimeDelta::one_nanosecond));
  return 0;
}

extern "C" int sstmac_msleep(unsigned int msecs){
  os::currentOs()->sleep(TimeDelta(msecs, TimeDelta::one_millisecond));
  return 0;
}

extern "C" int sstmac_fsleep(double secs){
  sstmac::sw::OperatingSystem::currentThread()->parentApp()->sleep(sstmac::TimeDelta(secs));
  return 0;
}

extern "C" void sstmac_compute(double secs){
  sstmac::sw::OperatingSystem::currentOs()->compute(TimeDelta(secs));
}

extern "C" void sstmac_memread(uint64_t bytes){
  sstmac::sw::OperatingSystem::currentThread()->parentApp()
    ->computeBlockRead(bytes);
}

extern "C" void sstmac_memwrite(uint64_t bytes){
  sstmac::sw::OperatingSystem::currentThread()->parentApp()
    ->computeBlockWrite(bytes);
}

extern "C" void sstmac_memcopy(uint64_t bytes){
  sstmac::sw::OperatingSystem::currentThread()->parentApp()
    ->computeBlockMemcpy(bytes);
}

extern "C" void sstmac_compute_detailed(uint64_t nflops, uint64_t nintops, uint64_t bytes){
  sstmac::sw::OperatingSystem::currentThread()
    ->computeDetailed(nflops, nintops, bytes);
}

extern "C" void sstmac_compute_detailed_nthr(uint64_t nflops, uint64_t nintops, uint64_t bytes,
                                        int nthread){
  sstmac::sw::OperatingSystem::currentThread()
    ->computeDetailed(nflops, nintops, bytes, nthread);
}

extern "C" void sstmac_computeLoop(uint64_t num_loops, uint32_t nflops_per_loop,
                    uint32_t nintops_per_loop, uint32_t bytes_per_loop){
  sstmac::sw::OperatingSystem::currentThread()->parentApp()
    ->computeLoop(num_loops, nflops_per_loop, nintops_per_loop, bytes_per_loop);
}

extern "C" void sstmac_compute_loop2(uint64_t isize, uint64_t jsize,
                    uint32_t nflops_per_loop,
                    uint32_t nintops_per_loop, uint32_t bytes_per_loop){
  uint64_t num_loops = isize * jsize;
  sstmac::sw::OperatingSystem::currentThread()->parentApp()
    ->computeLoop(num_loops, nflops_per_loop, nintops_per_loop, bytes_per_loop);
}

extern "C" void
sstmac_compute_loop3(uint64_t isize, uint64_t jsize, uint64_t ksize,
                    uint32_t nflops_per_loop,
                    uint32_t nintops_per_loop,
                    uint32_t bytes_per_loop){
  uint64_t num_loops = isize * jsize * ksize;
  sstmac::sw::OperatingSystem::currentThread()->parentApp()
    ->computeLoop(num_loops, nflops_per_loop, nintops_per_loop, bytes_per_loop);
}

extern "C" void
sstmac_compute_loop4(uint64_t isize, uint64_t jsize, uint64_t ksize, uint64_t lsize,
                     uint32_t nflops_per_loop,
                     uint32_t nintops_per_loop,
                     uint32_t bytes_per_loop){
  uint64_t num_loops = isize * jsize * ksize * lsize;
  sstmac::sw::OperatingSystem::currentThread()->parentApp()
    ->computeLoop(num_loops, nflops_per_loop, nintops_per_loop, bytes_per_loop);
}
