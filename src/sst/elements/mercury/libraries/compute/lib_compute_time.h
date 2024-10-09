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

#ifndef SSTMAC_SOFTWARE_LIBRARIES_COMPUTE_LIB_COMPUTE_TIME_H_INCLUDED
#define SSTMAC_SOFTWARE_LIBRARIES_COMPUTE_LIB_COMPUTE_TIME_H_INCLUDED

#include <sstmac/software/libraries/compute/lib_compute.h>
#include <sstmac/software/process/software_id.h>
#include <sstmac/common/timestamp.h>
#include <sstmac/common/sstmac_config.h>

namespace sstmac {
namespace sw {

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

}
} //end of namespace sstmac

#endif
