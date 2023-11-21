/**
Copyright 2009-2023 National Technology and Engineering Solutions of Sandia,
LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S. Government
retains certain rights in this software.

Sandia National Laboratories is a multimission laboratory managed and operated
by National Technology and Engineering Solutions of Sandia, LLC., a wholly
owned subsidiary of Honeywell International, Inc., for the U.S. Department of
Energy's National Nuclear Security Administration under contract DE-NA0003525.

Copyright (c) 2009-2023, NTESS

All rights reserved.

Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.

    * Neither the name of the copyright holder nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Questions? Contact sst-macro-help@sandia.gov
*/
#include <operating_system/process/progress_queue.h>
#include <operating_system/libraries/unblock_event.h>
#include <components/operating_system.h>

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
#if SSTMAC_SANITY_CHECK
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

}
}
