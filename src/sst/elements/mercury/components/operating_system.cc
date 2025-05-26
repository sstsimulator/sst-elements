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
#include <mercury/components/node_CL.h>
#include <mercury/operating_system/launch/app_launcher.h>
#include <mercury/operating_system/libraries/unblock_event.h>
#include <mercury/operating_system/process/app.h>
#include <mercury/operating_system/process/thread_id.h>
#include <mercury/operating_system/threading/thread_lock.h>
#include <mercury/operating_system/threading/stack_alloc.h>
#include <sst/core/eli/elementbuilder.h>
#include <sst/core/params.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sst/core/component.h> // or
#include <sst/core/subcomponent.h> // or
#include <sst/core/componentExtension.h>
// #ifndef HGHOLDERLIB
// #define HGHOLDERLIB
// #define MERCURY_LIB hg
//   #include <mercury/common/loader.h>
// #undef MERCURY_LIB
// #endif
extern "C" {
void* sst_hg_nullptr = nullptr;
void* sst_hg_nullptr_send = nullptr;
void* sst_hg_nullptr_recv = nullptr;
void* sst_hg_nullptr_range_max = nullptr;
}
static uintptr_t sst_hg_nullptr_range = 0;

namespace SST {
namespace Hg {

extern template class  HgBase<SST::Component>;
extern template class  HgBase<SST::SubComponent>;
extern template SST::TimeConverter HgBase<SST::SubComponent>::time_converter_;

// #if SST_HG_USE_MULTITHREAD
std::vector<OperatingSystem*> OperatingSystem::active_os_;
// #else
// OperatingSystem* OperatingSystem::active_os_ = nullptr;
// #endif

std::map<std::string,SST::Hg::loaderAPI*> OperatingSystem::loaders_;

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

OperatingSystem::OperatingSystem(SST::ComponentId_t id, SST::Params& params, NodeBase* parent) :
  OperatingSystemBase(id,params),
  node_(parent),
  des_context_(nullptr),
  next_condition_(0),
  next_mutex_(0),
  params_(params)
{
  TimeDelta::initStamps(TimeDelta::ASEC_PER_TICK);
  
  if (active_os_.size() == 0){
    RankInfo num_ranks = getNumRanks();
    active_os_.resize(num_ranks.thread);
  }
  // Need to load hg first before loading other libraries.
  //requireLibrary("hg");
  /* Neil B
   * Adding code the ensure the libraries are loaded across all ranks. 
   * The requireLibrary commands ensure that the libraries are loaded in the correct order
   * Loading the holder subcomponents ensures that the libraries are actually loaded.
   * Through testing with SST_CORE_DL_VERBOSE=1 requireLibrary doesn't enforce the loading of all libraries.
   */

  // std::vector<std::string> loads;
  // params.find_array<std::string>("app1.loads", loads);
  // // Load the libraries in app1.loads
  // for (const std::string& lib : loads) {
  //     //std::cerr << "requiring " << myLib << std::endl;
  //     //requireLibrary(myLib);
  //     std::cerr << "loading " << lib << std::endl;
  //     holder = loadAnonymousSubComponent<holderSubComponentAPI>(lib, "holder", 0, 0, params);
  // }

  // Load the app code itself. The library itself needs to be in a location that sst-core can find.
  // std::string name = params_.find<std::string>("app1.name");
  // requireLibrary(name);
  // holder = loadAnonymousSubComponent<holderSubComponentAPI>(name, "holder", 0, 0, params);

  my_addr_ = node_->addr();

  next_outgoing_id_.src_node = my_addr_;
  next_outgoing_id_.msg_num = 0;
  verbose_ = params.find<unsigned int>("verbose", 1);
  out_ = std::unique_ptr<SST::Output>(
      new SST::Output(sprintf("Node%d:HgOperatingSystem:", my_addr_), verbose_, 0,
                      Output::STDOUT));
  out_->debug(CALL_INFO, 1, 0, "constructing\n");

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

  if (!time_converter_){
      time_converter_ = SST::BaseComponent::getTimeConverter(tickIntervalString());
    }

  loadCheck(params_,*this);

  // Configure self link to handle event timing
  selfEventLink_ = configureSelfLink("self", time_converter_, new Event::Handler2<Hg::OperatingSystem,&OperatingSystem::handleEvent>(this));
  assert(selfEventLink_);
  selfEventLink_->setDefaultTimeBase(time_converter_);

  StackAlloc::init(params);
  initThreading(params);
}

OperatingSystem::~OperatingSystem()
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
OperatingSystem::setup() {
  app_launcher_ = new AppLauncher(this, npernode_);
  addLaunchRequests(params_);
  for (auto r : requests_)
    selfEventLink_->send(r);
}

static thread_lock loader_lock;

void
OperatingSystem::loadCheck(SST::Params& params, SST::Hg::OperatingSystem& me) {
  /* Neil B
   * Adding code the ensure the libraries are loaded across all ranks. 
   * Loading the holder subcomponents ensures that the libraries are actually loaded.
   * Through testing with SST_CORE_DL_VERBOSE=1 requireLibrary doesn't enforce the loading of all libraries.
   */
  std::vector<std::string> loads;
  params.find_array<std::string>("app1.loads", loads);
  loader_lock.lock();
  // Load the libraries in app1.loads
  int nslot = 0;
  for (const std::string& lib : loads) {
    if (loaders_.find(lib) == loaders_.end() || loaders_[lib] == nullptr) {
      me.requireLibrary(lib);
      std::string loader(lib);
      loader += ".loader";
      loaders_[lib] = me.loadAnonymousSubComponent<SST::Hg::loaderAPI>(loader.c_str(), "loader", nslot, ComponentInfo::SHARE_NONE, params);
      ++nslot;
      if (loaders_[lib] == nullptr) {
        std::cerr << "WARNING: " << lib << " did not succeed loading " << loader << ", but this may not be fatal\n";
      }
      else std::cerr << "SUCCESS: loaded " << lib << std::endl;
    }
  }
  loader_lock.unlock();
}

void
OperatingSystem::initThreading(SST::Params& params)
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
            parent->newTlsStorage());
    }
  running_threads_[t->tid()] = t;
}

