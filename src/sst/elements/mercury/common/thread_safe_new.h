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

#pragma once

#include <sst_element_config.h>
#include <vector>
#include <set>

#define SSTHG_TLS_OFFSET 64

#define SSTHG_NEW_SUPER_DEBUG 0

namespace SST {
namespace Hg {

template <class T>
T& thread_stack_size(){
  static T stacksize = 0;
  return stacksize;
}

static int inline currentThreadId() {
  int stacksize = thread_stack_size<int>();
  if (stacksize == 0){
    return 0;
  } else {
    char x;
    intptr_t stackptr = (intptr_t) &x;
    intptr_t stack_mult = stackptr / stacksize;
    char* aligned_stack_ptr = (char*) (stack_mult * stacksize);
    int* thrPtr = (int*) (aligned_stack_ptr + SSTHG_TLS_OFFSET);
    return *thrPtr;
  }
}

struct ThreadAllocatorSet {
#define MAX_NUM_NEW_SAFE_THREADS 128
  std::vector<char*> allocations[MAX_NUM_NEW_SAFE_THREADS];
  std::vector<void*> available[MAX_NUM_NEW_SAFE_THREADS];
  ~ThreadAllocatorSet(){
    for (int i=0; i < MAX_NUM_NEW_SAFE_THREADS; ++i){
      auto& vec = allocations[i];
      for (char* ptr : vec){
        delete[] ptr;
      }
    }
  }
};

template <class T>
class thread_safe_new {

 public:
#if SSTMAC_ENABLE_SANITY_CHECK
  static constexpr uint32_t magic_number = std::numeric_limits<uint32_t>::max();
#endif

#if SSTMAC_CUSTOM_NEW
  static void freeAtEnd(T*){
    //do nothing - the allocation is getting cleaned up
  }

  template <class... Args>
  static T* allocateAtBeginning(Args&&... args){
    void* ptr = allocate(0);
    return new (ptr) T(std::forward<Args>(args)...);
  }

  static void* allocate(int thread){
    if (alloc_.available[thread].empty()){
      grow(thread);
    }
    void* ret = alloc_.available[thread].back();
#if SSTMAC_ENABLE_SANITY_CHECK
    (uint32_t*) casted = (uint32_t*) ret;
    *casted = 0;
#endif
    alloc_.available[thread].pop_back();
#if SPKT_NEW_SUPER_DEBUG
    all_chunks_.erase(ret);
#endif
    return ret;
  }

  static void* operator new(size_t sz){
    if (sz != sizeof(T)){
      spkt_abort_printf("allocating mismatched sizes: %d != %d",
                        sz, sizeof(T));
    }
    int thread = currentThreadId();
    return allocate(thread);
  }

  static void* operator new(size_t  /*sz*/, void* ptr){
    return ptr;
  }

  static void operator delete(void* ptr){
    int thread = currentThreadId();
    alloc_.available[thread].push_back(ptr);
#if SSTMAC_ENABLE_SANITY_CHECK
    (uint32_t*) casted = (uint32_t*) ptr;
    if (*casted == magic_number){
      spkt_abort_printf("chunk %p already freed!", ptr);
    }
    *casted = magic_number;
#endif
#if SPKT_NEW_SUPER_DEBUG
    auto iter = all_chunks_.find(ptr);
    if (iter != all_chunks_.end()){
      spkt_abort_printf("freeing chunk twice-in-a-row: %p", ptr);
    }
    all_chunks_.insert(ptr);
#endif
  }

#define SSTMAC_CACHE_ALIGNMENT 64
  static void grow(int thread){
    size_t unitSize = sizeof(T);
    if (unitSize % SSTMAC_CACHE_ALIGNMENT != 0){
      size_t rem = SSTMAC_CACHE_ALIGNMENT - unitSize % SSTMAC_CACHE_ALIGNMENT;
      unitSize += rem;
    }

    char* newTs = new char[unitSize*increment];
    char* ptr = newTs;
    int numElems = increment;
    if (uintptr_t(ptr) % SSTMAC_CACHE_ALIGNMENT){
      size_t rem = SSTMAC_CACHE_ALIGNMENT - (uintptr_t(ptr) % SSTMAC_CACHE_ALIGNMENT);
      ptr += rem;
      numElems -= 1;
    }
    for (int i=0; i < numElems; ++i, ptr += unitSize){
      alloc_.available[thread].push_back(ptr);
#if SPKT_NEW_SUPER_DEBUG
      all_chunks_.insert(ptr);
#endif
    }
    alloc_.allocations[thread].push_back(newTs);
  }

 private:
  static ThreadAllocatorSet alloc_;
  static int constexpr increment = 512;
#if SPKT_NEW_SUPER_DEBUG
  static std::set<void*> all_chunks_;
#endif

#else
  //no custom new operators
  template <class... Args>
  static T* allocateAtBeginning(Args&&... args){
    return new T(std::forward<Args>(args)...);
  }

  static void freeAtEnd(T* ptr){
    delete ptr;
  }
#endif
};

#if SSTMAC_CUSTOM_NEW
template <class T> ThreadAllocatorSet thread_safe_new<T>::alloc_;

#if SPKT_NEW_SUPER_DEBUG
template <class T> std::set<void*> thread_safe_new<T>::all_chunks_;
#endif

#endif

} // end namespace Hg
} // end namespace SST
