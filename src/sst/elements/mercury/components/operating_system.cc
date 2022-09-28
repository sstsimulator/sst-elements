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

#include <components/operating_system.h>

#include <sst/core/params.h>
#include <common/events.h>
#include <common/factory.h>
#include <common/request.h>
#include <components/node.h>
#include <operating_system/launch/app_launcher.h>
#include <operating_system/process/app.h>
#include <operating_system/process/thread_id.h>
#include <operating_system/threading/stack_alloc.h>
#include <stdlib.h>

extern "C" void* sst_hg_nullptr = nullptr;
extern "C" void* sst_hg_nullptr_send = nullptr;
extern "C" void* sst_hg_nullptr_recv = nullptr;
extern "C" void* sst_hg_nullptr_range_max = nullptr;
static uintptr_t sst_hg_nullptr_range = 0;

namespace SST {
namespace Hg {

extern template SST::TimeConverter* HgBase<SST::SubComponent>::time_converter_;

OperatingSystem* OperatingSystem::active_os_ = nullptr;

//SST::TimeConverter* OperatingSystem::time_converter_ = nullptr;

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

OperatingSystem::OperatingSystem(SST::ComponentId_t id, SST::Params& params, Node* parent) :
  SST::Hg::SubComponent(id),
  node_(parent),
  des_context_(nullptr),
  next_condition_(0),
  next_mutex_(0)
{
  my_addr_ = node_ ? node_->addr() : 0;
  auto os_params = params.get_scoped_params("OperatingSystem");
  os_params.print_all_params(std::cerr);
  unsigned int verbose = os_params.find<unsigned int>("verbose",0);
  out_ = std::make_unique<SST::Output>(sprintf("Node%d:OperatingSystem:", my_addr_), verbose, 0, Output::STDOUT);
  out_->debug(CALL_INFO, 1, 0, "constructing\n");

  if (sst_hg_nullptr == nullptr){

    int range_bit_size = 30;
    sst_hg_nullptr_range = 1ULL<<range_bit_size;
    sst_hg_nullptr = mmap(nullptr, sst_hg_nullptr_range,
                          PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (sst_hg_nullptr == ((void*)-1)){
        sst_hg_abort_printf("address reservation failed in mmap: %s", ::strerror(errno));
    }
    sst_hg_nullptr_send = sst_hg_nullptr;
    sst_hg_nullptr_recv = ((char*)sst_hg_nullptr) + (sst_hg_nullptr_range/2);
    sst_hg_nullptr_range_max = ((char*)sst_hg_nullptr) + sst_hg_nullptr_range;
  }

  //eventSize = params.find<std::int64_t>("eventSize", 16);
  if (!time_converter_){
      time_converter_ = SST::BaseComponent::getTimeConverter(tickIntervalString());
    }

  // Configure self link to handle event timing
  selfEventLink_ = configureSelfLink("self", time_converter_, new Event::Handler<Hg::OperatingSystem>(this, &OperatingSystem::handleEvent));
  assert(selfEventLink_);
  selfEventLink_->setDefaultTimeBase(time_converter_);

  out_->debug(CALL_INFO, 1, 0, "adding launch requests\n");
  app_launcher_ = new AppLauncher(this);
  addLaunchRequests(params);

  //assume simple for now
  compute_sched_ = SST::Hg::create<ComputeScheduler>(
        "hg", params.find<std::string>("compute_scheduler", "simple"),
        params, this, node_ ? node_->ncores() : 1, node_ ? node_->nsockets() : 1);

  StackAlloc::init(params);
  initThreading(params);
}

OperatingSystem::~OperatingSystem()
{
  for (auto& pair : pending_library_request_){
    std::string name = pair.first;
    for (Request* req : pair.second){
        sst_hg_abort_printf("OperatingSystem:: never registered library %s on os %d for event %s",
                            name.c_str(), int(addr()),
                            toString(req).c_str());
    }
  }

  if (des_context_) {
    des_context_->destroyContext();
    delete des_context_;
  }
  if (compute_sched_) delete compute_sched_;
}

void
OperatingSystem::setup() {
  SubComponent::setup();
  for (auto r : requests_)
    selfEventLink_->send(r);
}

void
OperatingSystem::initThreading(SST::Params& params)
{
  if (des_context_){
      return; //already done
    }

  des_context_ = create<ThreadContext>(
        "macro", params.find<std::string>("context", ThreadContext::defaultThreading()));

  des_context_->initContext();

  active_thread_ = nullptr;
}

void
OperatingSystem::startThread(Thread* t)
{
  if (active_thread_){
      //crap - can't do this on this thread - need to do on DES thread
      selfEventLink_->send(0,time_converter_,newCallback(this, &OperatingSystem::startThread, t));
    } else {
      active_thread_ = t;
      activeOs() = this;
      App* parent = t->parentApp();
      void* stack = StackAlloc::alloc();
      t->initThread(
            parent->params(),
            threadId(),
            des_context_,
            stack,
            StackAlloc::stacksize(),
            parent->globalsStorage(),
            nullptr);
    }
  running_threads_[t->tid()] = t;

  //    if (gdb_active_){
  //      static thread_lock all_threads_lock;
  //      all_threads_lock.lock();
  //      auto tid = t->tid();
  //      Thread*& thrInMap = all_threads_[tid];
  //      if (thrInMap){
  //        spkt_abort_printf("error: thread %d already exists for app %d",
  //                          t->tid(), thrInMap->aid());
  //      }
  //      thrInMap = t;
  //      all_threads_lock.unlock();
  //    }

}

/* Event handler
   * Incoming events are scanned and deleted
   * Record if the event received is the last one our neighbor will send
   */
void OperatingSystem::handleEvent(SST::Event *ev) {
  if (auto *req = dynamic_cast<AppLaunchRequest*>(ev)) {
      Params params = req->params();
      //params.print_all_params(std::cerr);
      out_->debug(CALL_INFO, 1, 0, "app name: %s\n",  params.find<std::string>("name").c_str());
      app_launcher_->incomingRequest(req);
    }
  else {
      sst_hg_abort_printf("Error! Bad Event Type received by %s!\n",
                          getName().c_str());
    }
}

void
OperatingSystem::addLaunchRequests(SST::Params& params)
{
  bool keep_going = true;
  int aid = 1;
  SST::Params all_app_params = params.get_scoped_params("app");
  while (keep_going || aid < 10){
      std::string name = sprintf("app%d",aid);
      SST::Params app_params = params.get_scoped_params(name);
      app_params.print_all_params(std::cerr);
      if (!app_params.empty()){
          app_params.insert(all_app_params);
          //      bool terminate_on_end = app_params.find<bool>("terminate", false);
          //      if (terminate_on_end){
          //        terminators_.insert(aid);
          //      }
          out_->debug(CALL_INFO, 1, 0, "adding app launch request %d:%s\n", aid, name.c_str());
          out_->flush();
          AppLaunchRequest* mgr = new AppLaunchRequest(app_params, AppId(aid), name);
          requests_.push_back(mgr);
          keep_going = true;

          App::lockDlopen(aid);
        } else {
          keep_going = false;
        }
      ++aid;
    }
}

void
OperatingSystem::startApp(App* theapp, const std::string&  /*unique_name*/)
{
    out_->debug(CALL_INFO, 1, 0, "starting app %d:%d on thread %d\n",
                int(theapp->tid()), int(theapp->aid()), threadId());
  //this should be called from the actual thread running it
  initThreading(params_);
  startThread(theapp);
}

void
OperatingSystem::joinThread(Thread* t)
{
  sst_hg_abort_printf("OperatingSystem::joinThread not fully implemented");
  if (t->getState() != Thread::DONE) {
      //key* k = key::construct();
      //      os_debug("joining thread %ld - thread not done so blocking on thread %p",
      //          t->threadId(), active_thread_);
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
OperatingSystem::scheduleThreadDeletion(Thread* thr)
{
  //JJW 11/6/2014 This here is weird
  //The thread has run out of work and is terminating
  //However, because of weird thread swapping the DES thread
  //might still operate on the thread... we need to delay the delete
  //until the DES thread has completely finished processing its current event
  out_->debug(CALL_INFO, 1, 0, "scheduling thread deletion\n");
  selfEventLink_->send(new DeleteThreadEvent(thr));
}

void
OperatingSystem::completeActiveThread()
{
  //    if (gdb_active_){
  //      all_threads_.erase(active_thread_->tid());
  //    }
  Thread* thr_todelete = active_thread_;

  //if any threads waiting on the join, unblock them
  out_->debug(CALL_INFO, 1, 0, "completing thread %ld\n", thr_todelete->threadId());
  while (!thr_todelete->joiners_.empty()) {
      Thread* blocker = thr_todelete->joiners_.front();
      out_->debug(CALL_INFO, 1, 0, "thread %ld is unblocking joiner %p\n",
                  thr_todelete->threadId(), blocker);
      unblock(blocker);
      //to_awake_.push(thr_todelete->joiners_.front());
      thr_todelete->joiners_.pop();
    }
  active_thread_ = nullptr;  
  out_->debug(CALL_INFO, 1, 0, "completing context for %ld\n", thr_todelete->threadId());
  thr_todelete->context()->completeContext(des_context_);
}

void
OperatingSystem::switchToThread(Thread* tothread)
{
  if (active_thread_ != nullptr){ //not an error
      //but this must be thrown over to the DES context to actually execute
      //we cannot context switch directly from subthread to subthread
      selfEventLink_->send(newCallback(this, &OperatingSystem::switchToThread, tothread));
      return;
    }

  out_->debug(CALL_INFO, 1, 0, "switching to thread %d\n", tothread->threadId());
  if (active_thread_ == blocked_thread_){
      blocked_thread_ = nullptr;
    }
  active_thread_ = tothread;
  activeOs() = this;
  tothread->context()->resumeContext(des_context_);
  out_->debug(CALL_INFO, 0, 0, "switched back from thread %d to main thread\n", tothread->threadId());
  /** back to main thread */
  active_thread_ = nullptr;
}

void
OperatingSystem::block()
{
  Timestamp before = now();
  //back to main DES thread
  ThreadContext* old_context = active_thread_->context();
  if (old_context == des_context_){
      sst_hg_abort_printf("blocking main DES thread");
  }
  Thread* old_thread = active_thread_;
  //reset the time flag
  active_thread_->setTimedOut(false);
  out_->debug(CALL_INFO, 1, 0, "pausing context on thread %d\n", active_thread_->threadId());
  blocked_thread_ = active_thread_;
  active_thread_ = nullptr;
  old_context->pauseContext(des_context_);

//  while(hold_for_gdb_){
//    sst_gdb_swap();  //do nothing - this is only for gdb purposes
//  }

  //restore state to indicate this thread and this OS are active again
  activeOs() = this;
  out_->debug(CALL_INFO, 1, 0, "resuming context on thread %d\n", active_thread_->threadId());
  active_thread_ = old_thread;
  active_thread_->incrementBlockCounter();

   //collect any statistics associated with the elapsed time
  Timestamp after = now();
  TimeDelta elapsed = after - before;

//  if (elapsed.ticks()){
//    active_thread_->collectStats(before, elapsed);
//  }
}

void
OperatingSystem::unblock(Thread* thr)
{
  if (thr->isCanceled()){
      //just straight up delete the thread
      //it shall be neither seen nor heard
      delete thr;
    } else {
      switchToThread(thr);
    }
}

Thread*
OperatingSystem::getThread(uint32_t threadId) const
{
  auto iter = running_threads_.find(threadId);
  if (iter == running_threads_.end()){
      return nullptr;
    } else {
      return iter->second;
    }
}

mutex_t*
OperatingSystem::getMutex(int id)
{
  std::map<int,mutex_t>::iterator it = mutexes_.find(id);
  if (it==mutexes_.end()){
      return 0;
    } else {
      return &it->second;
    }
}

int
OperatingSystem::allocateMutex()
{
  int id = next_mutex_++;
  mutexes_[id]; //implicit make
  return id;
}

bool
OperatingSystem::eraseMutex(int id)
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
OperatingSystem::allocateCondition()
{
  int id = next_condition_++;
  conditions_[id]; //implicit make
  return id;
}

bool
OperatingSystem::eraseCondition(int id)
{
  std::map<int,condition_t>::iterator it = conditions_.find(id);
  if (it == conditions_.end()){
      return false;
    } else {
      conditions_.erase(id);
      return true;
    }
}

condition_t*
OperatingSystem::getCondition(int id)
{
  std::map<int,condition_t>::iterator it = conditions_.find(id);
  if (it==conditions_.end()){
      return 0;
    } else {
      return &it->second;
    }
}

//
// LIBRARIES
//

Library*
OperatingSystem::currentLibrary(const std::string &name)
{
  return currentOs()->lib(name);
}

Library*
OperatingSystem::lib(const std::string& name) const
{
  auto it = libs_.find(name);
  if (it == libs_.end()) {
    return nullptr;
  } else {
    return it->second;
  }
}

void
OperatingSystem::registerLib(Library* lib)
{
#if SSTMAC_SANITY_CHECK
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
  if (iter != pending_library_request_.end()){
    const std::list<Request*> reqs = iter->second;
    for (Request* req : reqs){
        out_->debug(CALL_INFO, 1, 0, "delivering delayed event to lib %s: %s\n",
                    lib->libName().c_str(), toString(req).c_str());
      sendExecutionEventNow(newCallback(lib, &Library::incomingRequest, req));
    }
    pending_library_request_.erase(iter);
  }
}

void
OperatingSystem::unregisterLib(Library* lib)
{
  out_->debug(CALL_INFO, 1, 0, "unregistering lib %s\n", lib->libName().c_str());
  int& refcount = lib_refcounts_[lib];
  if (refcount == 1){
    lib_refcounts_.erase(lib);
    out_->debug(CALL_INFO, 1, 0, "OS %d will now drop events for %s\n",
                addr(), lib->libName().c_str());
    libs_.erase(lib->libName());
    //delete lib;
  } else {
    --refcount;
  }
}

bool
OperatingSystem::handleLibraryRequest(const std::string& name, Request* req)
{
  auto it = libs_.find(name);
  bool found = it != libs_.end();
  if (found){
    Library* lib = it->second;
    out_->debug(CALL_INFO, 1, 0, "delivering message to lib %s:%p: %s\n",
                name.c_str(), lib, toString(req).c_str());
    lib->incomingRequest(req);
  } else {
      out_->debug(CALL_INFO, 1, 0, "unable to deliver message to lib %s: %s\n",
                  name.c_str(), toString(req).c_str());
  }
  return found;
}

void
OperatingSystem::handleRequest(Request* req)
{
  //this better be an incoming event to a library, probably from off node
  Flow* libmsg = dynamic_cast<Flow*>(req);
  if (!libmsg) {
      out_->debug(CALL_INFO, 1, 0, "OperatingSystem::handle_event: got event %s instead of library event\n",
     toString(req).c_str());
  }

  bool found = handleLibraryRequest(libmsg->libname(), req);
  if (!found){
//    os_debug("delaying event to lib %s: %s",
//             libmsg->libname().c_str(), libmsg->toString().c_str());
    pending_library_request_[libmsg->libname()].push_back(req);
  }
}


} // namespace Hg
} // namespace SST
