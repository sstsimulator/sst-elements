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

#include <mercury/common/thread_safe_int.h>
#include <mercury/common/errors.h>
#include <mercury/common/output.h>
#include <mercury/components/node.h>
#include <mercury/components/operating_system.h>
#include <mercury/operating_system/process/thread.h>
#include <mercury/operating_system/process/thread_info.h>
#include <mercury/operating_system/process/app.h>

#include <iostream>
#include <exception>
#include <unistd.h>  // getpagesize()
#include <sys/mman.h>
#include <memory.h>
#include <stdlib.h>
#include <cstdlib>
#include <stdio.h>

using namespace std;

namespace SST {
namespace Hg {

extern template class  HgBase<SST::Component>;
extern template class  HgBase<SST::SubComponent>;
extern template SST::TimeConverter HgBase<SST::SubComponent>::time_converter_;

static thread_safe_u32 THREAD_ID_CNT(0);

//
// Private method that gets called by the scheduler.
//
void
Thread::initThread(const SST::Params&  /*params*/,
  int physical_thread_id, ThreadContext* des_thread, void *stack,
  int stacksize, void* globals_storage, void* tls_storage)
{
  ThreadInfo::registerUserSpaceVirtualThread(physical_thread_id, stack,
                                             globals_storage, tls_storage,
                                             isMainThread(), true);
  stack_ = stack;

  initId();

  state_ = INITIALIZED;

  context_ = des_thread->copy();

  tls_storage_ = (char*) tls_storage;

  context_->startContext(stack, stacksize,
                          runRoutine, this,
                          des_thread);
}

Thread*
Thread::current()
{
  return OperatingSystem::currentThread();
}

void
Thread::cleanup()
{
  if (parent_app_){
    if (detach_state_ == DETACHED && state_ != CANCELED){
      parent_app_->removeSubthread(this);
      os_->scheduleThreadDeletion(this);
    } else{} //parent will join and then delete this
  } else {
    //no matter what, I have to delete myself
    os_->scheduleThreadDeletion(this);
  }
  // We are done, ask the scheduler to remove this task from the
  state_ = DONE;

  os_->completeActiveThread();
}

//
// Run routine that defines the initial context for this task.
//
void
Thread::runRoutine(void* threadptr)
{
//  abort("aborting Thread::runRoutine\n");
  Thread* self = (Thread*) threadptr;
  // Go.
  if (self->isInitialized()) {
    self->state_ = ACTIVE;
    try {
      {
        //need to scope it here to force destructor of guard
        //because of the way context switching works I might
        //never leave this try block and closing the guard
        //sstmac::sw::OperatingSystem::CoreAllocateGuard guard(self->os(), self);
        self->run();
      }
      //this doesn't so much kill the thread as context switch it out
      //it is up to the above delete thread event to actually to do deletion/cleanup
      //all of this is happening ON THE THREAD - it kills itself
      //this is not the DES thread killing it
      self->cleanup();
    } catch (const kill_exception& ex) {
      //great, we are done
    } catch (const clean_exit_exception& ex) {
      self->cleanup();
    } catch (const std::exception &ex) {
      cerrn << "thread terminated with exception: " << ex.what()
                << "\n";
      // should forward the exception to the main thread,
      // but for now will abort
      cerrn << "aborting" << std::endl;
      std::cout.flush();
      std::cerr.flush();
      abort("");
    } catch (const std::string& str) {
      cerrn << "thread terminated with string exception: " << str << "\n";
      cerrn << "aborting" << std::endl;
      std::cout.flush();
      std::cerr.flush();
      abort("");
    }
  } else {
    abort("thread::run_routine: task has not been initialized");
  }
}

Thread::Thread(SST::Params& params, SoftwareId sid, OperatingSystemAPI* os) :
  state_(PENDING),
  os_(os),
  parent_app_(nullptr),
  p_txt_(ProcessContext::none),
  sid_(sid),
  last_bt_collect_nfxn_(0),
  bt_nfxn_(0),
  timed_out_(false),
  tls_storage_(nullptr),
  thread_id_(Thread::main_thread),
  context_(nullptr),
  cpumask_(0),
  active_core_mask_(0),
  block_counter_(0),
  pthread_concurrency_(0),
  detach_state_(DETACHED)
{
  //make all cores possible active
  cpumask_ = ~(cpumask_);
}

void
Thread::startLibraryCall()
{
//  if (host_timer_){
//    double duration = host_timer_->stamp();
//    debug_printf(sprockit::dbg::host_compute,
//                 "host compute for %12.8es", duration);
//    parentApp()->compute(TimeDelta(duration));
//  }
}

void
Thread::endLibraryCall()
{
//  if (host_timer_){
//    host_timer_->start();
//  }
}

uint32_t
Thread::initId()
{
  if (thread_id_ == Thread::main_thread)
    thread_id_ = THREAD_ID_CNT++;
  //I have not yet been assigned a process context (address space)
  //make my own, for now
  if (p_txt_ == ProcessContext::none)
    p_txt_ = thread_id_;
  return thread_id_;
}

Library*
Thread::getAppLibrary(const std::string &name) const
{
  return parentApp()->getLibrary(name);
}

void*
Thread::getTlsValue(long thekey) const
{
  auto it = tls_values_.find(thekey);
  if (it == tls_values_.end())
    return nullptr;
  return it->second;
}

void
Thread::setTlsValue(long thekey, void *ptr)
{
  tls_values_[thekey] = ptr;
}

void
Thread::spawn(Thread* thr)
{
  thr->parent_app_ = parentApp();
  os_->startThread(thr);
}

Timestamp
Thread::now()
{
  return os_->now();
}

Thread::~Thread()
{
  active_cores_.clear();
 // if (stack_) StackAlloc::free(stack_);
  if (context_) {
    context_->destroyContext();
    delete context_;
  }
  if (tls_storage_) delete[] tls_storage_;
  //if (host_timer_) delete host_timer_;
}

void
Thread::startThread(Thread* thr)
{
  thr->p_txt_ = p_txt_;
  os_->startThread(thr);
}

void
Thread::join()
{
  //JJW 03/08/2018 It can now happen with std::thread wrappers
  //that you trying joining a thread before it has even initialized
  //I don't think this is actually an error
  //if (!this->isInitialized()) {
    // We can't context switch the caller out without first being initialized
  //  spkt_throw_printf(sprockit::illformed_error,
  //                   "thread::join: target thread has not been initialized.");
  //}
  os_->joinThread(this);
}

} // end namespace Hg
} // end namespace SST
