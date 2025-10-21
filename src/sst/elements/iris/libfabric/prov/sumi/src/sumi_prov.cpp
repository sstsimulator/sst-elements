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

#include <cstdint>
#include <sumi_fabric.hpp>
#include <mercury/operating_system/libraries/library.h>
#include <mercury/components/operating_system.h>
#include <mercury/operating_system/process/thread.h>

FabricTransport* sumi_fabric()
{
  SST::Hg::Thread* t = SST::Hg::OperatingSystem::currentThread();
  FabricTransport* tp = t->getLibrary<FabricTransport>("libfabric");
  if (!tp->inited())
    tp->init();
  return tp;
}

constexpr uint64_t FabricMessage::no_tag;
constexpr uint64_t FabricMessage::no_imm_data;
