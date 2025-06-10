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

#pragma once

#include <sst/core/params.h>
#include <mercury/common/timestamp.h>
#include <mercury/libraries/compute/memory_model.h>

namespace SST {
namespace Hg {

class InstructionProcessor {
 public:

  InstructionProcessor(SST::Params& params,
                        MemoryModel* mem, NodeCL* nd);

  ~InstructionProcessor() { }

  void compute(Event* ev, ExecutionEvent* cb);

 private:
  void setMemopDistribution(double stdev);

  void setFlopDistribution(double stdev);

  TimeDelta instructionTime(BasicComputeEvent* msg);

  MemoryModel* mem_;
  NodeCL* nodeCL_;

  TimeDelta tflop_;
  TimeDelta tintop_;
  TimeDelta tmemseq_;
  TimeDelta tmemrnd_;
  double freq_;
  double mem_freq_;
  uint64_t negligible_bytes_;
};

} // namespace Hg
} // namespace SST
