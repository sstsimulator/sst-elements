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

#include <sst_element_config.h>
#include <thread>

#ifdef SSTMAC_PTHREAD_MACRO_H
#error sstmacro thread_lock.h should never be used with pthread.h replacement headers
#endif

namespace SST {
namespace Hg {

class MutexThreadLock
{

 public:
  MutexThreadLock();

  ~MutexThreadLock();

  void lock();

  void unlock();

  bool trylock();

  bool locked() const {
    return locked_;
  }

 private:
  std::mutex mutex_;
  bool locked_;
};

#if USE_SPINLOCK
class SpinThreadLock
{
 public:
  SpinThreadLock();

  ~SpinThreadLock();

  void lock();

  void unlock();

  bool trylock();

  bool locked() const {
    return locked_;
  }

 private:
  pthread_spinlock_t lock_;

  bool locked_;
};
#endif

#if USE_SPINLOCK
typedef SpinThreadLock thread_lock;
#else
typedef MutexThreadLock thread_lock;
#endif

class Lockable {
 public:
  void lock(){
    lock_.lock();
  }

  void unlock(){
    lock_.unlock();
  }

 private:
  thread_lock lock_;
};

} // end namespace Hg
} // end namespace SST
