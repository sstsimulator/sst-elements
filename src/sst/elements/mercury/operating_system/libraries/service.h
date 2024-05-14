// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
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
#include <mercury/operating_system/libraries/library.h>

namespace SST {
namespace Hg {

class Service :
  public Library
{

 public:
  virtual void start() {}

 protected:
  Service(const std::string& libname, SoftwareId sid, OperatingSystem* os) :
    Library(libname, sid, os)
  {}

  Service(const char* prefix, SoftwareId sid, OperatingSystem* os) :
    Library(prefix, sid, os)
  {}

  ~Service() override{}
};

} // end namespace Hg
} // end namespace SST
