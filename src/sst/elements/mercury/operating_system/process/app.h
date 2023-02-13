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

#include <sst/core/params.h>

#include <common/component.h>
#include <common/factory.h>
#include <components/operating_system.h>
#include <operating_system/process/thread.h>
#include <operating_system/process/mutex.h>
#include <operating_system/libraries/api.h>

#include <fstream>

#ifdef sleep
#if sleep == sstmac_sleep
#define refactor_sleep_macro
#undef sleep
#endif
#endif

namespace SST {
namespace Hg {

/**
 * The app derived class adds to the thread base class by providing
 * facilities to allow applications to simulate computation.
 * Messaging models are supported through an api class,
 * which are stored by the app
 */
class App : public Thread
{
 public:
  SST_ELI_DECLARE_BASE(App)
  SST_ELI_DECLARE_DEFAULT_INFO()
  SST_ELI_DECLARE_CTOR(SST::Params&,SoftwareId,OperatingSystem*)

  typedef void (*destructor_fxn)(void*);

  typedef int (*main_fxn)(int argc, char** argv);
  typedef int (*empty_main_fxn)();

  int allocateTlsKey(destructor_fxn fnx);

  static SST::Params getParams();

  App* parentApp() const override {
    return const_cast<App*>(this);
  }

  bool isMainThread() const override {
    return true;
  }

  static void deleteStatics();

//  void sleep(TimeDelta time);

//  void compute(TimeDelta time);

//  void computeInst(ComputeEvent* cmsg);

//  void computeLoop(uint64_t num_loops,
//    int nflops_per_loop,
//    int nintops_per_loop,
//    int bytes_per_loop);

//  void computeBlockRead(uint64_t bytes);

//  void computeBlockWrite(uint64_t bytes);

//  void computeBlockMemcpy(uint64_t bytes);

//  LibComputeMemmove* computeLib();

  ~App() override;

  void cleanup() override;

  /**
   * @brief skeleton_main
   * @return The return code that would be returned by a main
   */
  virtual int skeletonMain() = 0;

  void run() override;

  int rc() const {
    return rc_;
  }

  SST::Params& params() {
    return params_;
  }

  char* getenv(const std::string& name) const;

  int putenv(char* input);

  int setenv(const std::string& name, const std::string& value, int overwrite);

  /**
   * Let a parent application know about the existence of a subthread
   * If thread does not have an initialized ID, a unique ID is allocated for the thread
   * Can be called from a constructor. This method does NOT throw.
   * @param thr
   */
  void addSubthread(Thread* thr);

  /**
   * Let a parent application know a subthread has finished.
   * This completely erases the thread. There will be no record of this thread after calling this function.
   * @param thr A thread with initialized ID
   */
  void removeSubthread(Thread* thr);

  void removeSubthread(uint32_t thr_id);

  /**
   * @brief get_subthread
   * @param id
   * @return
   */
  Thread* getSubthread(uint32_t id);

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

  /**
   * Allocate a unique ID for a condition variable
   * @return The unique ID
   */
  int allocateCondition();

  /**
   * Fetch a condition object corresponding to the ID
   * @param id
   * @return The condition object corresponding to the ID. Return NULL if not condition is found.
   */
  condition_t* getCondition(int id);

  bool eraseCondition(int id);

  void* globalsStorage() const {
    return globals_storage_;
  }

//  void* newTlsStorage() {
//    return allocateDataSegment(true);
//  }

  const std::string& uniqueName() const {
    return unique_name_;
  }

  void setUniqueName(const std::string& name) {
    unique_name_ = name;
  }

  static void dlopenCheck(int aid, SST::Params& params, bool check_name = true);

  static void dlcloseCheck(int aid);

  static void lockDlopen(int aid);

  static void unlockDlopen(int aid);

  static void dlcloseCheck_API(std::string api_name);

  static void lockDlopen_API(std::string api_name);

  static void unlockDlopen_API(std::string api_name);

