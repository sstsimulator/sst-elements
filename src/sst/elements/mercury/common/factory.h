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

#include <sst/core/params.h>
#include <sst/core/component.h>
#include <mercury/common/component.h>
#include <mercury/common/errors.h>

#include <type_traits>
#include <map>

namespace SST {
namespace Hg {

template <class T> struct wrap { using type=void; };


template <class Base, class Enable=void>
struct GetBaseName {};

template <class Base>
struct GetBaseName<Base, typename wrap<decltype(Base::ELI_baseName())>::type>
{
  const char* operator()(){
    return Base::ELI_baseName();
  }
};

template <class Base>
struct GetBaseName<Base, typename wrap<decltype(Base::SST_HG_baseName())>::type>
{
  const char* operator()(){
    return Base::SST_HG_baseName();
  }
};



template <class Base, class... Args>
Base* create(const std::string& elemlib, const std::string& elem, Args&&... args){
  auto* lib = Base::getBuilderLibrary(elemlib);
  if (!lib){
    sst_hg_abort_printf("cannot find library '%s' for base type '%s'",
                      elemlib.c_str(), GetBaseName<Base>()());
  }
  auto* builder = lib->getBuilder(elem);
  if (!builder){
    sst_hg_abort_printf("cannot find builder '%s' in library '%s' for base type %s",
                      elem.c_str(), elemlib.c_str(), GetBaseName<Base>()());
  }
  return builder->create(std::forward<Args>(args)...);
}

} // end namespace Hg
} // end namespace SST

