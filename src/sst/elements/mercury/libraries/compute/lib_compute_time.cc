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

#include <mercury/components/operating_system.h>
#include <mercury/libraries/compute/lib_compute_inst.h>
#include <mercury/libraries/compute/lib_compute_time.h>
#include <mercury/libraries/compute/compute_event.h>
#include <mercury/operating_system/process/thread.h>
#include <mercury/libraries/compute/lib_compute_memmove.h>
//#include <sstmac/software/process/backtrace.h>

namespace SST {
namespace Hg {

LibComputeTime::LibComputeTime(SST::Params& params, SoftwareId id,
                               OperatingSystem* os) :
  LibComputeTime(params, "libcomputetime", id, os)
{
}

LibComputeTime::LibComputeTime(SST::Params& params,
                                   const char* prefix, SoftwareId id,
                                   OperatingSystem* os) :
  LibCompute(params, prefix, id, os)
{
}

LibComputeTime::LibComputeTime(SST::Params& params,
                                   const std::string& name, SoftwareId id,
                                   OperatingSystem* os) :
  LibCompute(params, name, id, os)
{
}

LibComputeTime::~LibComputeTime()
{
}

void
LibComputeTime::compute(TimeDelta time)
{
  CallGraphAppend(ComputeTime);
  if (time.sec() < 0) {
    sprockit::abort("lib_compute_time can't compute for less than zero time");
  }
  os_->compute(time);
}

void
LibComputeTime::sleep(TimeDelta time)
{
  CallGraphAppend(Sleep);
  os_->sleep(time);
}

}
} //end of namespace
