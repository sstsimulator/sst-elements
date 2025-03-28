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

#include <mercury/common/component.h>

#include <sst/core/link.h>

#include <sst/core/eli/elementbuilder.h>
#include <mercury/components/operating_system_base.h>
#include <mercury/components/node_fwd.h>

#include <cstdint>
#include <memory>

namespace SST {
namespace Hg {

extern template SST::TimeConverter* HgBase<SST::SubComponent>::time_converter_;

class OperatingSystem : public SST::Hg::OperatingSystemBase {

public:
 
  SST_ELI_REGISTER_SUBCOMPONENT_DERIVED_API(SST::Hg::OperatingSystem, SST::Hg::OperatingSystemBase, SST::Hg::NodeBase*)

  SST_ELI_REGISTER_SUBCOMPONENT(
    SST::Hg::OperatingSystem,
    "hg",
    "OperatingSystem",
    SST_ELI_ELEMENT_VERSION(0,0,1),
    "Mercury Operating System",
    SST::Hg::OperatingSystem
  )

  OperatingSystem(SST::ComponentId_t id, SST::Params& params, NodeBase* parent);

  ~OperatingSystem();

  void setup() override;

  void handleEvent(SST::Event *ev) override;

  bool clockTic(SST::Cycle_t) override {
    //noop
    return true;
  }

  static size_t stacksize() {
    return sst_hg_global_stacksize;
  }

  std::function<void(NetworkMessage*)> nicDataIoctl() override;

  std::function<void(NetworkMessage*)> nicCtrlIoctl() override;

  /**
   * @brief block Block the currently running thread context.
   * This must be called from an application thread, NOT the DES thread
   * @param req [in-out] A key that will store blocking data that will be needed
   *                     for later unblocking
   * @return
   */
  void block() override;

  void unblock(Thread* thr) override;


  void blockTimeout(TimeDelta delay) override;
  // void sleep(TimeDelta time);
  // void compute(TimeDelta time);

  void startApp(App* theapp, const std::string&  /*unique_name*/) override;
  void startThread(Thread* t) override;
  void joinThread(Thread* t) override;
  void scheduleThreadDeletion(Thread* thr) override;
  void switchToThread(Thread* tothread) override;
  void completeActiveThread() override;

  static Thread* currentThread(){
    return staticOsThreadContext()->activeThread();
  }

  static inline OperatingSystem*& staticOsThreadContext(){
//  #if SST_HG_USE_MULTITHREAD
    int thr = ThreadInfo::currentPhysicalThreadId();
    return active_os_[thr];
//  #else
//    return active_os_;
//  #endif
  }

  inline OperatingSystem*& activeOs() {
//#if SST_HG_USE_MULTITHREAD
  return active_os_[threadId()];
//#else
//  return active_os_;
//#endif
  }

  Thread* activeThread() const override {
    return active_thread_;
  }

  static OperatingSystem* currentOs(){
    return staticOsThreadContext();
  }

  UniqueEventId allocateUniqueId() override {
    next_outgoing_id_.msg_num++;
    return next_outgoing_id_;
  }

protected:

  Thread* active_thread_;
  SST::Hg::NodeBase* node_;
  std::map<std::string, Library*> internal_apis_;

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

  unsigned int verbose_;
  unsigned int nranks_;
  unsigned int npernode_;
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

  std::unordered_map<std::string, EventLibrary*> libs_;
  std::unordered_map<EventLibrary*, int> lib_refcounts_;
  std::map<std::string, std::list<Request*>> pending_library_request_;

  NodeId my_addr_;
  UniqueEventId next_outgoing_id_;

//  int next_condition_;
//  int next_mutex_;
//  std::map<int, condition_t> conditions_;
//  std::map<int, mutex_t> mutexes_;

  //#if SST_HG_USE_MULTITHREAD
    static std::vector<OperatingSystem*> active_os_;
  //#else
  //  static OperatingSystem* active_os_;
  //#endif

 public:
  static SST::TimeConverter* timeConverter() {
    return time_converter_;
  }

 public:

  NodeBase* node() const override {
    return node_;
  }

  Thread* getThread(uint32_t threadId) const override;

  NodeId addr() const override {
    return my_addr_;
  }

  /**
   * Allocate a unique ID for a mutex variable
   * @return The unique ID
   */
  int allocateMutex() override;

  /**
   * Fetch a mutex object corresponding to their ID
   * @param id
   * @return The mutex object corresponding to the ID. Return NULL if no mutex is found.
   */
  mutex_t* getMutex(int id) override;

  bool eraseMutex(int id) override;

  int allocateCondition() override;

  bool eraseCondition(int id) override;

  condition_t* getCondition(int id) override;

  void set_nranks(unsigned int ranks) override {
    nranks_ = ranks;
  }

  unsigned int nranks() override {
    return nranks_;
  }

  void set_npernode(unsigned int npernode) override {
    npernode_ = npernode;
  }

  unsigned int npernode() override {
    return npernode_;
  }

//
// EVENT LIBRARIES
//

  // Library* currentEventLibrary(const std::string &name);

  EventLibrary* eventLibrary(const std::string& name) const override;

  void registerEventLib(EventLibrary* lib) override;

  void unregisterEventLib(EventLibrary* lib) override;

  bool handleEventLibraryRequest(const std::string& name, Request* req) override;

  void handleRequest(Request* req) override;

};

} // namespace Hg
} // namespace SST
