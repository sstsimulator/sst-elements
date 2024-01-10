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

#include <mercury/common/errors.h>
#include <mercury/common/output.h>
#include <mercury/operating_system/threading/stack_alloc_chunk.h>

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
