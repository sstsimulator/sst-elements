// Copyright 2009-2023 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2023, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <mercury/common/util.h>
#include <mercury/components/operating_system.h>
#include <mercury/operating_system/process/app.h>
//#include <sstmac/null_buffer.h>

#include <cstring>

extern template class  SST::Hg::HgBase<SST::Component>;
extern template class  SST::Hg::HgBase<SST::SubComponent>;

typedef int (*main_fxn)(int,char**);
typedef int (*empty_main_fxn)();

//extern "C" double
//sst_hg_now(){
//  return sstmac::sw::OperatingSystem::currentOs()->now().sec();
//}

//extern "C" void
//sst_hg_sleep_precise(double secs){
//  sstmac::sw::OperatingSystem::currentOs()->sleep(sstmac::TimeDelta(secs));
//}

// namespace sstmac {

//SST::Params& appParams(){
//  return sstmac::sw::OperatingSystem::currentThread()->parentApp()->params();
//}

//std::string getAppParam(const std::string& name){
//  return appParams().find<std::string>(name);
//}

//bool appHasParam(const std::string& name){
//  return appParams().contains(name);
//}

//void getAppUnitParam(const std::string& name, double& val){
//  val = appParams().find<SST::UnitAlgebra>(name).getValue().toDouble();
//}

//void getAppUnitParam(const std::string& name, int& val){
//  val = appParams().find<SST::UnitAlgebra>(name).getRoundedValue();
//}

//void getAppUnitParam(const std::string& name, const std::string& def, int& val){
//  val = appParams().find<SST::UnitAlgebra>(name, def).getRoundedValue();
//}

//void getAppUnitParam(const std::string& name, const std::string& def, double& val){
//  val = appParams().find<SST::UnitAlgebra>(name, def).getRoundedValue();
//}

//void getAppArrayParam(const std::string& name, std::vector<int>& vec){
//  appParams().find_array(name, vec);
//}

//} // end namespace sstmac

extern "C" void ssthg_exit(int code)
{
  SST::Hg::OperatingSystem::currentThread()->kill(code);
}

extern "C" unsigned int ssthg_alarm(unsigned int  /*delay*/)
{
  //for now, do nothing
  //seriously, why are you using the alarm function?
  return 0;
}

extern "C" int ssthg_on_exit(void(* /*fxn*/)(int,void*), void*  /*arg*/)
{
  //for now, just ignore any atexit functions
  return 0;
}

extern "C" int ssthg_atexit(void (* /*fxn*/)(void))
{
  //for now, just ignore any atexit functions
  return 0;
}

//extern "C"
//int sst_hg_gethostname(char* name, size_t len)
//{
//  std::string sst_name = sstmac::sw::OperatingSystem::currentOs()->hostname();
//  if (sst_name.size() > len){
//    return -1;
//  } else {
//    ::strcpy(name, sst_name.c_str());
//    return 0;
//  }
//}

//extern "C"
//long sst_hg_gethostid()
//{
//  return sstmac::sw::OperatingSystem::currentOs()->addr();
//}

//extern "C"
//void sst_hg_free(void* ptr){
//#ifdef free
//#error #sstmac free macro should not be defined in util.cc - refactor needed
//#endif
//  if (isNonNullBuffer(ptr)) free(ptr);
//}

#include <unordered_map>

//extern "C"
//void sst_hg_advance_time(const char* param_name)
//{
//  sstmac::sw::Thread* thr = sstmac::sw::OperatingSystem::currentThread();
//  sstmac::sw::App* parent = thr->parentApp();
//  using ValueCache = std::unordered_map<void*,sstmac::TimeDelta>;
//  static std::map<sstmac::sw::AppId,ValueCache> cache;
//  auto& subMap = cache[parent->aid()];
//  auto iter = subMap.find((void*)param_name);
//  if (iter == subMap.end()){
//    subMap[(void*)param_name] =
//        sstmac::TimeDelta(parent->params().find<SST::UnitAlgebra>(param_name).getValue().toDouble());
//    iter = subMap.find((void*)param_name);
//  }
//  parent->compute(iter->second);
//}

int
userSkeletonMainInitFxn(const char* name, main_fxn fxn)
{
  SST::Hg::UserAppCxxFullMain::registerMainFxn(name, fxn);
  return 42;
}

int
userSkeletonMainInitFxn(const char* name, empty_main_fxn fxn)
{
  SST::Hg::UserAppCxxEmptyMain::registerMainFxn(name, fxn);
  return 42;
}

extern "C"
char* ssthg_getenv(const char* name)
{
  SST::Hg::App* a = SST::Hg::OperatingSystem::currentThread()->parentApp();
  return a->getenv(name);
}

extern "C"
int ssthg_setenv(const char* name, const char* value, int overwrite)
{
  SST::Hg::App* a = SST::Hg::OperatingSystem::currentThread()->parentApp();
  return a->setenv(name, value, overwrite);
}

extern "C"
int ssthg_putenv(char* input)
{
  SST::Hg::App* a = SST::Hg::OperatingSystem::currentThread()->parentApp();
  return a->putenv(input);
}

