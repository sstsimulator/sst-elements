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

#include <sst/core/params.h>
#include <sst/core/unitAlgebra.h>

#include <common/errors.h>
#include <operating_system/process/thread_info.h>
#include <operating_system/threading/stack_alloc.h>
#include <operating_system/threading/stack_alloc_chunk.h>
#include <operating_system/threading/thread_lock.h>

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
