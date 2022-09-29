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
#include <sst/core/component.h>
#include <common/component.h>
#include <common/errors.h>

#include <type_traits>
#include <map>

namespace SST {
namespace Hg {

template <class Base, class... Args>
struct Builder
{
  typedef Base* (*createFxn)(Args...);

  virtual Base* create(Args... ctorArgs) = 0;

  virtual ~Builder(){}
};

template <class Base, class... CtorArgs>
class BuilderLibrary {
 public:
  using BaseBuilder = Builder<Base,CtorArgs...>;

  BaseBuilder* getBuilder(const std::string &name) {
    auto iter = factories_.find(name);
    if (iter == factories_.end()){
      return nullptr;
    } else {
      return iter->second;
    }
  }

  const std::map<std::string, BaseBuilder*>& getMap() const {
    return factories_;
  }

  bool addBuilder(const std::string& name, BaseBuilder* fact){
    std::cerr << "adding builder " << name << "\n";
    factories_[name] = std::move(fact);
    return true;
  }

 private:
  std::map<std::string, BaseBuilder*> factories_;
};

template <class Base, class... CtorArgs>
class BuilderLibraryDatabase {
 public:
  using Library=BuilderLibrary<Base,CtorArgs...>;
  using LibraryPtr=Library*;
  using BaseFactory=typename Library::BaseBuilder;

  static Library* getLibrary(const std::string& name){
    if (!libraries){
      libraries = new std::map<std::string,LibraryPtr>;
    }
    auto iter = libraries->find(name);
    if (iter == libraries->end()){
      auto info = new Library;
      auto* ptr = info;
      (*libraries)[name] = info;
      return ptr;
    } else {
      return iter->second;
    }
  }

 private:
  // Database - needs to be a pointer for static init order
  static std::map<std::string,Library*>* libraries;
};

template <class Base, class... CtorArgs>
 std::map<std::string,BuilderLibrary<Base,CtorArgs...>*>*
  BuilderLibraryDatabase<Base,CtorArgs...>::libraries = nullptr;


template <class Base, class T>
struct InstantiateBuilder {
  static bool isLoaded() {
    return loaded;
  }

  static const bool loaded;
};

template <class Base, class T>
 const bool InstantiateBuilder<Base,T>::loaded = Base::Ctor::template add<T>(T::SST_HG_getLibrary(), T::SST_HG_getName());

template <class Base, class T, class Enable=void>
struct Allocator
{
  template <class... Args>
  T* operator()(Args&&... args){
    return new T(std::forward<Args>(args)...);
  }
};

template <class T> struct wrap { using type=void; };
template <class Base, class T>
struct Allocator<Base,T,
  typename wrap<decltype(std::declval<T>().finalizeInit(std::declval<SST::Params&>()))>::type>
{
  template <class... Args>
  Base* operator()(SST::Params& params, Args&&... args){
    T* t = new T(params, std::forward<Args>(args)...);
    t->finalizeInit(params);
    return t;
  }
};

template <class Base, class T>
struct CachedAllocator
{
  template <class... Args>
  Base* operator()(Args&&... ctorArgs) {
    if (!cached_){
      cached_ = new T(std::forward<Args>(ctorArgs)...);
    }
    return cached_;
  }

  static Base* cached_;
};
template <class Base, class T>
  Base* CachedAllocator<Base,T>::cached_ = nullptr;

template <class Base, class T, class... Args>
struct DerivedBuilder : public Builder<Base,Args...>
{
  Base* create(Args... ctorArgs) override {
    return Allocator<Base,T>()(std::forward<Args>(ctorArgs)...);
  }
};


template <class T, class U>
struct is_tuple_constructible : public std::false_type {};

template <class T, class... Args>
struct is_tuple_constructible<T, std::tuple<Args...>> :
  public std::is_constructible<T, Args...>
{
};

struct BuilderDatabase {
  template <class T, class... Args>
  static BuilderLibrary<T,Args...>* getLibrary(const std::string& name){
    return BuilderLibraryDatabase<T,Args...>::getLibrary(name);
  }
};

template <class Base, class CtorTuple>
struct ElementsBuilder {};

template <class Base, class... Args>
struct ElementsBuilder<Base, std::tuple<Args...>>
{
  static BuilderLibrary<Base,Args...>* getLibrary(const std::string& name){
    return BuilderLibraryDatabase<Base,Args...>::getLibrary(name);
  }

