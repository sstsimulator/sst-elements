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

#include <mercury/libraries/compute/lib_compute_time.h>
#include <mercury/libraries/compute/compute_event_fwd.h>
#include <mercury/operating_system/process/software_id.h>
#include <stdint.h>

namespace SST {
namespace Hg {

class LibComputeInst :
  public LibComputeTime
{
 public:
  LibComputeInst(SST::Params& params, SoftwareId id, OperatingSystem* os);

  LibComputeInst(SST::Params& params, const std::string& libname,
                   SoftwareId id, OperatingSystem* os);

  ~LibComputeInst() override { }

  void computeInst(ComputeEvent* msg, int nthr = 1);

  void computeDetailed(uint64_t flops,
    uint64_t nintops,
    uint64_t bytes,
    int nthread = 1);

  void computeLoop(uint64_t nloops,
    uint32_t flops_per_loop,
    uint32_t intops_per_loop,
    uint32_t bytes_per_loop);

  void incomingEvent(Event *ev) override {
    Library::incomingEvent(ev);
  }

 protected:
  double loop_overhead_;

 private:
  void init(SST::Params& params);

};

}
} //end of namespace
