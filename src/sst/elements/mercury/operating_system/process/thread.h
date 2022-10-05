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

#include <common/errors.h>
#include <common/node_address.h>
#include <common/timestamp.h>
#include <components/operating_system_fwd.h>
#include <operating_system/process/process_context.h>
#include <operating_system/process/software_id.h>
#include <operating_system/process/app_fwd.h>
#include <operating_system/threading/threading_interface.h>

#include <queue>
#include <map>
#include <utility>
#include <list>
#include <unistd.h>

namespace SST {
namespace Hg {

/**
 * @brief The thread class
 * Encapsulates all the state associated with a simulated thread within SST/macro
 * Not to be confused with thread_context, which just manages the details
 * of context-switching between user space threads.
 */
class Thread
{
 public:
  class kill_exception : public std::exception {};
  class clean_exit_exception: public std::exception {};

  friend class OperatingSystem;
  friend class App;
  friend class DeleteThreadEvent;

  enum detach_t {
    JOINABLE=0,
    DETACHED=1
  };

  /// Help resolve deadlock situations.
  enum state {
    PENDING=0,
    INITIALIZED=1,
    ACTIVE=2,
    SUSPENDED=3,
    BLOCKED=4,
    CANCELED=5,
    DONE=6
  };

  static Thread* current();

//  template <class T> T* getApi(const std::string& name) {
//    API* a = getAppApi(name);
//    T* casted = dynamic_cast<T*>(a);
//    if (!casted) {
//      spkt_abort_printf("Failed to cast API to correct type for %s: got %s",
//                        name.c_str(), typeid(a).name());
//    }
//    return casted;
//  }

  virtual App* parentApp() const {
    return parent_app_;
  }

  static constexpr int no_core_affinity = -1;
  static constexpr int no_socket_affinity = -1;
  static constexpr uint32_t main_thread = -1;
  static constexpr uint32_t nic_thread = -2;
  static constexpr uint32_t rdma_thread = -3;
  static constexpr AppId main_thread_aid = -1;
  static constexpr TaskId main_thread_tid = -1;
  static constexpr int use_omp_num_threads = -1;

 public:
  virtual ~Thread();

  detach_t detachState() const {
    return detach_state_;
  }

  void setDetachState(detach_t d) {
    detach_state_= d;
  }

  state getState() const {
    return state_;
  }

  AppId aid() const {
    return sid_.app_;
  }

  TaskId tid() const {
    return sid_.task_;
  }

  SoftwareId sid() const {
    return sid_;
  }

  ThreadContext* context() const {
    return context_;
  }

  void spawn(Thread* thr);

  uint32_t initId();

  uint32_t threadId() const {
    return thread_id_;
  }

  /**
   * This thread is not currently active - blocked on something
   * However, some kill event happened and I never want to see
   * this thread again.  Make sure the thread doesn't unblock
   * and clean up all resources associated with the thread
   */
  void cancel(){
    state_ = CANCELED;
  }

  bool isCanceled() const {
    return state_ == CANCELED;
  }

  void setPthreadConcurrency(int lvl){
    pthread_concurrency_ = lvl;
  }

  int pthreadConcurrency() const {
    return pthread_concurrency_;
  }

  /**
   * This can get called by anyone to have a thread exit, including during normal app termination
   * This must be called while running on this thread's context, NOT the DES thread or any other thread
   */
  void kill(int code = 1) {
    if (code == 0){
      throw clean_exit_exception();
    } else {
      throw kill_exception();
    }
  }

  OperatingSystem* os() const {
    return os_;
  }

//  void collectStats(Timestamp start, TimeDelta elapsed);

//  const int* backtrace() const {
//    return backtrace_;
//  }

  virtual bool isMainThread() const {
    return false;
  }

  int lastBacktraceNumFxn() const {
    return last_bt_collect_nfxn_;
  }

  int backtraceNumFxn() const {
    return bt_nfxn_;
  }

  bool timedOut() const {
    return timed_out_;
  }

  void setTimedOut(bool flag){
    timed_out_ = flag;
  }

  uint64_t blockCounter() const {
    return block_counter_;
  }

  void incrementBlockCounter() {
    ++block_counter_;
  }

//  void appendBacktrace(int fxnId);

//  void popBacktrace();

//  void recordLastBacktrace(int nfxn);

  void initThread(const SST::Params& params, int phyiscal_thread_id,
    ThreadContext* tocopy, void *stack, int stacksize,
    void* globals_storage, void* tls_storage);

  virtual void run() = 0;

  /** A convenience request to start a new thread.
  *  The current thread has to be initialized for this to work.
  */
  void startThread(Thread* thr);

  void setThreadId(int thr);

  void join();

