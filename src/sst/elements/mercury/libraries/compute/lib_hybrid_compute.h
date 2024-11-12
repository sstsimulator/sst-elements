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

#include <mercury/libraries/compute/lib_compute_inst.h>
#include <mercury/libraries/compute/lib_compute_memmove.h>

namespace SST {
namespace Hg {

class lib_hybrid_compute :
  public LibComputeInst,
  public libComputeMemmove
{

 public:

  virtual
  ~lib_hybrid_compute {
  }

 protected:
  lib_hybrid_compute(SoftwareId id);

  lib_hybrid_compute(const std::string& id);

};

} // end namespace Hg
} // end namespace SST
