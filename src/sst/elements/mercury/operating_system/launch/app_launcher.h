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

#include <mercury/components/operating_system_fwd.h>
#include <mercury/operating_system/launch/app_launch_request.h>
#include <mercury/operating_system/process/app.h>
#include <mercury/operating_system/process/app_id.h>

#include <unordered_map>

namespace SST {
namespace Hg {

/**
 * A launcher that can be cooperatively scheduled by a very naive scheduler.
 */
class AppLauncher
{
 public:
  AppLauncher(OperatingSystem* os, unsigned int npernode);

  ~AppLauncher() {}

  void incomingRequest(AppLaunchRequest* ev);

  void requireLibraries(SST::Params& params);

  void setupExe(SST::Params& params);

 protected:
  bool is_completed_;

  unsigned int npernode_;
  std::unordered_map<AppId, unsigned int> local_offset;
  OperatingSystem* os_;
};

} // end of namespace Hg
} // end of namespace SST
