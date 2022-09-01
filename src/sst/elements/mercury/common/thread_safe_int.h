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

#include <operating_system/threading/thread_lock.h>

#include <cstring>
#include <cstdint>

namespace SST {
namespace Hg {

class thread_safe_int_base :
  public Lockable
{
};

template <class Integer>
class thread_safe_int_t :
  public thread_safe_int_base
{

 public:
  template <class Y>
  thread_safe_int_t(const Y& y) :
   value_(y)
  {
  }

  template <class Y>
  Integer& operator=(const Y& y){
    value_ = y;
    return value_;
  }

  Integer operator++(){
    lock();
    ++value_;
    Integer tmp = value_;
    unlock();
    return tmp;
  }

  Integer operator++(int  /*i*/){
    Integer tmp(value_);
    lock();
    value_++;
    unlock();
    return tmp;
  }

  operator Integer(){
    return value_;
  }

 protected:
  Integer value_;

};

typedef thread_safe_int_t<int> thread_safe_int;
typedef thread_safe_int_t<long> thread_safe_long;
typedef thread_safe_int_t<uint32_t> thread_safe_u32;
typedef thread_safe_int_t<uint64_t> thread_safe_u64;
typedef thread_safe_int_t<long long> thread_safe_long_long;
typedef thread_safe_int_t<size_t> thread_safe_size_t;

} // end namespace Hg
} // end namespace SST
