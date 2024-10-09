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

#include <libraries/system/system_api.h>
#include <common/timestamp.h>
#include <common/thread_safe_new.h>
#include <components/operating_system.h>
#include <operating_system/process/thread.h>
#include <operating_system/process/app.h>
#include <operating_system/libraries/unblock_event.h>
//#include <sstmac/skeleton.h>

namespace SST {
namespace Hg {

extern template class  HgBase<SST::Component>;
extern template SST::TimeConverter* HgBase<SST::SubComponent>::time_converter_;

using os = OperatingSystem;

systemAPI::systemAPI(SST::Params& params, App* app,
                     SST::Component* comp) :
  API(params,app,comp)
{ }

double
systemAPI::ssthg_block()
{
  os::currentOs()->block();
  return os::currentOs()->now().sec();
}

unsigned int
systemAPI::ssthg_sleep(unsigned int secs){
    os* cos = os::currentOs();
    Thread* t = cos->activeThread();
    UnblockEvent* ev = new UnblockEvent(cos, t);
    cos->sendDelayedExecutionEvent(TimeDelta(secs, TimeDelta::one_second), ev);
    int ncores = t->numActiveCcores();
    //when sleeping, release all cores
    cos->releaseCores(ncores, t);
    cos->block();
    cos->reserveCores(ncores, t);
    return 0;
}

//extern "C" unsigned sstmac_sleepUntil(double t){
//  os::currentOs()->sleepUntil(Timestamp(t));
//  return 0;
//}

//extern "C" int sstmac_usleep(unsigned int usecs){
//  os::currentOs()->sleep(TimeDelta(usecs, TimeDelta::one_microsecond));
//  return 0;
//}

//extern "C" int sstmac_nanosleep(unsigned int nanosecs){
//  os::currentOs()->sleep(TimeDelta(nanosecs, TimeDelta::one_nanosecond));
//  return 0;
//}

//extern "C" int sstmac_msleep(unsigned int msecs){
//  os::currentOs()->sleep(TimeDelta(msecs, TimeDelta::one_millisecond));
//  return 0;
//}

//extern "C" int sstmac_fsleep(double secs){
//  sstmac::sw::OperatingSystem::currentThread()->parentApp()->sleep(sstmac::TimeDelta(secs));
//  return 0;
//}

//extern "C" void sstmac_memread(uint64_t bytes){
//  sstmac::sw::OperatingSystem::currentThread()->parentApp()
//    ->computeBlockRead(bytes);
//}

//extern "C" void sstmac_memwrite(uint64_t bytes){
//  sstmac::sw::OperatingSystem::currentThread()->parentApp()
//    ->computeBlockWrite(bytes);
//}

//extern "C" void sstmac_memcopy(uint64_t bytes){
//  sstmac::sw::OperatingSystem::currentThread()->parentApp()
//    ->computeBlockMemcpy(bytes);
//}

} // end namespace Hg
} // end namespace SST
