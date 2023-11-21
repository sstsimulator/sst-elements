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

#include <mercury/common/events.h>
#include <mercury/common/thread_safe_new.h>
#include <mercury/components/operating_system_fwd.h>
#include <mercury/operating_system/process/thread_fwd.h>

namespace SST {
namespace Hg {

class UnblockEvent : 
  public ExecutionEvent,
  public SST::Hg::thread_safe_new<UnblockEvent>
{

 public:
  UnblockEvent(OperatingSystem* os, Thread* thr);

  void execute() override;

 protected:
  OperatingSystem* os_;
  Thread* thr_;

};

class TimeoutEvent : public ExecutionEvent
{

 public:
  TimeoutEvent(OperatingSystem* os, Thread* thr);

  void execute() override;

 protected:
  OperatingSystem* os_;
  Thread* thr_;
  uint64_t counter_;

};

} // end namespace Hg
} // end namespace SST
