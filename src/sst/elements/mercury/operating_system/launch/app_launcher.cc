// Copyright 2009-2023 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2023, NTESS
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

#include <common/factory.h>
#include <operating_system/launch/app_launcher.h>
#include <operating_system/process/software_id.h>

namespace SST {
namespace Hg {

extern template class  HgBase<SST::Component>;
extern template class  HgBase<SST::SubComponent>;

AppLauncher::AppLauncher(OperatingSystem* os) :
  os_(os)
{
}

void
AppLauncher::incomingRequest(AppLaunchRequest* req)
{
  Params app_params = req->params();
  SoftwareId sid(req->aid(), os_->addr()-1);

  std::string app_name;
  if (app_params.count("label")) {
      app_name = app_params.find<std::string>("label","");
  }
  else app_name = app_params.find<std::string>("name","");
  //std::cerr << "launching app_name " << app_name << std::endl;
  std::string exe = app_params.find<std::string>("exe","");
  //std::cerr << "launching exe " << exe << std::endl;

  App::dlopenCheck(req->aid(), app_params);
  //app_params.print_all_params(std::cerr);
  App* theapp = create<App>("hg", app_name, app_params, sid, os_);
  //theapp->setUniqueName(lreq->uniqueName());
  //int intranode_rank = num_apps_launched_[lreq->aid()]++;
  //int core_affinity = lreq->coreAffinity(intranode_rank);
  //if (core_affinity != Thread::no_core_affinity){
  //    theapp->setAffinity(core_affinity);
  //  }

  os_->startApp(theapp, "my unique name");
}

//void
//AppLauncher::start()
//{
//  Service::start();
//  if (!os_) {
//    spkt_throw_printf(sprockit::ValueError,
//                     "AppLauncher::start: OS hasn't been registered yet");
//  }
//}

//hw::NetworkMessage*
//LaunchRequest::cloneInjectionAck() const
//{
//  spkt_abort_printf("launch event should never be cloned for injection");
//  return nullptr;
//}

//int
//StartAppRequest::coreAffinity(int  /*intranode_rank*/) const
//{
//  return Thread::no_core_affinity;
//}

//void
//StartAppRequest::serialize_order(serializer &ser)
//{
//  LaunchRequest::serialize_order(ser);
//  ser & unique_name_;
//  ser & app_params_;
//  mapping_ = TaskMapping::serialize_order(aid(), ser);
//}

//std::string
//StartAppRequest::toString() const
//{
//  return sprockit::sprintf("start_app_event: app=%d task=%d node=%d", aid(), tid(), toaddr());
//}

//std::string
//JobStopRequest::toString() const
//{
//  return sprockit::sprintf("job_stop_event: app=%d task=%d node=%d", aid(), tid(), fromaddr());
//}


}
}
