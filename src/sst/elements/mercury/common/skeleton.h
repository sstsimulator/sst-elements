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

// There's a reasonable chance that we'll want the globals/TLS support eventually,
// so I'm leaving that sst-macro code here for now -- JPK

#pragma once

// Put ELI where there is otherwise no ELI
#include <mercury/common/appLoader.h>

#define SSTPP_QUOTE(name) #name
#define SSTPP_STR(name) SSTPP_QUOTE(name)

#if SST_HG_EXTERNAL
#define SST_APP_NAME_QUOTED __FILE__ __DATE__
#define SST_DEFINE_EXE_NAME extern "C" const char exe_main_name[] = SST_APP_NAME_QUOTED;
#else
#define SST_APP_NAME_QUOTED SSTPP_STR(ssthg_app_name)
#define SST_DEFINE_EXE_NAME
#endif

#define ELI_NAME(app) app##_eli
typedef int (*main_fxn)(int,char**);
typedef int (*empty_main_fxn)();

//#include <sstmac/common/sstmac_config.h>
#ifndef __cplusplus
#include <stdbool.h>
#else
#include <string>
#include <sstream>
#include <vector>
#endif

#ifdef __cplusplus
extern "C" {
#endif

//double sst_hg_sim_time();

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

/* hate that I have to do this for cmake */
#if __cplusplus < 201103L
#define char16_t char16type
#define char32_t char32type
#include <sstream>
#undef char16_t
#undef char32_t
#endif

#include <mercury/operating_system/process/cppglobal.h>
#include <new>
#include <utility>

#if __cplusplus >= 201103L
namespace SST {
class Params;
}

//namespace sstmac {

//class vector {
// public:
//  vector() : size_(0) {}

//  void resize(unsigned long sz){
//    size_ = sz;
//  }

//  unsigned long size() const {
//    return size_;
//  }

//  template <class... Args>
//  void push_back(Args...){
//    ++size_;
//  }

//  template <class... Args>
//  void emplace_back(Args...){
//    ++size_;
//  }

//  std::nullptr_t data() const {
//    return nullptr;
//  }

//  std::nullptr_t data() {
//    return nullptr;
//  }

//  bool empty() const {
//    return size_ == 0;
//  }

//  void clear() {
//    size_ = 0;
//  }

// private:
//  unsigned long  size_;
//};


//SST::Params& appParams();

//std::string getAppParam(const std::string& name);
//bool appHasParam(const std::string& name);

//void getAppUnitParam(const std::string& name, int& val);
//void getAppUnitParam(const std::string& name, const std::string& def, int& val);
//void getAppUnitParam(const std::string& name, double& val);
//void getAppUnitParam(const std::string& name, const std::string& def, double& val);
//void getAppArrayParam(const std::string& name, std::vector<int>& vec);

//template <class T> T getUnitParam(const std::string& name){
//  T t;
//  getAppUnitParam(name, t);
//  return t;
//}

//template <class T> T getUnitParam(const std::string& name, const std::string& def){
//  T t;
//  getAppUnitParam(name, def, t);
//  return t;
//}

//template <class T> std::vector<T> getArrayParam(const std::string& name){
//  std::vector<T> vec; getAppArrayParam(name, vec);
//  return vec;
//}

//template <class T> T getParam(const std::string& name){
//  std::string param = getAppParam(name);
//  std::istringstream sstr(param);
//  T t; sstr >> t;
//  return t;
//}

//template <class T, class U> T getParam(const std::string& name, U&& u){
//  if (appHasParam(name)){
//    return getParam<T>(name);
//  } else {
//    return u;
//  }
//}

//}
#endif

//#include <sstmac/software/process/global.h>
#include <mercury/operating_system/libraries/library_fwd.h>
/* end C++ */
#else
/* need for C */
static void* nullptr = 0;
#endif

#define define_var_name_pass_through(x) ssthg_dont_ignore_this##x
#define define_var_name(x) define_var_name_pass_through(x)

#ifndef SSTHG_NO_REFACTOR_MAIN
#undef main
#define main USER_MAIN
#define USER_MAIN(...) \
 fxn_that_nobody_ever_uses_to_make_magic_happen(); \
 typedef int (*this_file_main_fxn)(__VA_ARGS__); \
 int userSkeletonMainInitFxn(const char* name, this_file_main_fxn fxn); \
 static int userSkeletonMain(__VA_ARGS__); \
 SST_DEFINE_EXE_NAME \
 static int dont_ignore_this = \
  userSkeletonMainInitFxn(SST_APP_NAME_QUOTED, userSkeletonMain); \
 static int userSkeletonMain(__VA_ARGS__)
#else
#define main ssthg_ignore_for_app_name(); int main
#endif


#ifdef __cplusplus
#include <cstdint>
#ifndef _Bool
using _Bool = bool;
#endif
extern "C" {
#else
#include <stdint.h>
#endif

extern int ssthg_global_stacksize;
extern char* static_init_glbls_segment;
extern char* static_init_tls_segment;
void sst_hg_init_global_space(void* ptr, int size, int offset, bool tls);
void sst_hg_advance_time(const char* param_name);
void sst_hg_blocking_call(int condition, double timeout, const char* api);

#ifdef __cplusplus
}
#endif

#include <mercury/common/skeleton_tls.h>
#include <mercury/common/null_buffer.h>

