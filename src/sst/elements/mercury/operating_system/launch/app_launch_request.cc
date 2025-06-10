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

#include <mercury/operating_system/launch/app_launch_request.h>
#include <external/json.hpp>

namespace SST {
namespace Hg {

  AppLaunchRequest::AppLaunchRequest(SST::Params& params,
                         AppId aid,
                         const std::string& name) :
    aid_(aid),
    name_(name)
  {
    // params aren't actually serializable, store a json string instead
    std::set<std::string> keys = params.getKeys();
    nlohmann::json j;
    for (auto &k : keys) {
        j[k] = params.find<std::string>(k,"not found");
      }
    param_string_ = j.dump();
  }

  SST::Params
  AppLaunchRequest::params() {
    // convert the string back to json object
    nlohmann::json j = nlohmann::json::parse(param_string_);

    // reconstruct the app params
    SST::Params app_params;
    for (auto& elem : j.items()) {
        app_params.insert(elem.key(),elem.value());
      }
    return app_params;
  }

} // end of namespace Hg
} // end of namespace SST
