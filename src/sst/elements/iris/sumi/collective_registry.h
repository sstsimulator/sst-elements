// Copyright 2009-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2026, NTESS
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

#include <iris/sumi/collective.h>
#include <iris/sumi/comm_functions.h>
#include <iris/sumi/transport_fwd.h>
#include <iris/sumi/communicator_fwd.h>
#include <string>

namespace SST::Iris::sumi {

// Arguments for scalar-count collective factories. Variable-count operations
// are not supported by this registry.
struct CollectiveFactoryArgs {
  CollectiveEngine* engine;
  void* dst;
  void* src;
  int root;
  int nelems;
  int type_size;
  int tag;
  int cq_id;
  reduce_fxn fxn;
  Communicator* comm;
};

class CollectiveRegistry {
 public:
  // "lib.name" loads an external ELI library; bare names use iris.
  // nullptr if (ty, name) is unregistered or name is empty.
  static Collective* make(Collective::type_t ty, const std::string& name,
                          const CollectiveFactoryArgs& args);

  static bool has(Collective::type_t ty, const std::string& name);
};

#define SUMI_REGISTER_COLLECTIVE_LIBRARY(TYPE, LIB, NAME, CLASS, ...)           \
  class SumiEli_##TYPE##_##CLASS : public CLASS {                               \
   public:                                                                       \
    SST_ELI_REGISTER_DERIVED(                                                    \
      ::SST::Iris::sumi::Collective, SumiEli_##TYPE##_##CLASS, LIB,             \
      #TYPE "." NAME, SST_ELI_ELEMENT_VERSION(1, 0, 0),                         \
      #CLASS " collective algorithm")                                          \
    explicit SumiEli_##TYPE##_##CLASS(                                          \
        const ::SST::Iris::sumi::CollectiveFactoryArgs& a)                      \
      : CLASS(__VA_ARGS__) {}                                                    \
  };

#define SUMI_REGISTER_COLLECTIVE(TYPE, NAME, CLASS, ...)                         \
  SUMI_REGISTER_COLLECTIVE_LIBRARY(TYPE, "iris", NAME, CLASS, __VA_ARGS__)

}
