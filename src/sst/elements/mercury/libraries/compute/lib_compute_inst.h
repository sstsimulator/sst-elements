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

#ifndef SSTMAC_SOFTWARE_LIBRARIES_COMPUTE_LIB_COMPUTE_INST_H_INCLUDED
#define SSTMAC_SOFTWARE_LIBRARIES_COMPUTE_LIB_COMPUTE_INST_H_INCLUDED

#include <sstmac/software/libraries/compute/lib_compute_time.h>
#include <sstmac/software/libraries/compute/compute_event_fwd.h>
#include <sstmac/software/process/software_id.h>
#include <sstmac/common/sstmac_config.h>
#include <stdint.h>
#include <sstmac/common/stats/ftq_tag.h>

//these are the default instruction labels

DeclareDebugSlot(lib_compute_inst);

namespace sstmac {
namespace sw {

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
} //end of namespace sstmac

#endif
