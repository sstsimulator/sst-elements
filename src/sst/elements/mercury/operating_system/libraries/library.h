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

#pragma once

#include <sst/core/params.h>
#include <mercury/common/events.h>
#include <mercury/common/request_fwd.h>
#include <mercury/common/node_address.h>
#include <mercury/components/operating_system_fwd.h>
#include <mercury/operating_system/process/software_id.h>
#include <mercury/operating_system/libraries/library_fwd.h>
#include <mercury/common/hg_printf.h>
#include <map>


namespace SST {
namespace Hg {

class Library
{
 public:
  std::string toString() const {
    return libname_;
  }

  const std::string& libName() const {
    return libname_;
  }

  virtual void incomingEvent(Event* ev) = 0;

  virtual void incomingRequest(Request* req) = 0;

  OperatingSystem* os() const {
    return os_;
  }

  SoftwareId sid() const {
    return sid_;
  }

  int aid() const {
    return sid_.app_;
  }

  NodeId addr() const {
    return addr_;
  }

  virtual ~Library();

 protected:
  Library(const std::string& libname, SoftwareId sid, OperatingSystem* os);

  Library(const char* prefix, SoftwareId sid, OperatingSystem* os) :
    Library(standardLibname(prefix, sid), sid, os)
  {
  }

  static std::string standardLibname(const char* prefix, SoftwareId sid){
    return standardLibname(prefix, sid.app_, sid.task_);
  }

  static std::string standardLibname(const char* prefix, AppId aid, TaskId tid){
    std::string app_prefix = standardAppPrefix(prefix, aid);
    return standardAppLibname(app_prefix.c_str(), tid);
  }

  static std::string standardAppLibname(const char* prefix, TaskId tid){
    return SST::Hg::sprintf("%s-%d", prefix, tid);
  }

  static std::string standardAppPrefix(const char* prefix, AppId aid){
    return SST::Hg::sprintf("%s-%d", prefix, aid);
  }

 protected:
  OperatingSystem* os_;
  SoftwareId sid_;
  NodeId addr_;

 private:
  std::string libname_;

};

} // end namespace Hg
} // end namespace SST
