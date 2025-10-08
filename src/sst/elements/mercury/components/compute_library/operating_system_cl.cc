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

#include <mercury/components/compute_library/operating_system_cl.h>
#include <mercury/components/compute_library/node_cl.h>
#include <mercury/components/operating_system_impl.h>
#include <mercury/common/events.h>
#include <mercury/common/util.h>
#include <mercury/libraries/compute/compute_scheduler.h>
#include <mercury/libraries/compute/compute_event.h>

namespace SST {
namespace Hg {

OperatingSystemAPI* OperatingSystemCL::active_os_;

OperatingSystemCL::OperatingSystemCL(SST::ComponentId_t id, SST::Params& params) : 
  OperatingSystemCLAPI(id,params), OperatingSystemImpl(id,params,this)
{
  // Configure self link to handle event timing
  selfEventLink_ = configureSelfLink("self", time_converter_, new Event::Handler2<Hg::OperatingSystemCL,&OperatingSystemCL::handleEvent>(this));
  assert(selfEventLink_);
  selfEventLink_->setDefaultTimeBase(time_converter_);

  compute_sched_ = new ComputeScheduler( params, this);

  requireDependencies(params);
}

void OperatingSystemCL::setup() {
  app_launcher_ = new AppLauncher(this, npernode_);
  addLaunchRequests(params_);
  for (auto r : requests_)
    selfEventLink_->send(r);
}

void
OperatingSystemCL::execute(COMP_FUNC func, Event *data, int nthr)
{
  int owned_ncores = activeThread()->numActiveCcores();
  if (owned_ncores < nthr){
    compute_sched_->reserveCores(nthr-owned_ncores, active_thread_);
  }

  //initiate the hardware events
  ExecutionEvent* cb = newCallback(this, &OperatingSystemImpl::unblock, active_thread_);

  switch (func) {
    case COMP_INSTR:
      node_cl_->proc()->compute(data, cb);
      break;
    case COMP_TIME: {
      TimedComputeEvent* ev = safe_cast(TimedComputeEvent, data);
      sendDelayedExecutionEvent(ev->data(), cb);
      break;
    }
    default:
      sst_hg_throw_printf(SST::Hg::HgError,"cannot process kernel");
  }

  block();

  if (owned_ncores < nthr){
    compute_sched_->releaseCores(nthr-owned_ncores,active_thread_);
  }
}

} // namespace Hg
} // namespace SST
