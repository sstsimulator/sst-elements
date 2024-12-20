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

#include <mercury/libraries/compute/instruction_processor.h>
#include <mercury/components/node_CL.h>
#include <mercury/common/util.h>
#include <sst/core/unitAlgebra.h>

namespace SST {
namespace Hg {

InstructionProcessor::InstructionProcessor(SST::Params& params, MemoryModel* mem, NodeCL* node) :
mem_(mem), nodeCL_(node)
{
  negligible_bytes_ = params.find<SST::UnitAlgebra>("negligible_compute_bytes", "64B").getRoundedValue();

  double parallelism = params.find<double>("parallelism", 1.0);

  freq_ = params.find<SST::UnitAlgebra>("frequency", "2.0 GHz").getValue().toDouble();
  mem_freq_ = freq_;

  tflop_ = TimeDelta(1.0 / freq_ / parallelism);
  tintop_ = tflop_;
  tmemseq_ = TimeDelta(1.0 / mem_freq_);
  tmemrnd_ = tmemseq_;
}


TimeDelta
InstructionProcessor::instructionTime(BasicComputeEvent* cmsg)
{
  basic_instructions_st& st = cmsg->data();
  TimeDelta tsec;
  tsec += st.flops*tflop_;
  tsec += st.intops*tintop_;
  return tsec;
}

void
InstructionProcessor::compute(Event* ev, ExecutionEvent* cb)
{
  BasicComputeEvent* bev = test_cast(BasicComputeEvent, ev);
  basic_instructions_st& st = bev->data();
  int nthread = st.nthread;
  // compute execution time in seconds
  TimeDelta instr_time = instructionTime(bev) / nthread;
  // now count the number of bytes
  uint64_t bytes = st.mem_sequential;
  if (bytes <= negligible_bytes_) {
    nodeCL_->sendDelayedExecutionEvent(instr_time, cb);
  } else {
    //do the full memory modeling
    TimeDelta byte_request_delay = instr_time / bytes;
    mem_->accessFlow(bytes, byte_request_delay, cb);
  }
}

} // namespace Hg
} // namespace SST
