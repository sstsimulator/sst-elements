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

#include <string>
#include <mercury/components/operating_system_fwd.h>
#include <mercury/operating_system/libraries/event_library.h>

namespace SST {
namespace Hg {

class Service :
  public EventLibrary
{

 public:
  virtual void start() {}

 protected:
  Service(const std::string& libname, SoftwareId sid, OperatingSystem* os) :
    EventLibrary(libname, sid, os)
  {}

  Service(const char* prefix, SoftwareId sid, OperatingSystem* os) :
    EventLibrary(prefix, sid, os)
  {}

  ~Service() override{}
};

} // end namespace Hg
} // end namespace SST
