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

#include <sstmac/common/event_callback.h>
#include <sstmac/common/stats/ftq.h>
#include <sstmac/software/process/ftq_scope.h>
#include <sstmac/software/process/operating_system.h>
#include <sstmac/software/process/backtrace.h>
#include <sstmac/software/libraries/compute/lib_compute_inst.h>
#include <sstmac/software/libraries/compute/compute_event.h>
#include <sprockit/keyword_registration.h>
#include <sprockit/sim_parameters.h>
#include <sstmac/software/process/operating_system.h>
#include <sstmac/software/process/thread.h>

RegisterDebugSlot(lib_compute_inst);

namespace sstmac {
namespace sw {

RegisterKeywords(
 { "lib_compute_unroll_loops", "DEPRECATED: tunes the loop control overhead for compute loop functions" },
 { "lib_compute_loop_overhead", "the number of instructions in control overhead per compute loop" },
 { "lib_compute_access_width", "the size of each memory access access in bits" }
);

LibComputeInst::LibComputeInst(SST::Params& params,
                                   const std::string& libname, SoftwareId id,
                                   OperatingSystem* os)
  : LibComputeTime(params, libname, id, os)
{
  init(params);
}

LibComputeInst::LibComputeInst(SST::Params& params,
                                   SoftwareId sid, OperatingSystem* os) :
  LibComputeTime(params, "computelibinstr%s", sid, os)
{
  init(params);
}

void
LibComputeInst::computeDetailed(
  uint64_t flops,
  uint64_t nintops,
  uint64_t bytes,
  int nthread)
{
  /** Configure the compute request */
  auto cmsg = new ComputeEvent_impl<basic_instructions_st>;
  basic_instructions_st& st = cmsg->data();
  st.flops = flops;
  st.intops = nintops;
  st.mem_sequential = bytes;
  st.nthread = nthread;

  // Do not overwrite an existing tag
  FTQScope scope(os_->activeThread(), FTQTag::compute);

  computeInst(cmsg, nthread);
  delete cmsg;
}

void
LibComputeInst::computeLoop(uint64_t num_loops,
  uint32_t flops_per_loop,
  uint32_t nintops_per_loop,
  uint32_t bytes_per_loop)
{
  /** Configure the compute request */
  uint64_t loop_control_ops = 2 * num_loops * loop_overhead_;
  uint64_t num_intops = loop_control_ops + nintops_per_loop*num_loops;
  computeDetailed(
    flops_per_loop*num_loops,
    num_intops,
    bytes_per_loop*num_loops);
}

void
LibComputeInst::init(SST::Params& params)
{
  if (params.contains("lib_compute_unroll_loops")){
    double loop_unroll = params.find<double>("lib_compute_unroll_loops");
    loop_overhead_ = 1.0 / loop_unroll;
  } else {
    loop_overhead_ = params.find<double>("lib_compute_loop_overhead", 1.0);
  }
}

void
LibComputeInst::computeInst(ComputeEvent* cmsg, int nthr)
{
  CallGraphAppend(ComputeInstructions);
  os_->execute(ami::COMP_INSTR, cmsg, nthr);
}

}
} //end of namespace sstmac
