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

#include <mercury/operating_system/process/tls.h>
#include <list>
#include <map>
#include <functional>
#include <unordered_set>

extern "C" int sst_hg_global_stacksize;

namespace SST {
namespace Hg {

class GlobalVariableContext {
 public:
  void init();

  ~GlobalVariableContext();

  int append(const int size, const char* name);

  int globalsSize() {
    return stackOffset;
  }

  int allocSize() {
    return allocSize_;
  }

  void setAllocSize(int sz){
    allocSize_ = sz;
  }

  void* globalInit() {
    return globalInits;
  }

  void addActiveSegment(void* globals){
    activeGlobalMaps_.insert(globals);
  }

  void removeActiveSegment(void* globals){
    activeGlobalMaps_.erase(globals);
  }

  void initGlobalSpace(void* ptr, int size, int offset);

  void callInitFxns(void* globals);

  void unregisterInitFxn(int offset){
    initFxns.erase(offset);
  }

  void registerInitFxn(int offset, std::function<void(void*)>&& fxn);

 private:
  int stackOffset;
  char* globalInits;
  int allocSize_;
  //these should be ordered by the offset in the data segment
  //this ensures as much as possible that global variables
  //are initialized in the same order in SST/macro as they would be in the real app
  std::map<int, std::function<void(void*)>> initFxns;

 private:
  std::unordered_set<void*> activeGlobalMaps_;

};

class GlobalVariable {
 public:
  static int init(const int size, const char* name, bool tls = false);

  static GlobalVariableContext glblCtx;
  static GlobalVariableContext tlsCtx;
  static bool inited;
};

static inline void* get_special_at_offset(int offset, int map_offset)
{
  int stack; int* stackPtr = &stack;
  intptr_t stackTopInt = ((intptr_t)stackPtr/sst_hg_global_stacksize)*sst_hg_global_stacksize + map_offset;
  char** stackTopPtr = (char**) stackTopInt;
  char* globalMap = *stackTopPtr;
  return globalMap + offset;
}

static inline void* get_global_at_offset(int offset){
  return get_special_at_offset(offset, SST_HG_TLS_GLOBAL_MAP);
}

static inline void* get_tls_at_offset(int offset){
  return get_special_at_offset(offset, SST_HG_TLS_TLS_MAP);
}

template <class T>
static inline T& get_global_ref_at_offset(int offset){
  T* t = (T*) get_global_at_offset(offset);
  return *t;
}

template <class T>
static inline T& get_tls_ref_at_offset(int offset){
  T* t = (T*) get_tls_at_offset(offset);
  return *t;
}

template <class T,class enable=void>
struct global {};

template <class T>
struct global<T*,void> : public GlobalVariable {
  explicit global(){
    offset = GlobalVariable::init(sizeof(T*),"",false);
  }

  explicit global(T* t){
    offset = GlobalVariable::init(sizeof(T*),"",false);
    GlobalVariable::glblCtx.registerInitFxn(offset, [=](void* ptr){
      T** tPtr = (T**) ptr;
      *tPtr = t;
    });
  }

  T*& get() {
    return get_global_ref_at_offset<T*>(offset);
  }

  operator T*() const {
    return get_global_ref_at_offset<T*>(offset);
  }

  template <class U>
  T*& operator=(U*& u) {
    T*& t = get_global_ref_at_offset<T*>(offset);
    t = u;
    return t;
  }

  template <class U>
  T*& operator=(U& u) {
    T*& t = get_global_ref_at_offset<T*>(offset);
    t = u;
    return t;
  }

  int offset;
};


template <class T>
struct global<T,typename std::enable_if<std::is_arithmetic<T>::value>::type> {

  explicit global()
  {
    offset = GlobalVariable::init(sizeof(T), "", false);
  }

  explicit global(const T& t)
  {
    offset = GlobalVariable::init(sizeof(T), "", false);
    GlobalVariable::glblCtx.registerInitFxn(offset, [=](void* ptr){
      T* tPtr = (T*) ptr;
      *tPtr = t;
    });
  }

  template <class U>
  T& operator=(const U& u){
    T& t = get_global_ref_at_offset<T>(offset);
    t = u;
    return t;
  }

  template <class U>
  operator U() const {
    return get_global_ref_at_offset<T>(offset);
  }

  T& get() const {
    return get_global_ref_at_offset<T>(offset);
  }

  template <class U>
  bool operator<(const U& u){
    return get_global_ref_at_offset<T>(offset) < u;
  }

  template <class U>
  void operator+=(const U& u){
    get_global_ref_at_offset<T>(offset) += u;
  }

  T& operator++(){
    T& t = get_global_ref_at_offset<T>(offset);
    t++;
    return t;
  }

  T operator++(int offset){
    T& t = get_global_ref_at_offset<T>(offset);
    T result(t);
    t++;
    return result;
  }

  T* operator&() {
    return (T*) get_global_at_offset(offset);
  }

  int offset;
};

#define OPERATOR(op,ret) \
  template <typename T, typename U> ret \
  operator op(const global<T>& l, const U& r){ \
    return l.get() op r; \
  } \
  template <typename T, typename U> ret \
  operator op(const U& l, const global<T>& r){ \
    return l op r.get(); \
  }

OPERATOR(+,T)
OPERATOR(-,T)
OPERATOR(*,T)
OPERATOR(&,T)
OPERATOR(<,bool)
OPERATOR(<=,bool)
OPERATOR(>,bool)
OPERATOR(>=,bool)
OPERATOR(==,bool)

}
}

typedef SST::Hg::global<int> global_int;
typedef SST::Hg::global<const char*> global_cstr;
