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

#include <sst/core/params.h>
#include <sst/core/unitAlgebra.h>

#include <mercury/common/errors.h>
#include <mercury/operating_system/process/thread_info.h>
#include <mercury/operating_system/threading/stack_alloc.h>
#include <mercury/operating_system/threading/stack_alloc_chunk.h>
#include <mercury/operating_system/threading/thread_lock.h>

#include <unistd.h>

namespace SST {
namespace Hg {

StackAlloc::chunk_set StackAlloc::chunks_;
size_t StackAlloc::suggested_chunk_ = 0;
size_t StackAlloc::stacksize_ = 0;
bool StackAlloc::protect_stacks_ = false;

extern "C" {
int sst_hg_global_stacksize = 0;
}

void
StackAlloc::init(SST::Params& params)
{
  if (stacksize_ != 0){
    return; //we are good
  }

  sst_hg_global_stacksize = params.find<SST::UnitAlgebra>("stack_size", "131072B").getRoundedValue();
  //must be a multiple of 4096
  int stack_rem = sst_hg_global_stacksize % 4096;
  if (stack_rem != 0){
    sst_hg_global_stacksize += (4096 - stack_rem);
  }
  std::string chunk = Hg::sprintf("%dB", 8*sst_hg_global_stacksize);
  suggested_chunk_ = params.find<SST::UnitAlgebra>("stack_chunk_size", chunk).getRoundedValue();
  stacksize_ = sst_hg_global_stacksize;

  protect_stacks_ = params.find<bool>("protect_stacks", false);
}

void
StackAlloc::chunk_set::clear()
{
  for (chunk* ch : allocations){
    //this leads to an munmap during cxa_finalize
    //which segfaults for no apparent reason
    //delete ch;
  }
  allocations.clear();
  available.clear();
}

//
// Get a stack memory region.
//
void*
StackAlloc::alloc()
{
  static thread_lock lock;
  lock.lock();
  if (stacksize_ == 0) {
    sst_hg_throw_printf(ValueError, "stackalloc::stacksize was not initialized");
  }

  if(chunks_.available.empty()){
    // grab a new chunk.
    chunk* new_chunk = new chunk(stacksize_, suggested_chunk_, protect_stacks_);
    chunks_.allocations.push_back(new_chunk);
    void* buf = new_chunk->getNextStack();
    while (buf != nullptr){
      chunks_.available.push_back(buf);
      buf = new_chunk->getNextStack();
    }
  }
  void *buf = chunks_.available.back();
  chunks_.available.pop_back();
  lock.unlock();
  return buf;
}

//
// Return the given memory region.
//
void StackAlloc::free(void* buf)
{
  static thread_lock lock; 
  lock.lock();
  chunks_.available.push_back(buf);
  lock.unlock();
}


} // end pf namespace sw
} // end of namespace sstmac
