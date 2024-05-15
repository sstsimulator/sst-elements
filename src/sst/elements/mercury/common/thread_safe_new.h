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

#pragma once

//#include <sst_element_config.h>
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
#if SST_HG_ENABLE_SANITY_CHECK
  static constexpr uint32_t magic_number = std::numeric_limits<uint32_t>::max();
#endif

#if SST_HG_CUSTOM_NEW
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
#if SST_HG_ENABLE_SANITY_CHECK
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
#if SST_HG_ENABLE_SANITY_CHECK
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

#define SST_HG_CACHE_ALIGNMENT 64
  static void grow(int thread){
    size_t unitSize = sizeof(T);
    if (unitSize % SST_HG_CACHE_ALIGNMENT != 0){
      size_t rem = SST_HG_CACHE_ALIGNMENT - unitSize % SST_HG_CACHE_ALIGNMENT;
      unitSize += rem;
    }

    char* newTs = new char[unitSize*increment];
    char* ptr = newTs;
    int numElems = increment;
    if (uintptr_t(ptr) % SST_HG_CACHE_ALIGNMENT){
      size_t rem = SST_HG_CACHE_ALIGNMENT - (uintptr_t(ptr) % SST_HG_CACHE_ALIGNMENT);
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

#if SST_HG_CUSTOM_NEW
template <class T> ThreadAllocatorSet thread_safe_new<T>::alloc_;

#if SPKT_NEW_SUPER_DEBUG
template <class T> std::set<void*> thread_safe_new<T>::all_chunks_;
#endif

#endif

} // end namespace Hg
} // end namespace SST