/* Event handler
   * Incoming events are scanned and deleted
   * Record if the event received is the last one our neighbor will send
   */
void
OperatingSystem::handleEvent(SST::Event *ev) {
  if (auto *req = dynamic_cast<AppLaunchRequest *>(ev)) {
    Params params = req->params();
    out_->debug(CALL_INFO, 1, 0, "app name: %s\n",
                  params.find<std::string>("name").c_str());
    app_launcher_->incomingRequest(req);
  } else {
    sst_hg_abort_printf("Error! Bad Event Type received by %s!\n",
                        getName().c_str());
  }
}


std::function<void(NetworkMessage*)>
OperatingSystem::nicDataIoctl()
{
  return node_->nic()->dataIoctl();
}

std::function<void(NetworkMessage*)>
OperatingSystem::nicCtrlIoctl()
{
  return node_->nic()->ctrlIoctl();
}

void
OperatingSystem::addLaunchRequests(SST::Params& params)
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

      App::lockDlopen(aid);
    } else {
      keep_going = false;
    }
    ++aid;
  }
}

void 
OperatingSystem::startApp(App *theapp,
                          const std::string & /*unique_name*/) {
  out_->debug(CALL_INFO, 1, 0, "starting app %d:%d on physical thread %d\n",
                int(theapp->tid()), int(theapp->aid()), threadId());
  // this should be called from the actual thread running it
  initThreading(params_);
  startThread(theapp);
}

void
OperatingSystem::joinThread(Thread* t)
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
OperatingSystem::scheduleThreadDeletion(Thread* thr)
{
  //JJW 11/6/2014 This here is weird
  //The thread has run out of work and is terminating
  //However, because of weird thread swapping the DES thread
  //might still operate on the thread... we need to delay the delete
  //until the DES thread has completely finished processing its current event
  selfEventLink_->send(new DeleteThreadEvent(thr));
  node_->endSim();
}

void
OperatingSystem::completeActiveThread()
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
                thr_todelete->threadId(), threadId());
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

  out_->debug(CALL_INFO, 1, 0,
                "switching to context %d on physical thread %d\n",
                tothread->threadId(), threadId());
  if (active_thread_ == blocked_thread_) {
    blocked_thread_ = nullptr;
  }
  active_thread_ = tothread;
  activeOs() = this;
  tothread->context()->resumeContext(des_context_);
  out_->debug(CALL_INFO, 1, 0,
                "switched back from context %d to main thread %d\n",
                tothread->threadId(), threadId());
  /** back to main thread */
  active_thread_ = nullptr;
}

void
OperatingSystem::block()
{
  Timestamp before = now();
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
                active_thread_->threadId(), threadId());
  blocked_thread_ = active_thread_;
  active_thread_ = nullptr;
  old_context->pauseContext(des_context_);

//  while(hold_for_gdb_){
//    sst_gdb_swap();  //do nothing - this is only for gdb purposes
//  }

  //restore state to indicate this thread and this OS are active again
  activeOs() = this;
  out_->debug(CALL_INFO, 1, 0, "resuming context %d on physical thread %d\n",
                active_thread_->threadId(), threadId());
  active_thread_ = old_thread;
  active_thread_->incrementBlockCounter();

   //collect any statistics associated with the elapsed time
  Timestamp after = now();
  TimeDelta elapsed = after - before;
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

void
OperatingSystem::blockTimeout(TimeDelta delay)
{
  sendDelayedExecutionEvent(delay, new TimeoutEvent(this, active_thread_));
  block();
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

// Library*
// OperatingSystem::currentEventLibrary(const std::string &name)
// {
//   return currentOs()->eventLibrary(name);
// }

EventLibrary*
OperatingSystem::eventLibrary(const std::string& name) const
{
  auto it = libs_.find(name);
  if (it == libs_.end()) {
    return nullptr;
  } else {
    return it->second;
  }
}

void
OperatingSystem::registerEventLib(EventLibrary* lib)
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
      sendExecutionEventNow(newCallback(lib, &EventLibrary::incomingRequest, req));
    }
    pending_library_request_.erase(iter);
  }
}

void 
OperatingSystem::unregisterEventLib(EventLibrary *lib) {
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

bool
OperatingSystem::handleEventLibraryRequest(const std::string& name, Request* req)
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

void
OperatingSystem::handleRequest(Request* req)
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

} // namespace Hg
} // namespace SST
