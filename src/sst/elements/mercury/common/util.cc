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

#include <common/util.h>
#include <components/operating_system.h>
#include <operating_system/process/app.h>
//#include <sstmac/null_buffer.h>

#include <cstring>

typedef int (*main_fxn)(int,char**);
typedef int (*empty_main_fxn)();

//extern "C" double
//sstmac_now(){
//  return sstmac::sw::OperatingSystem::currentOs()->now().sec();
//}

//extern "C" void
//sstmac_sleep_precise(double secs){
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
//int sstmac_gethostname(char* name, size_t len)
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
//long sstmac_gethostid()
//{
//  return sstmac::sw::OperatingSystem::currentOs()->addr();
//}

//extern "C"
//void sstmac_free(void* ptr){
//#ifdef free
//#error #sstmac free macro should not be defined in util.cc - refactor needed
//#endif
//  if (isNonNullBuffer(ptr)) free(ptr);
//}

#include <unordered_map>

//extern "C"
//void sstmac_advance_time(const char* param_name)
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

