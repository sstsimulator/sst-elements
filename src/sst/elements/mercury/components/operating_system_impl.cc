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

#include <sst/core/factory.h>
#include <mercury/components/operating_system.h>

#include <mercury/common/events.h>
#include <mercury/common/request.h>
#include <mercury/components/node.h>
#include <mercury/operating_system/launch/app_launcher.h>
#include <mercury/operating_system/libraries/unblock_event.h>
#include <mercury/operating_system/process/app.h>
#include <mercury/operating_system/libraries/event_library.h>
#include <mercury/operating_system/libraries/unblock_event.h>
#include <mercury/operating_system/process/thread_id.h>
#include <mercury/operating_system/threading/stack_alloc.h>
#include <sst/core/eli/elementbuilder.h>
#include <sst/core/params.h>
#include <stdlib.h>
#include <sys/mman.h>

extern "C" {
void* sst_hg_nullptr = nullptr;
void* sst_hg_nullptr_send = nullptr;
void* sst_hg_nullptr_recv = nullptr;
void* sst_hg_nullptr_range_max = nullptr;
}
static uintptr_t sst_hg_nullptr_range = 0;

namespace SST {
namespace Hg {

std::vector<OperatingSystemAPI*> OperatingSystemImpl::active_os_;

class DeleteThreadEvent :
    public ExecutionEvent
{
public:
  DeleteThreadEvent(Thread* thr) :
    thr_(thr)
  {
  }

  void execute() override {
    delete thr_;
  }

protected:
  Thread* thr_;
};

OperatingSystemImpl::OperatingSystemImpl(SST::ComponentId_t id, SST::Params& params, OperatingSystemAPI* api) :
  des_context_(nullptr),
  next_condition_(0),
  next_mutex_(0),
  params_(params),
  os_api_(api)
{
  TimeDelta::initStamps(TimeDelta::ASEC_PER_TICK);

  if (active_os_.size() == 0){
    RankInfo num_ranks = os_api_->getNumRanks();
    active_os_.resize(num_ranks.thread);
  }

  if (sst_hg_nullptr == nullptr){

    int range_bit_size = 30;
    sst_hg_nullptr_range = 1ULL << range_bit_size;
    sst_hg_nullptr = mmap(nullptr, sst_hg_nullptr_range, PROT_NONE,
                          MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (sst_hg_nullptr == ((void *)-1)) {
      sst_hg_abort_printf("address reservation failed in mmap: %s",
                          ::strerror(errno));
    }
    sst_hg_nullptr_send = sst_hg_nullptr;
    sst_hg_nullptr_recv = ((char*)sst_hg_nullptr) + (sst_hg_nullptr_range/2);
    sst_hg_nullptr_range_max = ((char*)sst_hg_nullptr) + sst_hg_nullptr_range;
  }

  StackAlloc::init(params);
  initThreading(params);
}

OperatingSystemImpl::~OperatingSystemImpl()
{
  for (auto& pair : pending_library_request_){
    std::string name = pair.first;
    for (Request *req : pair.second) {
      sst_hg_abort_printf(
          "OperatingSystem:: never registered library %s on os %d for event %s",
          name.c_str(), int(addr()), toString(req).c_str());
    }
  }

  if (des_context_) {
    des_context_->destroyContext();
    delete des_context_;
  }
}

void 
OperatingSystemImpl::init(unsigned phase) {
  if (phase == 0) {
    active_os_[physical_thread_id_] = nullptr;
    my_addr_ = node_->addr();
    next_outgoing_id_.src_node = my_addr_;
    next_outgoing_id_.msg_num = 0;
    out_ = std::unique_ptr<SST::Output>(
        new SST::Output(sprintf("Node%d:HgOperatingSystem:", my_addr_),
                        verbose_, 0, Output::STDOUT));
    out_->debug(CALL_INFO, 1, 0, "constructing\n");
  }
}

//////////////////////////////////////////////////////////////////
// OperatingSystemAPI Implemetations (forwarded by OS components)
//////////////////////////////////////////////////////////////////

/* Event handler
   * Incoming events are scanned and deleted
   * Record if the event received is the last one our neighbor will send
   */
void
OperatingSystemImpl::handleEvent(SST::Event *ev) {
  if (auto *req = dynamic_cast<AppLaunchRequest *>(ev)) {
    Params params = req->params();
    out_->debug(CALL_INFO, 1, 0, "app name: %s\n",
                  params.find<std::string>("name").c_str());
    app_launcher_->incomingRequest(req);
  }
  // else {
  //   sst_hg_abort_printf("Error! Bad Event Type received by %s!\n",
  //                       getName().c_str());
  // }
}

std::function<void(NetworkMessage*)>
OperatingSystemImpl::nicDataIoctl()
{
  return node_->nic()->dataIoctl();
}

std::function<void(NetworkMessage*)>
OperatingSystemImpl::nicCtrlIoctl()
{
  return node_->nic()->ctrlIoctl();
}

void
OperatingSystemImpl::startApp(App *theapp,
                          const std::string & /*unique_name*/) {
  out_->debug(CALL_INFO, 1, 0, "starting app %d:%d on physical thread %d\n",
                int(theapp->tid()), int(theapp->aid()), physical_thread_id_);
  theapp->set_os_api(os_api_);
  // this should be called from the actual thread running it
  initThreading(params_);
  startThread(theapp);
}

void
OperatingSystemImpl::startThread(Thread* t)
{
  if (active_thread_){
      //crap - can't do this on this thread - need to do on DES thread
      os_api_->selfEventLink()->send(0,newCallback(this, &OperatingSystemImpl::startThread, t));
    } else {
      active_thread_ = t;
      //activeOs() = this;
      //os_api_->setActiveOs();
      activeOs() = os_api_;
      App* parent = t->parentApp();
      void* stack = StackAlloc::alloc();
      t->initThread(
            parent->params(),
            physical_thread_id_,
            des_context_,
            stack,
            StackAlloc::stacksize(),
            parent->globalsStorage(),
            parent->newTlsStorage());
    }
  running_threads_[t->tid()] = t;
}

void
OperatingSystemImpl::joinThread(Thread* t)
{
  sst_hg_abort_printf("OperatingSystem::joinThread not fully implemented");
  if (t->getState() != Thread::DONE) {
    // key* k = key::construct();
    //       os_debug("joining thread %ld - thread not done so blocking on
    //       thread %p",
    //           t->threadId(), active_thread_);
    t->joiners_.push(active_thread_);
    //      int ncores = active_thread_->numActiveCcores();
    //      //when joining - release all cores
    //      compute_sched_->releaseCores(ncores, active_thread_);
    //      block();
    //      compute_sched_->reserveCores(ncores, active_thread_);
  } else {
    //      os_debug("joining completed thread %ld", t->threadId());
  }
  running_threads_.erase(t->tid());
  delete t;
}

void
OperatingSystemImpl::switchToThread(Thread* tothread)
{
  if (active_thread_ != nullptr){ //not an error
      //but this must be thrown over to the DES context to actually execute
      //we cannot context switch directly from subthread to subthread
      os_api_->selfEventLink()->send(newCallback(this, &OperatingSystemImpl::switchToThread, tothread));
      return;
    }

  out_->debug(CALL_INFO, 1, 0,
                "switching to context %d on physical thread %d\n",
                tothread->threadId(), physical_thread_id_);
  if (active_thread_ == blocked_thread_) {
    blocked_thread_ = nullptr;
  }
  active_thread_ = tothread;
  //activeOs() = this;
  //os_api_->setActiveOs();
  activeOs() = os_api_;
  tothread->context()->resumeContext(des_context_);
  out_->debug(CALL_INFO, 1, 0,
                "switched back from context %d to main thread %d\n",
                tothread->threadId(), physical_thread_id_);
  /** back to main thread */
  active_thread_ = nullptr;
}

void
OperatingSystemImpl::scheduleThreadDeletion(Thread* thr)
{
  //JJW 11/6/2014 This here is weird
  //The thread has run out of work and is terminating
  //However, because of weird thread swapping the DES thread
  //might still operate on the thread... we need to delay the delete
  //until the DES thread has completely finished processing its current event
  os_api_->selfEventLink()->send(new DeleteThreadEvent(thr));
  node_->endSim();
}

void
OperatingSystemImpl::completeActiveThread()
{
  //    if (gdb_active_){
  //      all_threads_.erase(active_thread_->tid());
  //    }
  Thread* thr_todelete = active_thread_;

  //if any threads waiting on the join, unblock them
  while (!thr_todelete->joiners_.empty()) {
    Thread *blocker = thr_todelete->joiners_.front();
    out_->debug(CALL_INFO, 1, 0, "thread %d is unblocking joiner %p\n",
                  thr_todelete->threadId(), blocker);
    unblock(blocker);
    // to_awake_.push(thr_todelete->joiners_.front());
    thr_todelete->joiners_.pop();
  }
  active_thread_ = nullptr;
  out_->debug(CALL_INFO, 1, 0, "completing context %d on thread %d\n",
                thr_todelete->threadId(), physical_thread_id_);
  thr_todelete->context()->completeContext(des_context_);
}

void
OperatingSystemImpl::block()
{
  Timestamp before = os_api_->now();
  //back to main DES thread
  if (active_thread_ == nullptr) {
    sst_hg_abort_printf("null active_thread_, blocking main DES thread?");
  }
  ThreadContext* old_context = active_thread_->context();
  if (old_context == des_context_){
    sst_hg_abort_printf("blocking main DES thread");
  }
  Thread* old_thread = active_thread_;
  //reset the time flag
  active_thread_->setTimedOut(false);
  out_->debug(CALL_INFO, 1, 0, "pausing context %d on physical thread %d\n",
                active_thread_->threadId(), physical_thread_id_);
  blocked_thread_ = active_thread_;
  active_thread_ = nullptr;
  old_context->pauseContext(des_context_);

  //restore state to indicate this thread and this OS are active again
  //activeOs() = this;
  //os_api_->setActiveOs();
  activeOs() = os_api_;
  out_->debug(CALL_INFO, 1, 0, "resuming context %d on physical thread %d\n",
                active_thread_->threadId(), physical_thread_id_);
  active_thread_ = old_thread;
  active_thread_->incrementBlockCounter();

   //collect any statistics associated with the elapsed time
  // Timestamp after = os_api_->now();
  // TimeDelta elapsed = after - before;
}

void
OperatingSystemImpl::unblock(Thread* thr)
{
  if (thr->isCanceled()){
      //just straight up delete the thread
      //it shall be neither seen nor heard
      delete thr;
    } else {
      switchToThread(thr);
    }
}

void
OperatingSystemImpl::blockTimeout(TimeDelta delay)
{
  os_api_->sendDelayedExecutionEvent(delay, new TimeoutEvent(os_api_, active_thread_));
  block();
}

int
OperatingSystemImpl::allocateMutex()
{
  int id = next_mutex_++;
  mutexes_[id]; //implicit make
  return id;
}

mutex_t*
OperatingSystemImpl::getMutex(int id)
{
  std::map<int,mutex_t>::iterator it = mutexes_.find(id);
  if (it==mutexes_.end()){
      return 0;
    } else {
      return &it->second;
    }
}

bool
OperatingSystemImpl::eraseMutex(int id)
{
  std::map<int,mutex_t>::iterator it = mutexes_.find(id);
  if (it == mutexes_.end()){
      return false;
    } else {
      mutexes_.erase(id);
      return true;
    }
}

int
OperatingSystemImpl::allocateCondition()
{
  int id = next_condition_++;
  conditions_[id]; //implicit make
  return id;
}

condition_t*
OperatingSystemImpl::getCondition(int id)
{
  std::map<int,condition_t>::iterator it = conditions_.find(id);
  if (it==conditions_.end()){
      return 0;
    } else {
      return &it->second;
    }
}

bool
OperatingSystemImpl::eraseCondition(int id)
{
  std::map<int,condition_t>::iterator it = conditions_.find(id);
  if (it == conditions_.end()){
      return false;
    } else {
      conditions_.erase(id);
      return true;
    }
}

EventLibrary*
OperatingSystemImpl::eventLibrary(const std::string& name) const
{
  auto it = libs_.find(name);
  if (it == libs_.end()) {
    return nullptr;
  } else {
    return it->second;
  }
}

void
OperatingSystemImpl::registerEventLib(EventLibrary* lib)
{
#if SST_HG_SANITY_CHECK
  if (lib->libName() == "") {
    sprockit::abort("OperatingSystem: trying to register a lib with no name");
  }
#endif
  out_->debug(CALL_INFO, 1, 0, "registering lib %s:%p\n", lib->libName().c_str(), lib);
  int& refcount = lib_refcounts_[lib];
  ++refcount;
  libs_[lib->libName()] = lib;
  out_->debug(CALL_INFO, 1, 0, "OS %d should no longer drop events for %s\n",
                addr(), lib->libName().c_str());
  auto iter = pending_library_request_.find(lib->libName());
  if (iter != pending_library_request_.end()) {
    const std::list<Request *> reqs = iter->second;
    for (Request *req : reqs) {
      out_->debug(CALL_INFO, 1, 0, "delivering delayed event to lib %s: %s\n",
                    lib->libName().c_str(), toString(req).c_str());
      os_api_->sendExecutionEventNow(newCallback(lib, &EventLibrary::incomingRequest, req));
    }
    pending_library_request_.erase(iter);
  }
}

void
OperatingSystemImpl::unregisterEventLib(EventLibrary *lib) {
  out_->debug(CALL_INFO, 1, 0, "unregistering lib %s\n",
                lib->libName().c_str());
  int &refcount = lib_refcounts_[lib];
  if (refcount == 1) {
    lib_refcounts_.erase(lib);
    out_->debug(CALL_INFO, 1, 0, "OS %d will now drop events for %s\n",
                  addr(), lib->libName().c_str());
    libs_.erase(lib->libName());
    //delete lib;
  } else {
    --refcount;
  }
}

void
OperatingSystemImpl::handleRequest(Request* req)
{
  //this better be an incoming event to a library, probably from off node
  Flow* libmsg = dynamic_cast<Flow*>(req);
  if (!libmsg) {
    out_->debug(CALL_INFO, 1, 0,
                "OperatingSystem::handle_event: got event %s instead of library event\n",
                toString(req).c_str());
    // if libmsg is null we cannot continue
    return;
  }

  bool found = handleEventLibraryRequest(libmsg->libname(), req);
  if (!found) {
    out_->debug(CALL_INFO, 1, 0, "delaying event to lib %s: %s",
                libmsg->libname().c_str(), libmsg->toString().c_str());
    pending_library_request_[libmsg->libname()].push_back(req);
  }
}

///////////
// Private
///////////

void
OperatingSystemImpl::initThreading(SST::Params& params)
{
  if (des_context_){
      return; //already done
    }

  auto factory = Factory::getFactory();
  des_context_ = factory->Create<ThreadContext>("hg.fcontext");

  des_context_->initContext();

  active_thread_ = nullptr;
}

void
OperatingSystemImpl::addLaunchRequests(SST::Params& params)
{
  bool keep_going = true;
  int aid = 1;
  SST::Params all_app_params = params.get_scoped_params("app");
  while (keep_going || aid < 10) {
    std::string name = sprintf("app%d", aid);
    SST::Params app_params = params.get_scoped_params(name);
    if (!app_params.empty()) {
      app_params.insert(all_app_params);
      //      bool terminate_on_end = app_params.find<bool>("terminate", false);
      //      if (terminate_on_end){
      //        terminators_.insert(aid);
      //      }
      out_->debug(CALL_INFO, 1, 0, "adding app launch request %d:%s\n", aid,
                    name.c_str());
      out_->flush();
      for (int i = 0; i < npernode_; ++i) {
        AppLaunchRequest *mgr =
            new AppLaunchRequest(app_params, AppId(aid), name);
        requests_.push_back(mgr);
      }
      keep_going = true;
    }
    else {
      keep_going = false;
    }
    ++aid;
  }
}

bool
OperatingSystemImpl::handleEventLibraryRequest(const std::string& name, Request* req)
{
  auto it = libs_.find(name);
  bool found = it != libs_.end();
  if (found){
    EventLibrary* lib = it->second;
    out_->debug(CALL_INFO, 1, 0, "delivering message to event lib %s:%p: %s\n",
                  name.c_str(), lib, toString(req).c_str());
    lib->incomingRequest(req);
  } else {
    out_->debug(CALL_INFO, 1, 0, "unable to deliver message to event lib %s: %s\n",
                  name.c_str(), toString(req).c_str());
  }
  return found;
}

} // namespace Hg
} // namespace SST
