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
#include <sst/core/factory.h>
#include <mercury/operating_system/launch/app_launcher.h>
#include <mercury/operating_system/process/software_id.h>

namespace SST {
namespace Hg {

extern template class  HgBase<SST::Component>;
extern template class  HgBase<SST::SubComponent>;

AppLauncher::AppLauncher(OperatingSystemAPI* os, unsigned int npernode) :
  os_(os), npernode_(npernode)
{
}

void
AppLauncher::incomingRequest(AppLaunchRequest* req)
{
  Params app_params = req->params();

  if (local_offset.find(req->aid()) == local_offset.end() ) {
    local_offset[req->aid()] = 0;
  }
  unsigned int taskid = os_->addr() * npernode_ + local_offset[req->aid()];
  SoftwareId sid(req->aid(), taskid);
  ++local_offset[req->aid()];

  requireLibraries(app_params);
  setupExe(app_params);
  auto factory = Factory::getFactory();
  App* theapp = factory->Create<App>("hg.UserAppCxxFullMain", app_params, sid, os_);
  theapp->createLibraries();
  os_->startApp(theapp, "my unique name");
}

void
AppLauncher::requireLibraries(SST::Params& params)
{
  std::vector<std::string> libs;
  if (params.contains("libraries")){
    params.find_array<std::string>("libraries", libs);
  }
  else {
    libs.push_back("systemlibrary:SystemLibrary");
  }

  for (auto &str : libs) {
    auto pos = str.find(":");
    std::string libname = str.substr(0, pos);
    os_->requireLibraryForward(libname);
  }
}

void
AppLauncher::setupExe(SST::Params& params)
{
  if (params.contains("exe_library_name")){
    std::string libname = params.find<std::string>("exe_library_name");
    os_->requireLibraryForward(libname);
  }
  UserAppCxxEmptyMain::aliasMains();
  UserAppCxxFullMain::aliasMains();
}

} // end namespace Hg
} // end namespace SST
