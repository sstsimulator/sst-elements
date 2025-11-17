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

#include <mercury/components/operating_system_api.h>
#include <mercury/operating_system/launch/app_launcher_fwd.h>
#include <mercury/operating_system/launch/app_launch_request.h>
#include <mercury/common/timestamp.h>

#include <cstdint>
#include <memory>

namespace SST {
namespace Hg {

class OperatingSystemImpl {

  friend class OperatingSystem;
  friend class OperatingSystemCL;

public:

  OperatingSystemImpl(SST::ComponentId_t id, SST::Params &params, OperatingSystemAPI* api);

  ~OperatingSystemImpl();

  static inline OperatingSystemAPI*& staticOsThreadContext(){
    int thr = ThreadInfo::currentPhysicalThreadId();
    return active_os_[thr];
  }

  static Thread *currentThread() {
    return staticOsThreadContext()->activeThread();
  }

  static OperatingSystemAPI* currentOs(){
    return staticOsThreadContext();
  }

  inline OperatingSystemAPI *&activeOs() {
    return active_os_[physical_thread_id_];
  }

  void init(unsigned phase);

protected:

//////////////////////////////////////////////////////////////////
// OperatingSystemAPI Implemetations (forwarded by OS components)
//////////////////////////////////////////////////////////////////

  void handleEvent(SST::Event *ev);

  static size_t stacksize() {
    return sst_hg_global_stacksize;
  }

  std::function<void(NetworkMessage*)> nicDataIoctl();

  std::function<void(NetworkMessage*)> nicCtrlIoctl();

   void setParentNode(NodeBase* parent) {
    node_ = parent;
  }

  NodeBase* node() const {
    return node_;
  }

  NodeId addr() const {
    return my_addr_;
  }

  void setNumRanks(unsigned int ranks) {
    nranks_ = ranks;
  }

  unsigned int numRanks() {
    return nranks_;
  }

  void setRanksPerNode(unsigned int npernode) {
    npernode_ = npernode;
  }

  unsigned int ranksPerNode() {
    return npernode_;
  }

  void startApp(App* theapp, const std::string&  /*unique_name*/);

  Thread *activeThread() const { 
    return active_thread_; 
  }

  void startThread(Thread* t);

  void joinThread(Thread* t);

  void switchToThread(Thread* tothread);

  void scheduleThreadDeletion(Thread* thr);

  void completeActiveThread();

  void block();

  void unblock(Thread* thr);

  void blockTimeout(TimeDelta delay);

  UniqueEventId allocateUniqueId() {
    next_outgoing_id_.msg_num++;
    return next_outgoing_id_;
  }

  int allocateMutex();

  mutex_t* getMutex(int id);

  bool eraseMutex(int id);

  int allocateCondition();

  condition_t* getCondition(int id);

  bool eraseCondition(int id);

  EventLibrary* eventLibrary(const std::string& name) const;

  void registerEventLib(EventLibrary* lib);

  void unregisterEventLib(EventLibrary* lib);

  void handleRequest(Request* req);

///////////
// Private
///////////

private:

  static std::vector<OperatingSystemAPI*> active_os_;

  SST::Params params_;
  std::unique_ptr<SST::Output> out_;
  SST::Hg::NodeBase* node_;
  OperatingSystemAPI* os_api_;
  AppLauncher* app_launcher_;

  NodeId my_addr_;
  unsigned int verbose_;
  unsigned int nranks_;
  unsigned int npernode_;
  UniqueEventId next_outgoing_id_;

  /// The caller context (main DES thread).  We go back
  /// to this context on every context switch.
  ThreadContext *des_context_;

  uint32_t physical_thread_id_;
  Thread* active_thread_;
  Thread* blocked_thread_;
  int next_condition_;
  int next_mutex_;
  std::map<uint32_t, Thread*> running_threads_;
  std::map<int, condition_t> conditions_;
  std::map<int, mutex_t> mutexes_;


  std::map<std::string, Library*> internal_apis_;
  std::list<AppLaunchRequest*> requests_;
  std::unordered_map<std::string, EventLibrary*> libs_;
  std::unordered_map<EventLibrary*, int> lib_refcounts_;
  std::map<std::string, std::list<Request*>> pending_library_request_;

  void initThreading(SST::Params& params);

  const std::string& tickIntervalString()
  {
    return _tick_spacing_string_;
  }

  void addLaunchRequests(SST::Params& params);

  void setThreadId(uint32_t threadId) {
    physical_thread_id_ = threadId;
  }

  // uint32_t threadId() {
  //   return thread_id_;
  // }

  bool handleEventLibraryRequest(const std::string& name, Request* req);
};

} // namespace Hg
} // namespace SST
