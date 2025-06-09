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

#ifndef _H_VANADIS_OS_SYSCALL_SCHED_YIELD
#define _H_VANADIS_OS_SYSCALL_SCHED_YIELD

#include "os/syscall/syscall.h"
#include "os/callev/voscallschedyield.h"

namespace SST {
namespace Vanadis {

class VanadisSchedYieldSyscall : public VanadisSyscall {
public:
    VanadisSchedYieldSyscall(
        VanadisNodeOSComponent* os, SST::Link* coreLink,
        OS::ProcessInfo* process,
        VanadisSyscallSchedYieldEvent* event)
    : VanadisSyscall(os, coreLink, process, event, "sched_yield")
    {
        // No arguments to decode
        m_output->verbose(CALL_INFO, 2, VANADIS_OS_DBG_SYSCALL,
            "[syscall-sched_yield] -> sched_yield()\n");
        setReturnSuccess(0);
    }
};

} // namespace Vanadis
} // namespace SST

#endif
