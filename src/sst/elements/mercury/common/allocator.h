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

#include <common/thread_safe_new.h>
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

#if SSTMAC_CUSTOM_NEW
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
