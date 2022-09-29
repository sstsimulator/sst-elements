// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#pragma once

#include <common/component.h>

#include <sst/core/link.h>

#include <common/factory.h>
#include <components/node_fwd.h>
#include <operating_system/threading/threading_interface.h>
#include <operating_system/launch/app_launcher_fwd.h>
#include <operating_system/launch/app_launch_request.h>
#include <operating_system/process/app.h>
#include <operating_system/process/thread.h>
#include <operating_system/process/mutex.h>
#include <operating_system/process/tls.h>
#include <operating_system/process/compute_scheduler.h>
#include <operating_system/libraries/library.h>

#include <cstdint>
#include <memory>

namespace SST {
namespace Hg {

extern template SST::TimeConverter* HgBase<SST::SubComponent>::time_converter_;

//static std::string _tick_spacing_string_("1ps");

class OperatingSystem : public SST::Hg::SubComponent {

public:

  SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
    OperatingSystem,
    "hg",
    "operating_system",
    SST_ELI_ELEMENT_VERSION(0,0,1),
    "Mercury Operating System",
    SST::Hg::OperatingSystem
  )

  SST_ELI_REGISTER_SUBCOMPONENT_API(SST::Hg::OperatingSystem,
                                    SST::Hg::Node*)

  OperatingSystem(SST::ComponentId_t id, SST::Params& params, Node* parent);

  virtual ~OperatingSystem();

  void setup() override;

  void handleEvent(SST::Event *ev);

  bool clockTic(SST::Cycle_t) {
    //noop
    return true;
  }

  static size_t stacksize(){
    return sst_hg_global_stacksize;
  }

  /**
   * @brief block Block the currently running thread context.
   * This must be called from an application thread, NOT the DES thread
   * @param req [in-out] A key that will store blocking data that will be needed
   *                     for later unblocking
   * @return
   */
  void block();

  void startApp(App* theapp, const std::string&  /*unique_name*/);
  void startThread(Thread* t);
  void joinThread(Thread* t);
  void scheduleThreadDeletion(Thread* thr);
  void switchToThread(Thread* tothread);
  void unblock(Thread* thr);
  void completeActiveThread();

  static Thread* currentThread(){
    return staticOsThreadContext()->activeThread();
  }

  static inline OperatingSystem*& staticOsThreadContext(){
  #if SSTMAC_USE_MULTITHREAD
    int thr = ThreadInfo::currentPhysicalThreadId();
    return active_os_[thr];
  #else
    return active_os_;
  #endif
  }

  inline OperatingSystem*& activeOs() {
#if SSTMAC_USE_MULTITHREAD
  return active_os_[threadId()];
#else
  return active_os_;
#endif
  }

  Thread* activeThread() const {
    return active_thread_;
  }

  static OperatingSystem* currentOs(){
    return staticOsThreadContext();
  }

 private:

  void initThreading(SST::Params& params);

  const std::string& tickIntervalString()
  {
    return _tick_spacing_string_;
  }

  void addLaunchRequests(SST::Params& params);

  /// The caller context (main DES thread).  We go back
  /// to this context on every context switch.
  ThreadContext *des_context_;

  Node* node_;
  Thread* active_thread_;
  Thread* blocked_thread_;
  int next_condition_;
  int next_mutex_;
  std::map<int, condition_t> conditions_;
  std::map<int, mutex_t> mutexes_;
  SST::Params params_;
  Link* selfEventLink_;
  std::list<AppLaunchRequest*> requests_;
  std::unique_ptr<SST::Output> out_;
  AppLauncher* app_launcher_;
  std::map<uint32_t, Thread*> running_threads_;
  ComputeScheduler* compute_sched_;

  std::unordered_map<std::string, Library*> libs_;
  std::unordered_map<Library*, int> lib_refcounts_;
  std::map<std::string, std::list<Request*>> pending_library_request_;

  NodeId my_addr_;

//  int next_condition_;
//  int next_mutex_;
//  std::map<int, condition_t> conditions_;
//  std::map<int, mutex_t> mutexes_;

#if SSTMAC_USE_MULTITHREAD
  static std::vector<OperatingSystem*> active_os_;
#else
  static OperatingSystem* active_os_;
#endif

 public:
  static SST::TimeConverter* timeConverter() {
    return time_converter_;
  }
// protected:
//  static SST::TimeConverter* time_converter_;

 public:

  Node* node() const {
    return node_;
  }

  Thread* getThread(uint32_t threadId) const;

  NodeId addr() const {
    return my_addr_;
  }

  /**
   * Allocate a unique ID for a mutex variable
   * @return The unique ID
   */
  int allocateMutex();

  /**
   * Fetch a mutex object corresponding to their ID
   * @param id
   * @return The mutex object corresponding to the ID. Return NULL if no mutex is found.
   */
  mutex_t* getMutex(int id);

  bool eraseMutex(int id);

  int allocateCondition();

  bool eraseCondition(int id);

  condition_t* getCondition(int id);

  int ncores() const {
    return compute_sched_->ncores();
  }

  int nsockets() const {
    return compute_sched_->nsockets();
  }

  void reserveCores(int ncore, Thread* thr) {
    compute_sched_->reserveCores(ncore,thr);
  }

  void releaseCores(int ncore, Thread* thr) {
    compute_sched_->releaseCores(ncore,thr);
  }

//
// LIBRARIES
//

  Library* currentLibrary(const std::string &name);

  Library* lib(const std::string& name) const;

  void registerLib(Library* lib);

  void unregisterLib(Library* lib);

  bool handleLibraryRequest(const std::string& name, Request* req);

  void handleRequest(Request* req);

};

} // namespace Hg
} // namespace SST