  ProcessContext getProcessContext() const {
    return p_txt_;
  }

  bool isInitialized() const {
    return state_ >= INITIALIZED;
  }

  void setAffinity(int core){
    zeroAffinity();
    addAffinity(core);
  }
  
  void addAffinity(int core){
    cpumask_ = cpumask_ | (1<<core);
  }
  
  void zeroAffinity(){
    cpumask_ = 0;
  }

  void setCpumask(uint64_t cpumask);
  
  uint64_t cpumask() const {
    return cpumask_;
  }
  
  uint64_t activeCoreMask() const {
    return active_core_mask_;
  }

  static inline void addCore(int core, uint64_t& mask){
    mask = mask | (1<<core);
  }

  static inline void removeCore(int core, uint64_t& mask){
    mask = mask & ~(1<<core);
  }

  void addActiveCore(int core){
    active_cores_.push_back(core);
    addCore(core, active_core_mask_);
  }

  int popActiveCore() {
    int core = active_cores_.back();
    active_cores_.pop_back();
    removeCore(core, active_core_mask_);
    return core;
  }

  int numActiveCcores() const {
    return active_cores_.size();
  }

  void computeDetailed(uint64_t flops, uint64_t intops,
                        uint64_t bytes, int nthread=use_omp_num_threads);

//  int ompGetThreadNum() const {
//    auto& active = omp_contexts_.back();
//    return active.id;
//  }

//  int ompGetNumThreads() const {
//    auto& active = omp_contexts_.back();
//    return active.num_threads;
//  }

//  int ompGetMaxThreads() const {
//    auto& active = omp_contexts_.back();
//    return active.max_num_subthreads;
//  }

//  int ompGetAncestorThreadNum() const {
//    auto& active = omp_contexts_.back();
//    return active.parent_id;
//  }

//  void ompSetNumThreads(int thr) {
//    auto& active = omp_contexts_.back();
//    active.requested_num_subthreads = thr;
//  }

//  int ompGetLevel() const {
//    auto& active = omp_contexts_.back();
//    return active.level;
//  }

//  int ompInParallel() {
//    auto& active = omp_contexts_.back();
//    bool parallel = active.level > 0;
//    return parallel ? 1 : 0;
//  }

//  template <class T> static T* getCurrentApi(){
//    return current()->getApi<T>();
//  }

  void* getTlsValue(long thekey) const;

  void setTlsValue(long thekey, void* ptr);

  Timestamp now();

  void startAPICall();

  void endAPICall();

//  void setTag(const FTQTag& t){
//    ftag_ = t;
//  }

//  const FTQTag& tag() const {
//    return ftag_;
//  }

//  void spawnOmpParallel();

//  CallGraph* callGraph() const {
//    return callGraph_;
//  }

 protected:
  Thread(SST::Params& params,
         SoftwareId sid, OperatingSystem* os);

 private:
  struct omp_context {
    omp_context* parent;
    int level;
    int id;
    int parent_id;
    int num_threads;
    int requested_num_subthreads;
    int max_num_subthreads;
    std::vector<Thread*> subthreads;
    omp_context() :
      parent(nullptr), id(0), parent_id(-1),
      num_threads(1), max_num_subthreads(1)
    {}
  };

  /// Run routine that defines the initial context for this task.
  /// This routine calls the virtual thread::run method.
  static void runRoutine(void* threadptr);

//  void setOmpParentContext(int id, const omp_context& parent);

  /**
   * This should only ever be invoked by the delete thread event.
   * This ensures that the thread is completely done being operated on
   * It is now safe to free all resources (thread-local vars, etc)
   */
  virtual void cleanup();

 protected:
  state state_;

  OperatingSystem* os_;

  std::queue<Thread*> joiners_;

  App* parent_app_; // who created this one. null if launch/os.

  ProcessContext p_txt_;

//  FTQTag ftag_;

  SoftwareId sid_;

//  HostTimer* host_timer_;

 private:
//  API* getAppApi(const std::string& name) const;

//  CallGraphTrace backtrace_; //each function is labeled by unique integer

  int last_bt_collect_nfxn_;

  int bt_nfxn_;

  bool timed_out_;

  std::map<int, void*> tls_values_;

  std::vector<int> active_cores_;

  void* stack_;

  char* tls_storage_;
  
  uint32_t thread_id_;

  ThreadContext* context_;
  
  uint64_t cpumask_;
  
  uint64_t active_core_mask_;

  uint64_t block_counter_;

  int pthread_concurrency_;

  detach_t detach_state_;

  std::list<omp_context> omp_contexts_;

//  CallGraph* callGraph_;

//  FTQCalendar* ftq_trace_;

};

} // end namespace Hg
} // end namespace SST