  static int appRC(){
    return app_rc_;
  }

  FILE* stdOutFile(){
    return stdout_;
  }

  FILE* stdErrFile(){
    return stderr_;
  }

  std::ostream& coutStream();
  std::ostream& cerrStream();

 protected:
  friend class Thread;

  App(SST::Params& params, SoftwareId sid,
      OperatingSystem* os);

  SST::Params params_;

 private:
  API* getPrebuiltApi(const std::string& name);

  void dlcloseCheck(){
    dlcloseCheck(aid());
  }

  char* allocateDataSegment(bool tls);

  void computeDetailed(uint64_t flops, uint64_t intops, uint64_t bytes, int nthread);

//  LibComputeMemmove* compute_lib_;
  std::string unique_name_;

  int next_tls_key_;
  uint64_t min_op_cutoff_;

  std::map<long, Thread*> subthreads_;
  std::map<int, destructor_fxn> tls_key_fxns_;
  //  these can alias - so I can't use unique_ptr
  std::map<std::string, API*> apis_;
  std::map<std::string,std::string> env_;

  char env_string_[64];

  char* globals_storage_;

  bool notify_;

  int rc_;

  struct dlopen_entry {
    void* handle;
    int refcount;
    bool loaded;
    std::string name;
    dlopen_entry() : handle(nullptr), refcount(0), loaded(false) {}
  };

  static std::map<int, dlopen_entry> exe_dlopens_;
  static std::map<std::string, dlopen_entry> api_dlopens_;

  static int app_rc_;

  std::ofstream cout_;
  std::ofstream cerr_;
  FILE* stdout_;
  FILE* stderr_;
  std::unique_ptr<SST::Output> out_;
};


class UserAppCxxFullMain : public App
{
 public:
  SST_ELI_REGISTER_DERIVED(App,
    UserAppCxxFullMain,
    "hg",
    "UserAppCxxFullMain",
    SST_ELI_ELEMENT_VERSION(1,0,0),
    "an app that runs main(argc,argv)")

  UserAppCxxFullMain(SST::Params& params, SoftwareId sid,
                     OperatingSystem* os);

  static void registerMainFxn(const char* name, App::main_fxn fxn);

  int skeletonMain() override;

  static void deleteStatics();

  static void aliasMains();

  struct argv_entry {
    char** argv;
    int argc;
    argv_entry() : argv(0), argc(0) {}
  };

 private:
  void initArgv(argv_entry& entry);

  static std::unique_ptr<std::map<std::string, App::main_fxn>> main_fxns_;
  static std::map<std::string, App::main_fxn>* main_fxns_init_;
  static std::map<AppId, argv_entry> argv_map_;
  App::main_fxn fxn_;

};

class UserAppCxxEmptyMain : public App
{
 public:
  SST_ELI_REGISTER_DERIVED(App,
    UserAppCxxFullMain,
    "hg",
    "UserAppCxxFullMain",
    SST_ELI_ELEMENT_VERSION(1,0,0),
    "an app that runs main()")

  UserAppCxxEmptyMain(SST::Params& params, SoftwareId sid,
                          OperatingSystem* os);

  static void registerMainFxn(const char* name, App::empty_main_fxn fxn);

  static void aliasMains();

  int skeletonMain() override;

 private:
  //to work around static init bugs for older compilers
  //I have to init with a raw pointer - then transfer ownershipt to a unique_ptr
  //that cleans up at the end
  static std::unique_ptr<std::map<std::string, App::empty_main_fxn>> empty_main_fxns_;
  static std::map<std::string, App::empty_main_fxn>* empty_main_fxns_init_;
  App::empty_main_fxn fxn_;

};

/** utility function for computing stuff */
void computeTime(double tsec);

} // end of namespace Hg
} // end of namespace SST

#ifdef refactor_sleep_macro
#define sleep sstmac_sleep
#undef refactor_sleep_macro
#endif
