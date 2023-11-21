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

#include <mercury/common/thread_lock.h>
#include <mercury/common/sim_thread_lock.h>
#include <mercury/common/errors.h>
#include <string.h>

namespace SST {
namespace Hg {

class mutex_sim_thread_lock :
  public sim_thread_lock,
  public MutexThreadLock
{
  void lock() override{
    MutexThreadLock::lock();
  }

  void unlock() override{
    MutexThreadLock::unlock();
  }
};

sim_thread_lock*
sim_thread_lock::construct()
{
  return new mutex_sim_thread_lock;
}

MutexThreadLock::~MutexThreadLock()
{
  /** Ignore the signal for now since whatever person wrote
    some of the pthread implementations doesn't know how turn off
    all of the locks. This often erroneously returns signal 16 EBUSY */
  /*int signal = */ pthread_mutex_destroy(&mutex_);
}

void
MutexThreadLock::lock()
{
  mutex_.lock();
  locked_ = true;
}

bool
MutexThreadLock::trylock()
{
  bool locked = mutex_.try_lock();
  if (locked) {
    locked_ = true;
  }
  return locked;
}

void
MutexThreadLock::unlock()
{
  locked_ = false;
  mutex_.unlock();
}

#if USE_SPINLOCK
SpinThreadLock::SpinThreadLock()
{
  int signal = pthread_spin_init(&lock_, PTHREAD_PROCESS_PRIVATE);
  if (signal != 0) {
    spkt_abort_printf("mutex init error: %d", signal);
  }
}

SpinThreadLock::~SpinThreadLock()
{
  /** Ignore the signal for now since whatever person wrote
    some of the pthread implementations doesn't know how turn off
    all of the locks. This often erroneously returns signal 16 EBUSY */
  int signal = pthread_spin_destroy(&lock_);
}

void
SpinThreadLock::lock()
{
  int signal = pthread_spin_lock(&lock_);
  if (signal != 0) {
    spkt_abort_printf("pthread_lock::lock: mutex lock error %d:%s",
        signal, ::strerror(signal));
  }
  locked_ = true;
}

bool
SpinThreadLock::trylock()
{
  int signal = pthread_spin_trylock(&lock_);
  bool locked = signal == 0;
  if (locked) {
    locked_ = true;
  }
  return locked;
}

void
SpinThreadLock::unlock()
{
  locked_ = false;
  int signal = pthread_spin_unlock(&lock_);
  if (signal != 0) {
    spkt_abort_printf("pthread_lock::unlock: unlocking mutex that I don't own: %d:%s",
        signal, ::strerror(signal));
  }
}
#endif

} // end namespace Hg
} // end namespace SST
