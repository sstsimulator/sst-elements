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

#include <mercury/operating_system/process/progress_queue.h>
#include <mercury/operating_system/libraries/unblock_event.h>
#include <mercury/components/operating_system.h>

namespace SST {
namespace Hg {

void
ProgressQueue::block(std::list<Thread*>& q, double timeout){
  Thread* thr = os->activeThread();
  q.push_back(thr);
  if (timeout > 0){
    os->blockTimeout(TimeDelta(timeout));
  } else {
    os->block();
  }
  q.remove(thr);
}

void
ProgressQueue::unblock(std::list<Thread*>& q){
#if SST_HG_SANITY_CHECK
  if (q.empty()){
    spkt_abort_printf("trying to unblock CQ, but there are no pending threads");
  }
#endif
  auto* thr = q.front();
  //don't erase here - this gets erased in the block call
  os->unblock(thr);
}

void
PollingQueue::block()
{
  int max_empty_polls = 2;
  Timestamp now = os->now();
  if (now == last_check_){
    ++num_empty_calls_;
  } else {
    num_empty_calls_ = 0;
    last_check_ = now;
  }
  if (num_empty_calls_ >= max_empty_polls){
    ProgressQueue::block(pending_threads_, -1);
  }
}

void
PollingQueue::unblock()
{
  num_empty_calls_ = 0;
  ProgressQueue::unblock(pending_threads_);
}

} // end namespace Hg
} // end namespace SST
