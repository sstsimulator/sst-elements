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

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <sst/core/params.h>

#include <common/factory.h>
#include <common/output.h>
#include <common/util.h>
#include <common/errors.h>
#include <common/hg_printf.h>
#include <components/node.h>
//#include <sstmac/software/libraries/compute/lib_compute_inst.h>
//#include <sstmac/software/libraries/compute/lib_compute_time.h>
//#include <sstmac/software/libraries/compute/lib_compute_memmove.h>
#include <operating_system/process/app.h>
//#include <sstmac/software/api/api.h>
//#include <sstmac/software/process/backtrace.h>
//#include <sstmac/common/sstmac_env.h>
//#include <sstmac/dumpi_util/dumpi_meta.h>
//#include <sstmac/software/launch/job_launcher.h>
//#include <sstmac/software/launch/launch_event.h>
#include <operating_system/threading/thread_lock.h>
//#include <sstmac/common/sstmac_env.h>
//#include <sstmac/dumpi_util/dumpi_meta.h>
//#include <sprockit/statics.h>
//#include <sstmac/software/api/api.h>
//#include <sstmac/main/sstmac.h>
#include <operating_system/process/loadlib.h>
#include <components/operating_system.h>

#include <inttypes.h>
#include <dlfcn.h>

//static sprockit::NeedDeletestatics<sstmac::sw::UserAppCxxFullMain> del_app_statics;

//RegisterKeywords(
// { "host_compute_timer", "whether to use the time elapsed on the host machine in compute modeling" },
// { "min_op_cutoff", "the minimum number of operations in a compute before detailed modeling is perfromed" },
// { "notify", "whether the app should send completion notifications to job root" },
// { "globals_size", "the size of the global variable segment to allocate" },
// { "OMP_NUM_THREADS", "environment variable for configuring openmp" },
// { "exe", "an optional exe .so file to load for this app" },
//);

//MakeDebugSlot(app_compute);

void sstmac_app_loaded(int /*aid*/){}

extern "C" FILE* sstmac_stdout(){
  return SST::Hg::Thread::current()->parentApp()->stdOutFile();
}

extern "C" FILE* sstmac_stderr(){
  return SST::Hg::Thread::current()->parentApp()->stdErrFile();
}