  template <class T> static Builder<Base,Args...>* makeBuilder(){
    return new DerivedBuilder<Base,T,Args...>();
  }

};

template <class Base, class T, class... Args>
DerivedBuilder<Base,T,Args...>*
makeDerivedBuilder(){
  using ret = DerivedBuilder<Base,T,Args...>;
  return new ret;
}

template <class Base, class... Args>
struct SingleCtor
{
  template <class T> static bool add(const std::string& elemlib, const std::string& elem){
    //if abstract, force an allocation to generate meaningful errors
    Base::addBuilder(elemlib,elem,makeDerivedBuilder<Base,T,Args...>());
    return true;
  }
};


template <class Base, class Ctor, class... Ctors>
struct CtorList : public CtorList<Base,Ctors...>
{
  template <class T, int NumValid=0, class U=T>
  static typename std::enable_if<std::is_abstract<U>::value || is_tuple_constructible<U,Ctor>::value, bool>::type
  add(const std::string& elemlib, const std::string& elem){
    //if abstract, force an allocation to generate meaningful errors
    auto fact = ElementsBuilder<Base,Ctor>::template makeBuilder<U>();
    Base::addBuilder(elemlib,elem,std::move(fact));
    return CtorList<Base,Ctors...>::template add<T,NumValid+1>(elemlib,elem);
  }

