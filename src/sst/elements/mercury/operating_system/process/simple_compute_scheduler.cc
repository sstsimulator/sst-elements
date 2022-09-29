/**
Copyright 2009-2021 National Technology and Engineering Solutions of Sandia, 
LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S.  Government 
retains certain rights in this software.

Sandia National Laboratories is a multimission laboratory managed and operated
by National Technology and Engineering Solutions of Sandia, LLC., a wholly 
owned subsidiary of Honeywell International, Inc., for the U.S. Department of 
Energy's National Nuclear Security Administration under contract DE-NA0003525.

Copyright (c) 2009-2021, NTESS

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

#include <components/operating_system.h>
#include <operating_system/process/simple_compute_scheduler.h>
#include <operating_system/process/thread.h>
//#include <operating_system/process/sstmac_config.h>

namespace SST {
namespace Hg {

void
SimpleComputeScheduler::reserveCores(int ncores, Thread* thr)
{
//#if SSTMAC_SANITY_CHECK
//  if (ncore_active_ > ncores_){
//    spkt_abort_printf(
//      "simple_compute_scheduler::reserve_core: %d cores active, only %d cores total",
//      ncore_active_, ncores_);
//  }
//#endif
  int total_cores_needed = ncores + ncore_active_;
  while (total_cores_needed > ncores_){
//    debug_printf(sprockit::dbg::compute_scheduler,
//        "Need %d cores, have %d for thread %ld - blocking",
//        ncores, ncores_ - ncore_active_, thr->threadId());
    pending_threads_.emplace_back(ncores, thr);
    os_->block();
    //we can accidentally unblock due to "race" conditions
    //reset the core check to make sure we have what we need
    total_cores_needed = ncores + ncore_active_;
  }
//#if SSTMAC_SANITY_CHECK
//  if (ncores > (ncores_ - ncore_active_)){
//    spkt_abort_printf(
//      "simple_compute_scheduler::reserve_core: %d cores free, but needed %d for thread %d",
//      ncores_ - ncore_active_, ncores_, thr->threadId());
//  }
//#endif
//  debug_printf(sprockit::dbg::compute_scheduler,
//      "Reserved %d cores for thread %ld",
//       ncores, thr->threadId());
  //no worrying about masks
  for (int i=ncore_active_; i < ncore_active_ + ncores; ++i){
    thr->addActiveCore(i);
  }
  ncore_active_ += ncores;
}

void
SimpleComputeScheduler::releaseCores(int ncores, Thread* thr)
{
  ncore_active_ -= ncores;
//  debug_printf(sprockit::dbg::compute_scheduler,
//      "Released %d cores for thread %ld - now %d active, %d free",
//       ncores, thr->threadId(), ncore_active_, ncores_ - ncore_active_);
  for (int i=0; i < ncores; ++i){
    thr->popActiveCore();
  }


  int nfree_cores = ncores_ - ncore_active_;
  Thread* to_unblock = nullptr;
  for (auto iter = pending_threads_.begin(); iter != pending_threads_.end(); ++iter){
    auto pair = *iter;
    int ncores_needed = pair.first;
//    debug_printf(sprockit::dbg::compute_scheduler,
//        "Thread %d trying to restart thread %d with %d free cores - need %d",
//         thr->threadId(), pair.second->threadId(),
//         nfree_cores, ncores_needed);
    if (nfree_cores >= ncores_needed){
      pending_threads_.erase(iter);
      to_unblock = pair.second;
      break;
    }
  }
  if (to_unblock){
    os_->unblock(to_unblock);
  }
}

} // end namespace Hg
} // end namespace SST
