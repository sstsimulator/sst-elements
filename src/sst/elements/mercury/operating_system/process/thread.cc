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

#include <common/thread_safe_int.h>
#include <common/errors.h>
#include <common/output.h>
#include <components/node.h>
#include <components/operating_system.h>
//#include <sstmac/common/stats/ftq.h>
#include <operating_system/process/thread.h>
#include <operating_system/process/thread_info.h>
#include <operating_system/process/app.h>
//#include <sstmac/software/libraries/library.h>
//#include <sstmac/software/libraries/compute/compute_event.h>
//#include <sstmac/software/api/api.h>
//#include <sstmac/common/sst_event.h>

#include <iostream>
#include <exception>
#include <unistd.h>  // getpagesize()
#include <sys/mman.h>
#include <memory.h>
#include <stdlib.h>
#include <cstdlib>
#include <stdio.h>

//#include <unusedvariablemacro.h>

//MakeDebugSlot(host_compute)

using namespace std;

namespace SST {
namespace Hg {

extern template SST::TimeConverter* HgBase<SST::SubComponent>::time_converter_;

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

Thread::Thread(SST::Params& params, SoftwareId sid, OperatingSystem* os) :
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

  //auto* ftq_stat = os->node()->registerMultiStatistic<int,uint64_t,uint64_t>(params, "ftq", subname);
  //this will either be a null stat or an ftq stat
  //the rest of the code will do null checks on the variable before dumping traces
  //ftq_trace_ = dynamic_cast<FTQCalendar*>(ftq_stat)
}

void
Thread::startAPICall()
{
//  if (host_timer_){
//    double duration = host_timer_->stamp();
//    debug_printf(sprockit::dbg::host_compute,
//                 "host compute for %12.8es", duration);
//    parentApp()->compute(TimeDelta(duration));
//  }
}

void
Thread::endAPICall()
{
//  if (host_timer_){
//    host_timer_->start();
//  }
}

uint32_t
Thread::initId()
{
  //abort("aborting Thread::initID\n");
  //thread id not yet initialized
  if (thread_id_ == Thread::main_thread)
    thread_id_ = THREAD_ID_CNT++;
  //I have not yet been assigned a process context (address space)
  //make my own, for now
  if (p_txt_ == ProcessContext::none)
    p_txt_ = thread_id_;
  return thread_id_;
}

//API*
//Thread::getAppApi(const std::string &name) const
//{
//  return parentApp()->getPrebuiltApi(name);
//}

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

//void
//Thread::appendBacktrace(int  /*id*/)
//{
//#if SSTMAC_HAVE_CALL_GRAPH
//  backtrace_[bt_nfxn_] = id;
//  bt_nfxn_++;
//#else
//  sprockit::abort("did not compile with call graph support");
//#endif
//}

//void
//Thread::popBacktrace()
//{
//  --bt_nfxn_;
//  last_bt_collect_nfxn_ = std::min(last_bt_collect_nfxn_, bt_nfxn_);
//}

//void
//Thread::recordLastBacktrace(int nfxn)
//{
//  last_bt_collect_nfxn_ = nfxn;
//}

void
Thread::spawn(Thread* thr)
{
//  abort("abort Thread::spawn\n");
  thr->parent_app_ = parentApp();
//  if (host_timer_){
//    thr->host_timer_ = new HostTimer;
//    thr->host_timer_->start();
//  }
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

//void
//Thread::spawnOmpParallel()
//{
//  spkt_abort_printf("unimplemented: spawn_omp_parallel");
//  omp_context& active = omp_contexts_.back();
//  active.subthreads.resize(active.requested_num_subthreads);
//  // App* parent = parentApp();
//  // for (int i=1; i < active.requested_num_subthreads; ++i){
//  //   thread* thr = new thread(params, parent->sid(), os_);
//  //   thr->setOmpParentContext(active);
//  //   startThread(thr);
//  //   active.subthreads[i] = thr;
//  // }
//  //and finally have this thread enter the region as thread 0
//  setOmpParentContext(0, active);
//}

//void
//Thread::setOmpParentContext(int id, const omp_context& context)
//{
//  omp_contexts_.emplace_back();
//  omp_context& active = omp_contexts_.back();
//  active.level = context.level + 1;
//  active.num_threads = context.requested_num_subthreads;
//  active.max_num_subthreads = active.requested_num_subthreads =
//      context.max_num_subthreads / context.requested_num_subthreads;
//  active.id = id;
//  active.parent_id = context.id;
//}

//void
//Thread::computeDetailed(uint64_t flops, uint64_t nintops, uint64_t bytes, int nthread)
//{
//  omp_context& active = omp_contexts_.back();
//  int used_nthread = nthread == use_omp_num_threads ? active.num_threads : nthread;
//  parentApp()->computeDetailed(flops, nintops, bytes, used_nthread);
//}

//void
//Thread::collectStats(
//     Timestamp start,
//     TimeDelta elapsed)
//{
//#if !SSTMAC_INTEGRATED_SST_CORE
//#if SSTMAC_HAVE_CALL_GRAPH
//  if (callGraph_) {
//    callGraph_->collect(elapsed.ticks(), this);
//  }
//#endif
//  if (ftq_trace_){
//    ftq_trace_->addData(ftag_.id(), start.time.ticks(), elapsed.ticks());
//  }
//#endif
//}

void
Thread::startThread(Thread* thr)
{
  thr->p_txt_ = p_txt_;
  os_->startThread(thr);
}

//void
//Thread::setCpumask(uint64_t cpumask)
//{
//  cpumask_ = cpumask;
//  os_->reassignCores(this);
//}

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

//#include <sstmac/software/process/std_thread.h>
//#include <sstmac/software/process/std_mutex.h>

//namespace sstmac {
//namespace sw {

//class stdThread : public Thread {
// public:
//  stdThread(std_thread_base* base,
//            Thread* parent) :
//    Thread(parent->parentApp()->params(),
//           SoftwareId(parent->aid(), parent->tid(), -1),
//           parent->os()),
//    base_(base)
//  {
//    parent_app_ = parent->parentApp();
//    //std threads need to be joinable
//    setDetachState(JOINABLE);
//  }

//  void run() override {
//    base_->run();
//  }

// private:
//  std_thread_base* base_;
//};

//int start_std_thread(std_thread_base* base)
//{
//  Thread* parent = OperatingSystem::currentThread();
//  stdThread* thr = new stdThread(base, parent);
//  base->setOwner(thr);
//  parent->os()->startThread(thr);
//  return thr->threadId();
//}

//void join_std_thread(std_thread_base *thr)
//{
//  thr->owner()->join();
//}


//stdMutex::stdMutex()
//{
//  parent_app_ = OperatingSystem::currentThread()->parentApp();
//  id_ = parent_app_->allocateMutex();
//}

//void stdMutex::lock()
//{
//  mutex_t* mut = parent_app_->getMutex(id_);
//  if (mut == nullptr){
//    spkt_abort_printf("error: bad mutex id for std::mutex: %d", id_);
//  } else if (mut->locked) {
//    mut->waiters.push_back(OperatingSystem::currentThread());
//    parent_app_->os()->block();
//  } else {
//    mut->locked = true;
//  }
//}

//stdMutex::~stdMutex()
//{
//  parent_app_->eraseMutex(id_);
//}

//void stdMutex::unlock()
//{
//  mutex_t* mut = parent_app_->getMutex(id_);
//  if (mut == nullptr || !mut->locked){
//    return;
//  } else if (!mut->waiters.empty()){
//    Thread* blocker = mut->waiters.front();
//    mut->waiters.pop_front();
//    parent_app_->os()->unblock(blocker);
//  } else {
//    mut->locked = false;
//  }
//}

//bool stdMutex::try_lock()
//{
//  mutex_t* mut = parent_app_->getMutex(id_);
//  if (mut == nullptr){
//    return false;
//  } else if (mut->locked){
//    return false;
//  } else {
//    return true;
//  }
//}
