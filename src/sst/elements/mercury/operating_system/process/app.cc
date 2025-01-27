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

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <sst/core/params.h>

#include <mercury/common/factory.h>
#include <sst/core/eli/elementbuilder.h>
#include <mercury/common/output.h>
#include <mercury/common/util.h>
#include <mercury/common/errors.h>
#include <mercury/common/hg_printf.h>
#include <mercury/components/node.h>
#include <mercury/operating_system/process/app.h>
#include <mercury/operating_system/threading/thread_lock.h>
#include <mercury/operating_system/process/loadlib.h>
#include <mercury/components/operating_system.h>
#include <inttypes.h>
#include <dlfcn.h>

void sst_hg_app_loaded(int /*aid*/){}

extern "C" FILE* sst_hg_stdout(){
  return SST::Hg::Thread::current()->parentApp()->stdOutFile();
}

extern "C" FILE* sst_hg_stderr(){
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
App::lockDlopen_Library(std::string api_name)
{
  dlopen_entry& entry = api_dlopens_[api_name];
  entry.refcount++;
}

void
App::unlockDlopen_Library(std::string api_name)
{
  dlcloseCheck_Library(api_name);
}

void
App::dlopenCheck(int aid, SST::Params& params,  bool check_name)
{
  std::vector<std::string> libs;
  if (params.contains("libraries")){
    params.find_array<std::string>("libraries", libs);
  }
  else {
    libs.push_back("SystemLibrary:libsystemlibrary.so");
  }

  // parse libs and dlopen them
  for (auto& str : libs){
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
    sst_hg_app_loaded(aid);
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
App::dlcloseCheck_Library(std::string api_name)
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
  os_(os),
  next_tls_key_(0),
  min_op_cutoff_(0),
  globals_storage_(nullptr),
  notify_(true),
  rc_(0)
{
  unsigned int verbose = params.find<unsigned int>("verbose", 0);
  out_ = std::unique_ptr<SST::Output>(
      new SST::Output("app:", verbose, 0, Output::STDOUT));

  //globals_storage_ = allocateDataSegment(false); //not tls
  min_op_cutoff_ = params.find<long>("min_op_cutoff", 1000);

  notify_ = params.find<bool>("notify", true);

  SST::Params env_params = params.get_scoped_params("env");

  std::set<std::string> keys = env_params.getKeys();
  for (auto& key : keys){
    env_[key] = env_params.find<std::string>(key);
  }

  std::vector<std::string> libs;
  if (params.contains("libraries")){
    params.find_array("libraries", libs);
  } else {
      libs.push_back("SystemLibrary:libsystemlibrary.so");
  }

  for (auto& str : libs){
    std::string alias;
    std::string name;
    auto pos = str.find(":");
    if (pos == std::string::npos){
      name = str;
      alias = str;
    } else {
      name = str.substr(0, pos);
      alias = str.substr(pos + 1);
    }

    out_->debug(CALL_INFO, 1, 0, "checking %s\n", name.c_str());

    if (name != "SimTransport") {
      auto iter = apis_.find(name);
      if (iter == apis_.end()){
        out_->debug(CALL_INFO, 1, 0, "loading %s\n", name.c_str());
        // It'll take some work to make this possible in pymerlin
        //SST::Params lib_params = params.get_scoped_params(name);
        //lib_params.print_all_params(std::cerr);
        // Library* lib = SST::Hg::create<Library>(
        //       "hg", name, lib_params, this);
        Library *lib =
            SST::Hg::create<Library>("hg", name, params, this);
        apis_[name] = lib;
      }
      apis_[alias] = apis_[name];
    }
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
  if (globals_storage_) delete[] globals_storage_;
}

void 
App::addAPI(std::string name, Library* lib) {
  auto iter = apis_.find(name);
  if (iter == apis_.end()) {
    apis_[name] = lib;
  }
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

// void
// App::sleep(TimeDelta time)
// {
//   os_->blockTimeout(time);
// }

// void
// App::compute(TimeDelta time)
// {
//   os_->blockTimeout(time);
// }

SST::Params
App::getParams()
{
  return OperatingSystem::currentThread()->parentApp()->params();
}

Library*
App::getLibrary(const std::string &name)
{
  auto iter = apis_.find(name);
  if (iter == apis_.end()){
    sst_hg_abort_printf("Library %s not found for app %d",
                name.c_str(), aid());
  }
  return iter->second;
}

void
App::run()
{
  endLibraryCall(); //this initializes things, "fake" api call at beginning
  rc_ = skeletonMain();
  //we are ending but perform the equivalent
  //to a start api call to flush any compute
  startLibraryCall();

  std::set<Library*> unique;
  //because of aliasing...
  for (auto& pair : apis_){
    unique.insert(pair.second);
  }
  apis_.clear();
  for (Library* api : unique) delete api;

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
  static thread_lock lock;
  lock.lock();
  if (!main_fxns_){
    main_fxns_ = std::unique_ptr<std::map<std::string, App::main_fxn>>(main_fxns_init_);
    main_fxns_init_ = nullptr;
  }
  auto* lib = App::getBuilderLibrary("hg");
  if (main_fxns_){
    for (auto& pair : *main_fxns_){
    auto* builder = lib->getBuilder("UserAppCxxFullMain");
    lib->addBuilder(pair.first, builder);
    }
  }
  else {
      std::cerr << "no main_fxns_\n";
    }
  lock.unlock();
}

void
UserAppCxxFullMain::registerMainFxn(const char *name, App::main_fxn fxn)
{
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
  if (empty_main_fxns_){ //already cleared static init
    (*empty_main_fxns_)[name] = fxn;
  } else { 
    if (!empty_main_fxns_init_){
      empty_main_fxns_init_ = new std::map<std::string, empty_main_fxn>;
    }

    (*empty_main_fxns_init_)[name] = fxn;
  }
}

int
UserAppCxxEmptyMain::skeletonMain()
{
  return (*fxn_)();
}

} // end of namespace Hg
} // end of namespace SST
