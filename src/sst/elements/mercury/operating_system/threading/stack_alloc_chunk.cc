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

#include <common/errors.h>
#include <common/output.h>
#include <operating_system/threading/stack_alloc_chunk.h>

#include <sys/mman.h>
#include <unistd.h>
#include <iostream>
#include <stdio.h>
#include <cstring>
#include <errno.h>

namespace SST {
namespace Hg {
//
// Make a new chunk.
//
StackAlloc::chunk::chunk(size_t stacksize, size_t suggested_chunk_size, bool protect) :
  addr_(nullptr),
  protect_(protect),
  size_((protect_) ? 2 * suggested_chunk_size : suggested_chunk_size),
  stacksize_(stacksize),
  step_size_((protect_) ? 2 * stacksize_ : stacksize_)
{
  // Now allocate our chunk.
  int mmap_flags = MAP_PRIVATE | MAP_ANON;
  addr_ = (char*)mmap(0, size_, PROT_READ | PROT_WRITE,
                      mmap_flags, -1, 0);
  if(addr_ == MAP_FAILED) {
    cerrn << "Failed to mmap a region of size " << size_ << ": "
              << strerror(errno) << "\n";
    SST::Hg::abort("stackalloc::chunk: failed to mmap region.");
  }

  //make sure we are aligned on boundaries of size stack_size
  size_t stack_mod = ((size_t)addr_) % stacksize_;
  if (stack_mod != 0){ //this aligns us on boundaries
    next_stack_offset_ = stacksize_ - stack_mod;
  }

  // If protected we need to mprotect the first stack and advance our offset to
  // the end of the second stack
  if(protect_){
    // Start protection on the first stack
    auto protected_offset_ = next_stack_offset_;
    next_stack_offset_ += stacksize_; // Second stack is first usable stack

    while(protected_offset_ + step_size_ < size_){
      void *protected_address = addr_ + protected_offset_;
      mprotect(protected_address, stacksize_, PROT_NONE);
      protected_offset_ += step_size_;
    }
  }
}

void* 
StackAlloc::chunk::getNextStack() {
  if(next_stack_offset_ + step_size_ >= size_) {
    return nullptr;
  } 

  void* rv = addr_ + next_stack_offset_;
  next_stack_offset_ += step_size_;
  return rv;
}

StackAlloc::chunk::~chunk()
{
  if(addr_) {
    munmap(addr_, size_);
  }
}

} // end of namespace Hg
} // end of namespace SST
