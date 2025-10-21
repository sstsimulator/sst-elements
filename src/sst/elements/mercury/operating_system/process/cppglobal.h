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

#include <utility>
#include <mercury/operating_system/process/global.h>
#include <new>

#if __cplusplus >= 201103L
#include <tuple>
#include <functional>

namespace SST {
namespace Hg {

/**
 * @brief The CppGlobalRegisterGuard struct
 * Constructor causes the global variable to be registered and requires init for each new thread
 * Destructor causes the global variable to be unregistestered
 */
struct CppGlobalRegisterGuard {
  CppGlobalRegisterGuard(int& offset, int size,
                         bool tls, const char* name, std::function<void(void*)>&& fxn);

  ~CppGlobalRegisterGuard();

 private:
  bool tls_;
  int offset_;
};

template <class T, bool tls>
struct CppInplaceGlobalInitializer {
  CppInplaceGlobalInitializer(const char* name)
  {
    offset = GlobalVariable::init(sizeof(T), name, tls);
  }
  static int offset;
};
template <class T, bool tls> int CppInplaceGlobalInitializer<T,tls>::offset;

template <class Tag, class T, bool tls>
int inplaceCppGlobal(const char* name, std::function<void(void*)>&& fxn){
  static CppInplaceGlobalInitializer<T,tls> init{name};
  static CppGlobalRegisterGuard holder(init.offset, sizeof(T), tls, name, std::move(fxn));
  return init.offset;
}

/** a special wrapper for variable template members */
template <class Tag, class T, bool tls>
class CppVarTemplate {
 public:
  template <class... CtorArgs>
  static void invoke(void* ptr, CtorArgs&&... args){
    new (ptr) T(std::forward<CtorArgs>(args)...);
  }

  template <class... Args>
  CppVarTemplate(Args&&... args){
    std::function<void(void*)> f = std::bind(&invoke<Args&...>, std::placeholders::_1, std::forward<Args>(args)...);
    offset_ = inplaceCppGlobal<Tag,T,tls>("no name", std::move(f));
  }

  int getOffset() const {
    return offset_;
  }

  T& operator()(){
    return tls ? get_tls_ref_at_offset<T>(offset_) : get_global_ref_at_offset<T>(offset_);
  }

  int offset_;
};

}
}

#endif //C++11

