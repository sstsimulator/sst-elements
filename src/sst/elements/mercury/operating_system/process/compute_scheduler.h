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
#include <sst/core/eli/elementinfo.h>
//#include <mercury/common/factory.h>
#include <sst/core/eli/elementbuilder.h>
#include <mercury/hardware/common/flow.h>
#include <mercury/components/operating_system_fwd.h>
#include <mercury/operating_system/process/thread_fwd.h>
//#include <sprockit/debug.h>

//DeclareDebugSlot(compute_scheduler)

namespace SST {
namespace Hg {

class ComputeScheduler
{
 public:
  SST_ELI_DECLARE_BASE(ComputeScheduler)
  SST_ELI_DECLARE_DEFAULT_INFO()
  SST_ELI_DECLARE_CTOR(SST::Params&, OperatingSystem*, int/*ncores*/, int/*nsockets*/)

  ComputeScheduler(SST::Params&  /*params*/, OperatingSystem* os,
                    int ncores, int nsockets) :
    ncores_(ncores), 
    nsocket_(nsockets),
    os_(os)
  {
  }

  virtual ~ComputeScheduler() {}


  int ncores() const {
    return ncores_;
  }

  int nsockets() const {
    return nsocket_;
  }

  /**
   * @brief reserve_core
   * @param thr   The physical thread requesting to compute
   */
  virtual void reserveCores(int ncore, Thread* thr) = 0;
  
  virtual void releaseCores(int ncore, Thread* thr) = 0;


 protected:
  int ncores_;
  int nsocket_;
  int cores_per_socket_;
  OperatingSystem* os_;

};

} // end namespace Hg
} // end namespace SST