  template <class T, int NumValid=0, class U=T>
  static typename std::enable_if<!std::is_abstract<U>::value && !is_tuple_constructible<U,Ctor>::value, bool>::type
  add(const std::string& elemlib, const std::string& elem){
    return CtorList<Base,Ctors...>::template add<T,NumValid>(elemlib,elem);
  }

};

template <int NumValid>
struct NoValidConstructorsForDerivedType {
  static constexpr bool atLeastOneValidCtor = true;
};

template <> class NoValidConstructorsForDerivedType<0> {};

template <class Base> struct CtorList<Base,void>
{
  template <class T,int numValidCtors>
  static bool add(const std::string&  /*lib*/, const std::string&  /*elem*/){
    return NoValidConstructorsForDerivedType<numValidCtors>::atLeastOneValidCtor;
  }
};

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

#define SST_HG_CTOR(...) std::tuple<__VA_ARGS__>
#define SST_HG_DEFAULT_CTOR() std::tuple<>
#define SST_HG_FORWARD_AS_ONE(...) __VA_ARGS__

#define SST_HG_DECLARE_BASE(Base) \
  using __LocalHgBase = Base; \
  static const char* SST_HG_baseName(){ return #Base; }

#define SST_HG_CTORS_COMMON(...) \
  using Ctor = sprockit::CtorList<__LocalHgBase,__VA_ARGS__,void>; \
  template <class __TT, class... __CtorArgs> \
  using DerivedBuilder = sprockit::DerivedBuilder<__LocalHgBase,__TT,__CtorArgs...>; \
  template <class... __InArgs> static sprockit::BuilderLibrary<__LocalHgBase,__InArgs...>* \
  getBuilderLibraryTemplate(const std::string& name){ \
    return sprockit::BuilderDatabase::getLibrary<__LocalHgBase,__InArgs...>(name); \
  } \
  template <class __TT> static bool addDerivedBuilder(const std::string& lib, const std::string& elem){ \
    return Ctor::template add<0,__TT>(lib,elem); \
  } \

#define SST_HG_DECLARE_CTORS(...) \
  SST_HG__CTORS_COMMON(SST_HG_FORWARD_AS_ONE(__VA_ARGS__)) \
  template <class... Args> static bool addBuilder(const std::string& elemlib, const std::string& elem, \
                                           sprockit::Builder<__LocalHgBase,Args...>* builder){ \
    return getBuilderLibraryTemplate<Args...>(elemlib)->addBuilder(elem, builder); \
  }

#define SST_HG_DECLARE_CTORS_EXTERN(...) \
  SST_HG_CTORS_COMMON(SST_HG_FORWARD_AS_ONE(__VA_ARGS__))

#define SST_HG_CTOR_COMMON(...) \
  using Ctor = sprockit::SingleCtor<__LocalHgBase,__VA_ARGS__>; \
  using BaseBuilder = sprockit::Builder<__LocalHgBase,__VA_ARGS__>; \
  using BuilderLibrary = sprockit::BuilderLibrary<__LocalHgBase,__VA_ARGS__>; \
  using BuilderLibraryDatabase = sprockit::BuilderLibraryDatabase<__LocalHgBase,__VA_ARGS__>; \
  template <class __TT> using DerivedBuilder = sprockit::DerivedBuilder<__LocalHgBase,__TT,__VA_ARGS__>; \
  template <class __TT> static bool addDerivedBuilder(const std::string& lib, const std::string& elem){ \
    return addBuilder(lib,elem,new DerivedBuilder<__TT>); \
  }

//I can make some extra using typedefs because I have only a single ctor
#define SST_HG_DECLARE_CTOR(...) \
  SST_HG_CTOR_COMMON(__VA_ARGS__) \
  static BuilderLibrary* getBuilderLibrary(const std::string& name){ \
    return sprockit::BuilderDatabase::getLibrary<__LocalHgBase,__VA_ARGS__>(name); \
  } \
  static bool addBuilder(const std::string& elemlib, const std::string& elem, \
                BaseBuilder* builder){ \
    return getBuilderLibrary(elemlib)->addBuilder(elem,builder); \
  }

#define SST_HG_DECLARE_CTOR_EXTERN(...) \
  SST_HG_CTOR_COMMON(__VA_ARGS__) \
  static BuilderLibrary* getBuilderLibrary(const std::string& name); \
  static bool addBuilder(const std::string& elemlib, const std::string& elem, \
                         BaseBuilder* builder);

#define SST_HG_DEFINE_CTOR_EXTERN(base) \
  bool base::addBuilder(const std::string& elemlib, const std::string& elem, \
                        BaseBuilder* builder){ \
    return getBuilderLibrary(elemlib)->addBuilder(elem,std::move(builder)); \
  } \
  base::BuilderLibrary* base::getBuilderLibrary(const std::string& elemlib){ \
    return BuilderLibraryDatabase::getLibrary(elemlib); \
  }

#define SST_HG_DEFAULT_CTOR_COMMON() \
  using Ctor = sprockit::SingleCtor<__LocalHgBase>; \
  using BaseBuilder = sprockit::Builder<__LocalHgBase>; \
  using BuilderLibrary = sprockit::BuilderLibrary<__LocalHgBase>; \
  template <class __TT> using DerivedBuilder = sprockit::DerivedBuilder<__LocalHgBase,__TT>;

//I can make some extra using typedefs because I have only a single ctor
#define SST_HG_DECLARE_DEFAULT_CTOR() \
  SST_HG_DEFAULT_CTOR_COMMON() \
  static BuilderLibrary* getBuilderLibrary(const std::string& name){ \
    return sprockit::BuilderDatabase::getLibrary<__LocalHgBase>(name); \
  } \
  static bool addBuilder(const std::string& elemlib, const std::string& elem, \
                         BaseBuilder* builder){ \
    return getBuilderLibrary(elemlib)->addBuilder(elem,std::move(builder)); \
  }

#define SST_HG_DECLARE_DEFAULT_CTOR_EXTERN() \
  SST_HG_DEFAULT_CTOR_COMMON() \
  static BuilderLibrary* getBuilderLibrary(const std::string& name); \
  static bool addBuilder(const std::string& elemlib, const std::string& elem, \
                         BaseBuilder* builder);

#define SST_HG_REGISTER_DERIVED(base,cls,lib,name,desc) \
  bool ELI_isLoaded() { \
    return sprockit::InstantiateBuilder<base,cls>::isLoaded(); \
  } \
    static const std::string SST_HG_getLibrary() { \
    return lib; \
  } \
  static const std::string SST_HG_getName() { \
    return name; \
  }

