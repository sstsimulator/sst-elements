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

#include <common/errors.h>
#include <operating_system/threading/thread_lock.h>
#include <operating_system/threading/sim_thread_lock.h>

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

MutexThreadLock::MutexThreadLock()
{
  int signal = pthread_mutex_init(&mutex_, NULL);
  if (signal != 0) {
    sst_hg_throw_printf(SST::Hg::HgError,
        "mutex init error: %d", signal);
  }
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
  int signal = pthread_mutex_lock(&mutex_);
  if (signal != 0) {
    sst_hg_throw_printf(SST::Hg::HgError,
        "pthread_lock::lock: mutex lock error %d:%s",
        signal, ::strerror(signal));
  }
  locked_ = true;
}

bool
MutexThreadLock::trylock()
{
  int signal = pthread_mutex_trylock(&mutex_);
  bool locked = signal == 0;
  if (locked) {
    locked_ = true;
  }
  return locked;
}

void
MutexThreadLock::unlock()
{
  locked_ = false;
  int signal = pthread_mutex_unlock(&mutex_);
  if (signal != 0) {
    sst_hg_abort_printf("pthread_lock::unlock: unlocking mutex that I don't own: %d:%s",
        signal, ::strerror(signal));
  }
}

#if SSTMAC_USE_SPINLOCK
SpinThreadLock::SpinThreadLock()
{
  int signal = pthread_spin_init(&lock_, PTHREAD_PROCESS_PRIVATE);
  if (signal != 0) {
    sst_hg_abort_printf("mutex init error: %d", signal);
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
    sst_hg_abort_printf("pthread_lock::lock: mutex lock error %d:%s",
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
    sst_hg_abort_printf("pthread_lock::unlock: unlocking mutex that I don't own: %d:%s",
        signal, ::strerror(signal));
  }
}
#endif

} // end of namespace Hg
} // end of namespace SST
