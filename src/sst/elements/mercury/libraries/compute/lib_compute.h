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

#include <mercury/operating_system/libraries/library.h>
#include <mercury/common/errors.h>

namespace sstmac {
namespace sw {

class LibCompute :
  public Library
{  
 public:
  ~LibCompute() override{}

 protected:
  LibCompute(SST::Params&  /*params*/,
              const std::string& libname, SoftwareId sid,
              OperatingSystem* os)
    : Library(libname, sid, os) {
  }

  LibCompute(SST::Params&  /*params*/,
              const char* name, SoftwareId sid,
              OperatingSystem* os)
    : Library(name, sid, os)
  {
  }

  void incomingRequest(Request* req) override {
    Library::incomingRequest(req);
  }

};

} // end namespace Hg
} // end namespace SST
