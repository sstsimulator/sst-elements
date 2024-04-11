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

#include <mercury/components/operating_system.h>
#include <mercury/operating_system/process/simple_compute_scheduler.h>
#include <mercury/operating_system/process/thread.h>
//#include <mercury/operating_system/process/sstmac_config.h>

namespace SST {
namespace Hg {

extern template class  HgBase<SST::Component>;
extern template class  HgBase<SST::SubComponent>;

void
SimpleComputeScheduler::reserveCores(int ncores, Thread* thr)
{
//#if SST_HG_SANITY_CHECK
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
//#if SST_HG_SANITY_CHECK
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
