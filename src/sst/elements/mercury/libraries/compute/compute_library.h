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

#include <mercury/common/errors.h>
#include <mercury/components/operating_system_CL.h>
#include <mercury/operating_system/libraries/library.h>
#include <mercury/libraries/compute/compute_api.h>
#include <mercury/libraries/compute/compute_event.h>

namespace SST {
namespace Hg {

class ComputeLibrary : public ComputeAPI, public Library
{
 public:

   SST_ELI_REGISTER_DERIVED(
    Library,
    ComputeLibrary,
    "computelibrary",
    "ComputeLibrary",
    SST_ELI_ELEMENT_VERSION(1,0,0),
    "provides basic compute modeling")

  ~ComputeLibrary() override{}

  ComputeLibrary(SST::Params& params, App* parent);

  void sleep(TimeDelta time) override;

  void compute(TimeDelta time) override;

  void computeBlockMemcpy(uint64_t bytes) override;

  void write(uint64_t bytes) override;

  void copy(uint64_t bytes) override;

  private:

  void doAccess(uint64_t bytes);

  void computeLoop(uint64_t num_loops, uint32_t flops_per_loop,
                   uint32_t nintops_per_loop, uint32_t bytes_per_loop);

  void computeDetailed(uint64_t flops, uint64_t nintops, uint64_t bytes,
                       int nthread);

  void computeInst(ComputeEvent* cmsg, int nthr);

private:
  OperatingSystemCL* parent_os_;
  int access_width_bytes_;
  double loop_overhead_;
};

} // end namespace Hg
} // end namespace SST
