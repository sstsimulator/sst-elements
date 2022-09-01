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
