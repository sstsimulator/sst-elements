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
