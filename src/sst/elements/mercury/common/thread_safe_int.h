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

#pragma once

#include <cstddef>
#include <mercury/operating_system/threading/thread_lock.h>

#include <cstdint>
#include <cstring>

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
    lock();
    Integer tmp(value_);
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
