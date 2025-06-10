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

#include <mercury/components/operating_system.h>
#include <mercury/operating_system/libraries/unblock_event.h>
#include <mercury/operating_system/process/thread.h>

namespace SST {
namespace Hg {

extern template class  HgBase<SST::Component>;
extern template class  HgBase<SST::SubComponent>;

UnblockEvent::UnblockEvent(OperatingSystem *os, Thread *thr)
  :  os_(os), thr_(thr)
{
}

void
UnblockEvent::execute()
{
  os_->unblock(thr_);
}

TimeoutEvent::TimeoutEvent(OperatingSystem* os, Thread* thr) :
  os_(os), thr_(thr), counter_(thr->blockCounter())
{
}


void
TimeoutEvent::execute()
{
  if (thr_->blockCounter() == counter_){
    thr_->setTimedOut(true);
    os_->unblock(thr_);
  } else {
    //already fired, no timeout
  }
}


} // end namespace Hg
} // end namespace SST
