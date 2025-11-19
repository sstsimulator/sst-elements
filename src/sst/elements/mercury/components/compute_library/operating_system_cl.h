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

#include <mercury/components/compute_library/node_cl.h>
#include <mercury/components/node_base.h>
#include <mercury/components/compute_library/operating_system_cl_api.h>
#include <mercury/libraries/compute/compute_event.h>
#include <mercury/libraries/compute/compute_scheduler.h>
#include <mercury/operating_system/launch/app_launcher.h>
#include <sst/core/link.h>

namespace SST {
namespace Hg {

extern template SST::TimeConverter HgBase<SST::SubComponent>::time_converter_;

class OperatingSystemCL : public OperatingSystemCLAPI, public OperatingSystemImpl {

  public:

  SST_ELI_REGISTER_SUBCOMPONENT(SST::Hg::OperatingSystemCL, "hg",
                                "OperatingSystemCL",
                                SST_ELI_ELEMENT_VERSION(0, 0, 1),
                                "Mercury Operating System using ComputeLibrary",
                                SST::Hg::OperatingSystemCLAPI)

  OperatingSystemCL(SST::ComponentId_t id, SST::Params &params);

  ~OperatingSystemCL() { delete compute_sched_; }

  void init(unsigned phase) override {
    OperatingSystemImpl::init(phase);
  }

  void setup() override;

  NodeCL* nodeCL() const override {
    return node_cl_;
  }

  void execute(COMP_FUNC, Event *data, int nthr = 1) override;

  static size_t stacksize() {
    return sst_hg_global_stacksize;
  }

  // Forward calls to OperatingSystemImpl here
  #include <mercury/components/operating_system_call_forward.h>

  void setParentNode(NodeCL *parent) override {
    node_cl_ = parent;
    node_base_ = dynamic_cast<NodeBase*>(parent);
    OperatingSystemImpl::setParentNode(node_base_);
  }

protected:
  SST::Hg::NodeBase *node_base_;
  std::map<std::string, Library *> internal_apis_;

private:

  Link* selfEventLink_;
  SST::Hg::NodeCL* node_cl_;
  static OperatingSystemAPI* active_os_;
  ComputeScheduler *compute_sched_;

  void requireDependencies(SST::Params &params) {
    std::vector<std::string> libs;
    params.find_array<std::string>("app1.dependencies", libs);
    for (const std::string &lib : libs) {
      requireLibrary(lib);
    }
  }
};

} // namespace Hg
} // namespace SST
