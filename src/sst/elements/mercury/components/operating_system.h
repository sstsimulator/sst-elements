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

#include <mercury/common/component.h>
#include <sst/core/link.h>

#include <mercury/components/operating_system_api.h>
#include <mercury/components/operating_system_impl.h>
#include <mercury/components/node_base_fwd.h>
#include <mercury/operating_system/launch/app_launcher.h>

#include <cstdint>
#include <memory>

namespace SST {
namespace Hg {

class OperatingSystem : public SST::Hg::OperatingSystemAPI, public SST::Hg::OperatingSystemImpl {

public:
  SST_ELI_REGISTER_SUBCOMPONENT(SST::Hg::OperatingSystem, "hg",
                                "OperatingSystem",
                                SST_ELI_ELEMENT_VERSION(0, 0, 1),
                                "Mercury Operating System",
                                SST::Hg::OperatingSystemAPI)

  OperatingSystem(SST::ComponentId_t id, SST::Params &params);

  ~OperatingSystem() {}

  void init(unsigned phase) override {
    OperatingSystemImpl::init(phase);
  }

  void setup() override;

  static size_t stacksize() {
    return sst_hg_global_stacksize;
  }

  // Forward calls to OperatingSystemImpl here
  #include <mercury/components/operating_system_call_forward.h>

protected:

  NodeBase* node_;
  std::map<std::string, Library*> internal_apis_;

 private:

  Link* selfEventLink_;

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
