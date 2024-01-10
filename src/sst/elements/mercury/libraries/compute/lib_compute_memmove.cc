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


#include <sst/core/params.h>
#include <mercury/components/operating_system.h>
#include <mercury/libraries/compute/lib_compute_inst.h>
#include <mercury/libraries/compute/compute_event.h>
#include <mercury/operating_system/process/thread.h>
#include <mercury/libraries/compute/lib_compute_memmove.h>
//#include <sstmac/common/event_callback.h>
//#include <sstmac/software/process/backtrace.h>

namespace SST {
namespace Hg {

LibComputeMemmove::LibComputeMemmove(SST::Params& params,
                                     SoftwareId id, OperatingSystem* os) :
  LibComputeMemmove(params, "libmemmove", id, os)
{
}

LibComputeMemmove::LibComputeMemmove(SST::Params& params,
                                     const char* prefix, SoftwareId sid,
                                     OperatingSystem* os) :
  LibComputeInst(params, prefix, sid, os)
{
  access_width_bytes_ = params.find<int>("lib_compute_access_width", 64) / 8;
}

void
LibComputeMemmove::doAccess(uint64_t bytes)
{
  //if (bytes == 0){
  //  return;
  //}
  uint64_t num_loops = bytes / access_width_bytes_;
  int nflops = 0;
  int nintops = 1; //memmove instruction
  computeLoop(num_loops, nflops, nintops, access_width_bytes_);
}

void
LibComputeMemmove::read(uint64_t bytes)
{
  CallGraphAppend(memread);
  doAccess(bytes);
}

void
LibComputeMemmove::write(uint64_t bytes)
{
  CallGraphAppend(memwrite);
  doAccess(bytes);
}

void
LibComputeMemmove::copy(uint64_t bytes)
{
  CallGraphAppend(memcopy);
  doAccess(bytes);
}


}
} //end of namespace
