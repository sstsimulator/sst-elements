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

#include <mercury/components/operating_system.h>
#include <mercury/common/timestamp.h>
#include <mercury/common/thread_safe_new.h>
#include <mercury/common/skeleton.h>
#include <mercury/operating_system/process/thread.h>
#include <mercury/operating_system/process/app.h>
#include <mercury/libraries/compute/compute_api.h>


using SST::Hg::TimeDelta;
using SST::Hg::Timestamp;
using os = SST::Hg::OperatingSystem;

extern "C" double sst_hg_block()
{
  os::currentOs()->block();
  return os::currentOs()->now().sec();
}

extern "C" unsigned int sst_hg_sleep(unsigned int secs){
  os::currentOs()->sleep(TimeDelta(secs, TimeDelta::one_second));
  return 0;
}

extern "C" unsigned sst_hg_sleepUntil(double t){
  os::currentOs()->sleepUntil(Timestamp(t));
  return 0;
}

extern "C" int sst_hg_usleep(unsigned int usecs){
  os::currentOs()->sleep(TimeDelta(usecs, TimeDelta::one_microsecond));
  return 0;
}

extern "C" int sst_hg_nanosleep(unsigned int nanosecs){
  os::currentOs()->sleep(TimeDelta(nanosecs, TimeDelta::one_nanosecond));
  return 0;
}

extern "C" int sst_hg_msleep(unsigned int msecs){
  os::currentOs()->sleep(TimeDelta(msecs, TimeDelta::one_millisecond));
  return 0;
}

extern "C" int sst_hg_fsleep(double secs){
  sstmac::sw::OperatingSystem::currentThread()->parentApp()->sleep(sstmac::TimeDelta(secs));
  return 0;
}

extern "C" void sst_hg_compute(double secs){
  sstmac::sw::OperatingSystem::currentOs()->compute(TimeDelta(secs));
}

extern "C" void sst_hg_memread(uint64_t bytes){
  sstmac::sw::OperatingSystem::currentThread()->parentApp()
    ->computeBlockRead(bytes);
}

extern "C" void sst_hg_memwrite(uint64_t bytes){
  sstmac::sw::OperatingSystem::currentThread()->parentApp()
    ->computeBlockWrite(bytes);
}

extern "C" void sst_hg_memcopy(uint64_t bytes){
  sstmac::sw::OperatingSystem::currentThread()->parentApp()
    ->computeBlockMemcpy(bytes);
}

extern "C" void sst_hg_compute_detailed(uint64_t nflops, uint64_t nintops, uint64_t bytes){
  sstmac::sw::OperatingSystem::currentThread()
    ->computeDetailed(nflops, nintops, bytes);
}

extern "C" void sst_hg_compute_detailed_nthr(uint64_t nflops, uint64_t nintops, uint64_t bytes,
                                        int nthread){
  sstmac::sw::OperatingSystem::currentThread()
    ->computeDetailed(nflops, nintops, bytes, nthread);
}

extern "C" void sst_hg_computeLoop(uint64_t num_loops, uint32_t nflops_per_loop,
                    uint32_t nintops_per_loop, uint32_t bytes_per_loop){
  sstmac::sw::OperatingSystem::currentThread()->parentApp()
    ->computeLoop(num_loops, nflops_per_loop, nintops_per_loop, bytes_per_loop);
}

extern "C" void sst_hg_compute_loop2(uint64_t isize, uint64_t jsize,
                    uint32_t nflops_per_loop,
                    uint32_t nintops_per_loop, uint32_t bytes_per_loop){
  uint64_t num_loops = isize * jsize;
  sstmac::sw::OperatingSystem::currentThread()->parentApp()
    ->computeLoop(num_loops, nflops_per_loop, nintops_per_loop, bytes_per_loop);
}

extern "C" void
sst_hg_compute_loop3(uint64_t isize, uint64_t jsize, uint64_t ksize,
                    uint32_t nflops_per_loop,
                    uint32_t nintops_per_loop,
                    uint32_t bytes_per_loop){
  uint64_t num_loops = isize * jsize * ksize;
  sstmac::sw::OperatingSystem::currentThread()->parentApp()
    ->computeLoop(num_loops, nflops_per_loop, nintops_per_loop, bytes_per_loop);
}

extern "C" void
sst_hg_compute_loop4(uint64_t isize, uint64_t jsize, uint64_t ksize, uint64_t lsize,
                     uint32_t nflops_per_loop,
                     uint32_t nintops_per_loop,
                     uint32_t bytes_per_loop){
  uint64_t num_loops = isize * jsize * ksize * lsize;
  sstmac::sw::OperatingSystem::currentThread()->parentApp()
    ->computeLoop(num_loops, nflops_per_loop, nintops_per_loop, bytes_per_loop);
}
