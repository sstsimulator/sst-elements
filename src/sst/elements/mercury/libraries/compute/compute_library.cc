// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
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
#include <mercury/common/errors.h>
#include <mercury/components/operating_system_CL.h>
#include <mercury/libraries/compute/compute_library.h>
#include <mercury/operating_system/process/app.h>

namespace SST {
namespace Hg {

ComputeLibrary::ComputeLibrary(SST::Params &params, App *parent)
    : Library(params, parent) 
{
  parent_os_ = dynamic_cast<OperatingSystemCL*>(parent->os());

  //FIXME: do 64 bit accesses make sense? (cache line?)
  access_width_bytes_ = params.find<int>("compute_library_access_width", 64) / 8;

  loop_overhead_ = params.find<double>("compute_library_loop_overhead", 1.0);
}

void 
ComputeLibrary::compute(TimeDelta time) 
{
  if (time.sec() < 0) {
    sst_hg_abort_printf("ComputeLibrary can't compute for less than zero time");
  }
  parent_os_->blockTimeout(time);
}

void
ComputeLibrary::sleep(TimeDelta time)
{
  parent_os_->blockTimeout(time);
}

void ComputeLibrary::computeBlockMemcpy(uint64_t bytes) { 
  copy(bytes); 
}

void
ComputeLibrary::write(uint64_t bytes)
{
  doAccess(bytes);
}

void
ComputeLibrary::copy(uint64_t bytes)
{
  doAccess(bytes);
}

void
ComputeLibrary::doAccess(uint64_t bytes)
{
  uint64_t num_loops = bytes / access_width_bytes_;
  int nflops = 0;
  int nintops = 1; //memmove instruction
  computeLoop(num_loops, nflops, nintops, access_width_bytes_);
}

void
ComputeLibrary::computeLoop(uint64_t num_loops,
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
    bytes_per_loop*num_loops, 1);
}

void
ComputeLibrary::computeDetailed(
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

  // // Do not overwrite an existing tag
  // FTQScope scope(os_->activeThread(), FTQTag::compute);

  computeInst(cmsg, nthread);
  delete cmsg;
}

void
ComputeLibrary::computeInst(ComputeEvent* cmsg, int nthr)
{
  parent_os_->execute(OperatingSystemCL::COMP_INSTR, cmsg, nthr);
}

} // end namespace Hg
} // end namespace SST