namespace SST {
namespace Hg {

extern template SST::TimeConverter* HgBase<SST::Component>::time_converter_;

std::ostream& cout_wrapper(){
  return SST::Hg::Thread::current()->parentApp()->coutStream();
}

std::ostream& cerr_wrapper(){
  return SST::Hg::Thread::current()->parentApp()->cerrStream();
}

std::unique_ptr<std::map<std::string, App::main_fxn>> UserAppCxxFullMain::main_fxns_;
std::unique_ptr<std::map<std::string, App::empty_main_fxn>> UserAppCxxEmptyMain::empty_main_fxns_;
std::map<std::string, App::main_fxn>* UserAppCxxFullMain::main_fxns_init_ = nullptr;
std::map<std::string, App::empty_main_fxn>* UserAppCxxEmptyMain::empty_main_fxns_init_ = nullptr;
std::map<AppId, UserAppCxxFullMain::argv_entry> UserAppCxxFullMain::argv_map_;

std::map<int, App::dlopen_entry> App::exe_dlopens_;
std::map<std::string, App::dlopen_entry> App::api_dlopens_;
int App::app_rc_ = 0;

int
App::allocateTlsKey(destructor_fxn fxn)
{
  int next = next_tls_key_;
  tls_key_fxns_[next] = fxn;
  ++next_tls_key_;
  return next;
}

//static char* get_data_segment(SST::Params& params,
//                             const char* param_name, GlobalVariableContext& ctx)
//{
//  int allocSize = ctx.allocSize();
//  if (params.contains(param_name)){
//    allocSize = params.find<int>(param_name);
//    if (ctx.allocSize() != allocSize){
//      ctx.setAllocSize(allocSize);
//    }
//  }
//  if (allocSize != 0){
//    char* segment = new char[allocSize];
//    ::memcpy(segment, ctx.globalInit(), ctx.globalsSize());
//    return segment;
//  } else {
//    return nullptr;
//  }
//}


static thread_lock dlopen_lock;

void
App::lockDlopen(int aid)
{
  dlopen_entry& entry = exe_dlopens_[aid];
  entry.refcount++;
}

void
App::unlockDlopen(int aid)
{
  dlcloseCheck(aid);
}

void
App::lockDlopen_API(std::string api_name)
{
  dlopen_entry& entry = api_dlopens_[api_name];
  entry.refcount++;
}

void
App::unlockDlopen_API(std::string api_name)
{
  dlcloseCheck_API(api_name);
}

void
App::dlopenCheck(int aid, SST::Params& params,  bool check_name)
{
  // check params for apis
  std::vector<std::string> apis;
  if (params.contains("apis")){
    params.find_array("apis", apis);
  }
  else {
    apis.push_back("system:libsystemapi.so");
  }

  // parse apis and dlopen the libraries
  for (auto& str : apis){
    std::string name;
    std::string file;
    auto pos = str.find(":");
    if (pos == std::string::npos){
      name = str;
      file = str;
    } else {
      name = str.substr(0, pos);
      file = str.substr(pos + 1);
    }

    std::cerr << "loading " << name.c_str() << "API\n";
    dlopen_lock.lock();
    dlopen_entry& entry = api_dlopens_[name];
    entry.name = file;
    if (entry.refcount == 0 || !entry.loaded){
      entry.handle = loadExternLibrary(file, loadExternPathStr());
      entry.loaded = true;
    }

    ++entry.refcount;
    dlopen_lock.unlock();
  }

  // check params for exes and dlopen the libraries
  if (params.contains("exe")){
    dlopen_lock.lock();
    std::string libname = params.find<std::string>("exe");
    std::cerr << libname << std::endl;
    dlopen_entry& entry = exe_dlopens_[aid];
    entry.name = libname;
    if (entry.refcount == 0 || !entry.loaded){
      entry.handle = loadExternLibrary(libname, loadExternPathStr());
      entry.loaded = true;
    }

    if (check_name){
      void* name = dlsym(entry.handle, "exe_main_name");
      if (name){
        const char* str_name = (const char*) name;
        if (params.contains("name")){
          std::string given_name = params.find<std::string>("name");
          params.insert("label", given_name);
        }
        params.insert("name", str_name);
      }
    }

    ++entry.refcount;
    sstmac_app_loaded(aid);
    dlopen_lock.unlock();
  }
  else {
      std::cerr << "no exe in params\n";
  }

  UserAppCxxEmptyMain::aliasMains();
  UserAppCxxFullMain::aliasMains();
}

void
App::dlcloseCheck(int aid)
{
  dlopen_lock.lock();
  auto iter = exe_dlopens_.find(aid);
  if (iter != exe_dlopens_.end()){
    dlopen_entry& entry = iter->second;
    --entry.refcount;
    if (entry.refcount == 0 && entry.loaded){
      unloadExternLibrary(entry.handle);
      exe_dlopens_.erase(iter);
    }
  }
  dlopen_lock.unlock();
}

void
App::dlcloseCheck_API(std::string api_name)
{
  dlopen_lock.lock();
  auto iter = api_dlopens_.find(api_name);
  if (iter != api_dlopens_.end()){
    dlopen_entry& entry = iter->second;
    --entry.refcount;
    if (entry.refcount == 0 && entry.loaded){
      unloadExternLibrary(entry.handle);
      api_dlopens_.erase(iter);
    }
  }
  dlopen_lock.unlock();
}

//char*
//App::allocateDataSegment(bool tls)
//{
//  if (tls){
//    return get_data_segment(params_, "tls_size", GlobalVariable::tlsCtx);
//  } else {
//    return get_data_segment(params_, "globals_size", GlobalVariable::glblCtx);
//  }
//}

App::App(SST::Params& params, SoftwareId sid,
         OperatingSystem* os) :
  Thread(params, sid, os),
  params_(params),
//  compute_lib_(nullptr),
  next_tls_key_(0),
  min_op_cutoff_(0),
  globals_storage_(nullptr),
  notify_(true),
  rc_(0)
{
  out_ = std::make_unique<Output>(sprintf("application%d:", sid.app_), 1, 0, Output::STDOUT);
  out_->debug(CALL_INFO, 1, 0, "constructing");

  //globals_storage_ = allocateDataSegment(false); //not tls
  min_op_cutoff_ = params.find<long>("min_op_cutoff", 1000);
//  bool host_compute = params.find<bool>("host_compute_timer", false);
//  if (host_compute){
//    host_timer_ = new HostTimer;
//  }

  notify_ = params.find<bool>("notify", true);

  SST::Params env_params = params.get_scoped_params("env");
//  omp_contexts_.emplace_back();
//  omp_context& active = omp_contexts_.back();
//  active.max_num_subthreads = active.requested_num_subthreads =
//    env_params.find<int>("OMP_NUM_THREADS", 1);
//  active.level = 0;
//  active.num_threads = 1;

  std::set<std::string> keys = env_params.getKeys();
  for (auto& key : keys){
    env_[key] = env_params.find<std::string>(key);
  }

//  for (auto iter=os->env_begin(); iter != os->env_end(); ++iter){
//    auto my_iter = env_.find(iter->first);
//    if (my_iter == env_.end()){
//      //don't overwrite - app env taks precedence
//      env_[iter->first] = iter->second;
//    }
//  }

  std::vector<std::string> apis;
  if (params.contains("apis")){
    params.find_array("apis", apis);
  } else {
//    apis.push_back("mpi");
//    apis.push_back("sumi:mpi");
      apis.push_back("system");
  }

  for (auto& str : apis){
    std::string alias;
    std::string name;
    auto pos = str.find(":");
    if (pos == std::string::npos){
      name = str;
      alias = str;
    } else {
      alias = str.substr(0, pos);
      name = str.substr(pos + 1);
    }

    out_->debug(CALL_INFO, 1, 0, "checking %sAPI", name.c_str());

    auto iter = apis_.find(name);
    if (iter == apis_.end()){
      out_->debug(CALL_INFO, 1, 0, "loading %sAPI", name.c_str());
      SST::Params api_params = params.get_scoped_params(name);
      //SST::Component* comp = dynamic_cast<SST::Component*>(os->node());
      //if(!comp) sst_hg_abort_printf("APP can't dyncast to SST::Component*");
      API* api = SST::Hg::create<API>(
          "hg", name, api_params, this, os->node());
      apis_[name] = api;
    }
    apis_[alias] = apis_[name];
  }

  std::string stdout_str = params.find<std::string>("stdout", "stdout");
  std::string stderr_str = params.find<std::string>("stderr", "stderr");
  std::string cout_str = params.find<std::string>("cout", "cout");
  std::string cerr_str = params.find<std::string>("cerr", "cerr");

  if (stdout_str == "stdout"){
    stdout_ = stdout;
  } else if (stdout_str == "app"){
    std::string name = SST::Hg::sprintf("stdout.app%d", sid.app_);
    stdout_ = fopen(name.c_str(), "a");
  } else if (stdout_str == "rank"){
    std::string name = SST::Hg::sprintf("stdout.app%d.%d", sid.app_, sid.task_);
    stdout_ = fopen(name.c_str(), "w");
  } else {
    //this must be a filename
    stdout_ = fopen(stdout_str.c_str(), "a");
  }

  if (stderr_str == "stderr"){
    stderr_ = stderr;
  } else if (stdout_str == "app"){
    std::string name = SST::Hg::sprintf("stderr.app%d", sid.app_);
    stderr_ = fopen(name.c_str(), "a");
  } else if (stdout_str == "rank"){
    std::string name = SST::Hg::sprintf("stderr.app%d.%d", sid.app_, sid.task_);
    stderr_ = fopen(name.c_str(), "w");
  } else {
    //this must be a filename
    stderr_ = fopen(stderr_str.c_str(), "a");
  }

  if (cout_str == "cout"){
    //do nothing - by doing nothing we will return cout later
  } else if (cout_str == "app") {
    std::string name = SST::Hg::sprintf("cout.app%d", sid.app_);
    cout_.open(name);
  } else if (cout_str == "rank") {
    std::string name = SST::Hg::sprintf("cout.app%d.%d", sid.app_, sid.task_);
    cout_.open(name);
  } else {
    //this must be a filename
    cout_.open(cout_str);
  }

  if (cerr_str == "cerr"){
    //do nothing - by doing nothing we will return cout later
  } else if (cerr_str == "app") {
    std::string name = SST::Hg::sprintf("cerr.app%d", sid.app_);
    cerr_.open(name);
  } else if (cerr_str == "rank") {
    std::string name = SST::Hg::sprintf("cerr.app%d.%d", sid.app_, sid.task_);
    cerr_.open(cerr_str);
  } else {
    //this must be a filename
    cerr_.open(cerr_str);
  }
}

App::~App()
{
  /** These get deleted by unregister */
  //sprockit::delete_vals(apis_);
//  if (compute_lib_) delete compute_lib_;
  if (globals_storage_) delete[] globals_storage_;
}

std::ostream&
App::coutStream(){
  if (cout_.is_open()){
    return cout_;
  } else {
    return std::cout;
  }
}

std::ostream&
App::cerrStream(){
  if (cerr_.is_open()){
    return cerr_;
  } else {
    return std::cerr;
  }
}


int
App::putenv(char* input)
{
  sst_hg_abort_printf("app::putenv: not implemented - cannot put %d",
                    input);
  return 0;
}

int
App::setenv(const std::string &name, const std::string &value, int overwrite)
{
  if (overwrite){
    env_[name] = value;
  } else {
    auto iter = env_.find(name);
    if (iter == env_.end()){
      env_[name] = value;
    }
  }
  return 0;
}

char*
App::getenv(const std::string &name) const
{
  char* my_buf = const_cast<char*>(env_string_);
  auto iter = env_.find(name);
  if (iter == env_.end()){
    return nullptr;
  } else {
    auto& val = iter->second;
    if (val.size() >= sizeof(env_string_)){
      sst_hg_abort_printf("Environment variable %s=%s is too long - need less than %d",
                        name.c_str(), val.c_str(), int(val.size()));
    }
    ::strcpy(my_buf, val.data());
  }
  //ugly but necessary
  return my_buf;
}

//LibComputeMemmove*
//App::computeLib()
//{
//  if(!compute_lib_) {
//    compute_lib_ = new LibComputeMemmove(params_, sid_, os_);
//  }
//  return compute_lib_;
//}

void
App::deleteStatics()
{
}

void
App::cleanup()
{
  //okay, the app is dying
  //it may be that we have subthreads that are still active
  //all of these subthreads must be cancelled and never start again
  for (auto& pair : subthreads_){
    pair.second->cancel();
  }
  subthreads_.clear();

  Thread::cleanup();
}

//void
//App::sleep(TimeDelta time)
//{
//  computeLib()->sleep(time);
//}

//void
//App::compute(TimeDelta time)
//{
//  computeLib()->compute(time);
//}

//void
//App::computeInst(ComputeEvent* cmsg)
//{
//  computeLib()->computeInst(cmsg);
//}

//void
//App::computeLoop(uint64_t num_loops,
//  int nflops_per_loop,
//  int nintops_per_loop,
//  int bytes_per_loop)
//{
//  computeLib()->LibComputeInst::computeLoop(
//          num_loops, nflops_per_loop, nintops_per_loop, bytes_per_loop);
//}

//void
//App::computeDetailed(uint64_t flops, uint64_t nintops, uint64_t bytes, int nthread)
//{
//  static const uint64_t overflow = 18006744072479883520ull;
//  if (flops > overflow || bytes > overflow){
//    spkt_abort_printf("flops/byte counts for compute overflowed");
//  }
//  if ((flops+nintops) < min_op_cutoff_){
//    return;
//  }

//  debug_printf(sprockit::dbg::app_compute,
//               "Rank %d for app %d: detailed compute for flops=%" PRIu64 " intops=%" PRIu64 " bytes=%" PRIu64,
//               sid_.task_, sid_.app_, flops, nintops, bytes);

//  computeLib()->computeDetailed(flops, nintops, bytes, nthread);
//}

//void
//App::computeBlockRead(uint64_t bytes)
//{
//  computeLib()->read(bytes);
//}

//void
//App::computeBlockWrite(uint64_t bytes)
//{
//  computeLib()->write(bytes);
//}

SST::Params
App::getParams()
{
  return OperatingSystem::currentThread()->parentApp()->params();
}

//void
//App::computeBlockMemcpy(uint64_t bytes)
//{
//  computeLib()->copy(bytes);
//}

API*
App::getPrebuiltApi(const std::string &name)
{
  auto iter = apis_.find(name);
  if (iter == apis_.end()){
    sst_hg_abort_printf("API %s was not included in launch params for app %d",
                name.c_str(), aid());
  }
  return iter->second;
}

void
App::run()
{
//  CallGraphAppend(main);
//  os_->incrementAppRefcount();
  endAPICall(); //this initializes things, "fake" api call at beginning
  rc_ = skeletonMain();
  //we are ending but perform the equivalent
  //to a start api call to flush any compute
  startAPICall();

  std::set<API*> unique;
  //because of aliasing...
  for (auto& pair : apis_){
    unique.insert(pair.second);
  }
  apis_.clear();
  for (API* api : unique) delete api;

  //now we have to send a message to the job launcher to let it know we are done
//  os_->decrementAppRefcount();
  //for now assume that the application has finished with a barrier - which is true of like everything
//  if (sid_.task_ == 0 && notify_){
//    int launchRoot = os_->node()->launchRoot();
//    JobStopRequest* lev = new JobStopRequest(os_->node()->allocateUniqueId(),
//                                             sid_.app_, unique_name_, launchRoot, os_->addr());
//    os_->nicCtrlIoctl()(lev);
//  }
//  TaskMapping::removeGlobalMapping(sid_.app_, unique_name_);
//  ThreadInfo::deregisterUserSpaceVirtualThread(stack_);
  dlcloseCheck();

  app_rc_ = rc_;
}

void
App::addSubthread(Thread *thr)
{
  if (thr->threadId() == Thread::main_thread){
    thr->initId();
  }
  subthreads_[thr->threadId()] = thr;
}

Thread*
App::getSubthread(uint32_t id)
{
  auto it = subthreads_.find(id);
  if (it==subthreads_.end()){
    sst_hg_throw_printf(SST::Hg::ValueError,
      "unknown thread id %u",
      id);
  }
  return it->second;
}

void
App::removeSubthread(uint32_t id)
{
  subthreads_.erase(id);
}

void
App::removeSubthread(Thread *thr)
{
  subthreads_.erase(thr->threadId());
}

bool
App::eraseMutex(int id)
{
  return os_->eraseMutex(id);
}

mutex_t*
App::getMutex(int id)
{
  return os_->getMutex(id);
}

int
App::allocateMutex()
{
  return os_->allocateMutex();
}

bool
App::eraseCondition(int id)
{
  return os_->eraseCondition(id);
}

int
App::allocateCondition()
{
  // this needs to be on the OS in case
  // we have a process shared thing
  return os_->allocateCondition();
}

condition_t*
App::getCondition(int id)
{
  // this needs to be on the OS in case
  // we have a process shared thing
  return os_->getCondition(id);
}

void
UserAppCxxFullMain::deleteStatics()
{
  for (auto& pair : argv_map_){
    argv_entry& entry = pair.second;
    char* main_buffer = entry.argv[0];
    delete[] main_buffer;
    delete[] entry.argv;
  }
  argv_map_.clear();
  main_fxns_ = nullptr;
}

UserAppCxxFullMain::UserAppCxxFullMain(SST::Params& params, SoftwareId sid,
                                       OperatingSystem* os) :
  App(params, sid, os)
{
  if (!main_fxns_){
    //because of the awful XC8 bug
    main_fxns_ = std::unique_ptr<std::map<std::string,main_fxn>>(main_fxns_init_);
    main_fxns_init_ = nullptr;
  }

  std::string name = params.find<std::string>("name");
  std::map<std::string, main_fxn>::iterator it = main_fxns_->find(name);
  if (it == main_fxns_->end()){
    sst_hg_throw_printf(SST::Hg::ValueError,
                     "no user app with the name %s registered",
                     name.c_str());
  }
  fxn_ = it->second;
}

void
UserAppCxxFullMain::aliasMains()
{
  //std::cerr << "UserAppCxxFullMain::aliasMains()\n";
  static thread_lock lock;
  lock.lock();
  if (!main_fxns_){
    main_fxns_ = std::unique_ptr<std::map<std::string, App::main_fxn>>(main_fxns_init_);
    main_fxns_init_ = nullptr;
  }
  auto* lib = App::getBuilderLibrary("hg");
  //std::cerr << "lib: " << lib << "\n";
  //std::cerr << "got Builder Library\n";
  if (main_fxns_){
    //std::cerr << "have main_fxns_\n";
    for (auto& pair : *main_fxns_){
    auto* builder = lib->getBuilder("UserAppCxxFullMain");
    //std::cerr << "adding " << pair.first << " builder\n";
    lib->addBuilder(pair.first, builder);
    }
  }
  else {
      //std::cerr << "no main_fxns_\n";
    }
  lock.unlock();
}

void
UserAppCxxFullMain::registerMainFxn(const char *name, App::main_fxn fxn)
{
  //std::cerr << "registering full main " << name << "\n";
  if (main_fxns_){  //already passed static init
    (*main_fxns_)[name] = fxn; 
  } else {
    if (!main_fxns_init_){
      main_fxns_init_ = new std::map<std::string, main_fxn>;
    }
    (*main_fxns_init_)[name] = fxn;
  }
}

void
UserAppCxxEmptyMain::aliasMains()
{
  static thread_lock lock;
  lock.lock();
  if (!empty_main_fxns_){
    empty_main_fxns_ = std::unique_ptr<std::map<std::string, App::empty_main_fxn>>(empty_main_fxns_init_);
    empty_main_fxns_init_ = nullptr;
  }
  auto* lib = App::getBuilderLibrary("hg");
  if (empty_main_fxns_){
    for (auto& pair : *empty_main_fxns_){
      auto* builder = lib->getBuilder("UserAppCxxEmptyMain");
      lib->addBuilder(pair.first, builder);
    }
  }
  lock.unlock();
}

void
UserAppCxxFullMain::initArgv(argv_entry& entry)
{
  std::string appname = params_.find<std::string>("name");
  std::string argv_str = params_.find<std::string>("argv", "");
  std::deque<std::string> argv_param_dq;
//  pst::BasicStringTokenizer::tokenize(argv_str, argv_param_dq, std::string(" "));
  int argc = argv_param_dq.size() + 1;
  char* argv_buffer = new char[256 * argc];
  char* argv_buffer_ptr = argv_buffer;
  char** argv = new char*[argc+1];
  argv[0] = argv_buffer;
  ::strcpy(argv_buffer, appname.c_str());
  int i=1;
  argv_buffer_ptr += appname.size() + 1;
  for (auto& src_str : argv_param_dq){
    ::strcpy(argv_buffer_ptr, src_str.c_str());
    argv[i] = argv_buffer_ptr;
    //increment pointer for next strcpy
    argv_buffer_ptr += src_str.size() + 1; //+1 for null terminator
    ++i;
  }
  argv[argc] = nullptr; //missing nullptr - Issue #269
  entry.argc = argc;
  entry.argv = argv;
}

int
UserAppCxxFullMain::skeletonMain()
{
  static thread_lock argv_lock;
  argv_lock.lock();
  argv_entry& entry = argv_map_[sid_.app_];
  if (entry.argv == 0){
    initArgv(entry);
  }
  argv_lock.unlock();
  int rc = (*fxn_)(entry.argc, entry.argv);
  return rc;
}

UserAppCxxEmptyMain::UserAppCxxEmptyMain(SST::Params& params, SoftwareId sid,
                                         OperatingSystem* os) :
  App(params, sid, os)
{
  if (!empty_main_fxns_){
    empty_main_fxns_ = std::unique_ptr<std::map<std::string, App::empty_main_fxn>>(empty_main_fxns_init_);
    empty_main_fxns_init_ = nullptr;
  }

  std::string name = params.find<std::string>("name");
  std::map<std::string, empty_main_fxn>::iterator it = empty_main_fxns_->find(name);
  if (it == empty_main_fxns_->end()){
    sst_hg_throw_printf(SST::Hg::ValueError,
                     "no user app with the name %s registered",
                     name.c_str());
  }
  fxn_ = it->second;
}

void
UserAppCxxEmptyMain::registerMainFxn(const char *name, App::empty_main_fxn fxn)
{
  std::cerr << "registering empty main " << name << "\n";
  if (empty_main_fxns_){ //already cleared static init
    (*empty_main_fxns_)[name] = fxn;
  } else { 
    if (!empty_main_fxns_init_){
      empty_main_fxns_init_ = new std::map<std::string, empty_main_fxn>;
    }

    (*empty_main_fxns_init_)[name] = fxn;
  }

#if 0
  auto* lib = App::getBuilderLibrary("macro");
#if SSTMAC_INTEGRATED_SST_CORE
  using builder_t = SST::ELI::DerivedBuilder<App,UserAppCxxFullMain,SST::Params&,SoftwareId,OperatingSystem*>;
  lib->addBuilder(name, new builder_t);
#else
  using builder_t = sprockit::DerivedBuilder<App,UserAppCxxFullMain,SST::Params&,SoftwareId,OperatingSystem*>;
  lib->addBuilder(name, std::unique_ptr<builder_t>(new builder_t));
#endif
#endif
}

int
UserAppCxxEmptyMain::skeletonMain()
{
  return (*fxn_)();
}

//void computeTime(double tsec)
//{
//  Thread* t = OperatingSystem::currentThread();
//  App* a = safe_cast(App, t,
//     "cannot cast current thread to app in compute_time function");
//  a->compute(TimeDelta(tsec));
//}

}
}

namespace sstmac {

std::ostream& cout_wrapper(){
  return std::cout;
}

std::ostream& cerr_wrapper(){
  return std::cerr;
}

}
