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

namespace SST {
namespace Hg {

class ComputeAPI
{  
 public:
  virtual void sleep(TimeDelta time) = 0;
  virtual void compute(TimeDelta time) = 0;
  virtual void computeBlockMemcpy(uint64_t bytes) = 0;
  virtual void write(uint64_t bytes) = 0;
  virtual void copy(uint64_t bytes) = 0;
};

} // end namespace Hg
} // end namespace SST
