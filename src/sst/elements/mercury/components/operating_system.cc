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

#include <mercury/components/operating_system.h>

#include <mercury/components/node.h>
#include <mercury/operating_system/threading/stack_alloc.h>

namespace SST {
namespace Hg {

class DeleteThreadEvent :
    public ExecutionEvent
{
public:
  DeleteThreadEvent(Thread* thr) :
    thr_(thr)
  {
  }

  void execute() override {
    delete thr_;
  }

protected:
  Thread* thr_;
};

OperatingSystem::OperatingSystem(SST::ComponentId_t id, SST::Params& params) :
  OperatingSystemAPI(id,params),
  OperatingSystemImpl(id,params,this)
{
  // Configure self link to handle event timing
  selfEventLink_ = configureSelfLink("self", time_converter_, new Event::Handler2<Hg::OperatingSystem,&OperatingSystem::handleEvent>(this));
  assert(selfEventLink_);
  selfEventLink_->setDefaultTimeBase(time_converter_);

  OperatingSystemImpl::setThreadId(threadId());

  // These are libraries that a SST::Hg::Library depends on. We have core load them early
  // in hopes that everything is in place when the SST::Hg::Library is instanced.
  // Impl class isn't a subcomponent, so it can't make this call.
  requireDependencies(params);
}

void OperatingSystem::setup() {
  app_launcher_ = new AppLauncher(this, npernode_);
  addLaunchRequests(params_);
  for (auto r : requests_)
    selfEventLink_->send(r);
}

} // namespace Hg
} // namespace SST
