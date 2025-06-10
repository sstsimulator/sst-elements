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

#include <mercury/operating_system/process/global.h>
#include <mercury/operating_system/process/tls.h>

#include <cstring>
#include <cstdio>
#include <cstdint>
#include <set>

extern "C" int sst_hg_global_stacksize;

namespace SST {
namespace Hg {

class ThreadInfo {
 public:
  static void registerUserSpaceVirtualThread(int phys_thread_id, void* stack,
                                             void* globalsMap, void* tlsMap,
                                             bool isAppStartup, bool isThreadStartup);

  static void deregisterUserSpaceVirtualThread(void* stack);

  static inline int currentPhysicalThreadId(){
    uintptr_t localStorage = get_sst_hg_tls();
    int* tls = (int*) (localStorage + SST_HG_TLS_THREAD_ID);
    return *tls;
  }

};

} // end namespace Hg
} // end namespace SST
