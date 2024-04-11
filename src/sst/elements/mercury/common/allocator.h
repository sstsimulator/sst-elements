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

#pragma once

#include <mercury/common/thread_safe_new.h>
#include <vector>
#include <cstdint>
#include <cstddef>
#include <iostream>

namespace SST {
namespace Hg {

template <class T>
class allocator
{
 public:
  typedef size_t    size_type;
  typedef ptrdiff_t difference_type;
  typedef T*        pointer;
  typedef const T*  const_pointer;
  typedef T&        reference;
  typedef const T&  const_reference;
  typedef T         value_type;

  template <class U>
  struct rebind { typedef allocator<U> other; };


  size_t  unit_size;
  std::vector<T*> storage;
  std::vector<char*> allocations;

  allocator() {
    init();
  }

  allocator(const allocator&) {
    init();
  }
  
  ~allocator(){
    for (char* ptr : allocations){
      delete[] ptr;
    }
  }

  void init(){
    size_t rem = sizeof(T) % 32;
    if (rem){
      unit_size = sizeof(T) + 32 - rem;
    } else {
      unit_size = sizeof(T);
    }
  }

  void destroy(pointer p) { p->~T(); }

  pointer allocate(size_type n, const void * = 0) {
    //ignore hints
    static int constexpr increment = 1024;
    if (n > 1){
      ::abort();
      return (T*) new char[unit_size*n];
    } else {
      if (storage.empty()){
        char* ptr = new char[unit_size*increment];
        allocations.push_back(ptr);
        storage.resize(increment);
        for (int i=0; i < increment; ++i, ptr += unit_size)
          storage[i] = (T*) ptr;
      }
      T* ret = storage.back();
      storage.pop_back();
      return ret;
    }
  }

  void deallocate(void* p, size_type n) {
    if (n > 1){
      char* arr = (char*) p;
      delete[] arr;
    } else {
      storage.push_back((T*)p);
    }
  }

};

#if SST_HG_CUSTOM_NEW
template <class T>
class threadSafeAllocator
{
 public:
  typedef size_t    size_type;
  typedef ptrdiff_t difference_type;
  typedef T*        pointer;
  typedef const T*  const_pointer;
  typedef T&        reference;
  typedef const T&  const_reference;
  typedef T         value_type;

  template <class U>
  struct rebind { typedef threadSafeAllocator<U> other; };

  void destroy(pointer p) { p->~T(); }

  template <class U, class... Args>
  void construct(U* p, Args&&... args){ ::new ((void*)p) U(std::forward<Args>(args)...); }

  pointer allocate(size_type n, const void * = 0) {
    //ignore hints
    if (n > 1){
      std::cerr << "thread safe allocator cannot allocate more than 1 item at a time" << std::endl;
      ::abort();
    } else {
      return (pointer) thread_safe_new<T>::operator new(sizeof(T));
    }
  }

  void deallocate(void* p, size_type n) {
    if (n > 1){
      std::cerr << "thread safe allocator cannot allocate more than 1 item at a time" << std::endl;
      ::abort();
    } 
    thread_safe_new<T>::operator delete(p);
  }

};
#else
template <class T> using threadSafeAllocator = std::allocator<T>;
#endif

} // end namespace Hg
} // end namespace SST
