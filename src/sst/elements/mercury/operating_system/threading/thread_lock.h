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

#include <sst_hg_config.h>

#include <pthread.h>

#ifdef SST_HG_PTHREAD_MACRO_H
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
  pthread_mutex_t mutex_;

  bool locked_;

};

typedef MutexThreadLock thread_lock;

class Lockable {
 public:
  void lock(){
//#if SST_HG_USE_MULTITHREAD
    lock_.lock();
//#endif
  }

  void unlock(){
//#if SST_HG_USE_MULTITHREAD
    lock_.unlock();
//#endif
  }

 private:
//#if SST_HG_USE_MULTITHREAD
  thread_lock lock_;
//#endif
};

} // end of namespace Hg
} // end of namespace SST
