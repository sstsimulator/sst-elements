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

#include <mercury/libraries/compute/lib_compute.h>
#include <mercury/operating_system/process/software_id.h>
#include <mercury/common/timestamp.h>
//#include <sstmac/common/sstmac_config.h>

namespace SST {
namespace Hg {

class LibComputeTime :
  public LibCompute
{
 public:
  LibComputeTime(SST::Params& params, SoftwareId id,
                   OperatingSystem* os);

  LibComputeTime(SST::Params& params,
                   const char* prefix, SoftwareId id,
                   OperatingSystem* os);

  LibComputeTime(SST::Params& params,
                   const std::string& name, SoftwareId id,
                   OperatingSystem* os);

  ~LibComputeTime() override;

  void incomingEvent(Event *ev) override{
    Library::incomingEvent(ev);
  }

  void compute(TimeDelta time);

  void sleep(TimeDelta time);

};

} // end namespace Hg
} // end namespace SST
