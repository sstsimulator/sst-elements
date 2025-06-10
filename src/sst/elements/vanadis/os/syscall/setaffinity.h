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

#ifndef _H_VANADIS_OS_SYSCALL_SETAFFINITY
#define _H_VANADIS_OS_SYSCALL_SETAFFINITY

#include "os/syscall/syscall.h"
#include "os/callev/voscallsetaffinity.h"

namespace SST {
namespace Vanadis {

class VanadisSetaffinitySyscall : public VanadisSyscall {
public:
    VanadisSetaffinitySyscall(
        VanadisNodeOSComponent* os,
        SST::Link* coreLink,
        OS::ProcessInfo* process,
        VanadisSyscallSetaffinityEvent* event )
    : VanadisSyscall(os, coreLink, process, event, "setaffinity")
    {
        m_output->verbose(CALL_INFO, 2, VANADIS_OS_DBG_SYSCALL, "[syscall-setaffinity] pid=%" PRIu64 " cpusetsize=%" PRIu64 " maskAddr=%#" PRIx64 "\n",
                                      event->getPid(), event->getCpusetsize(), event->getMaskAddr());

        // Load the CPU mask from memory
        m_mask.resize(event->getCpusetsize(), 0);
        readMemory(event->getMaskAddr(), m_mask);
    }

    void memReqIsDone(bool success) override {
        m_process->setAffinity(m_mask);
        setReturnSuccess(0);
    }

private:
    std::vector<uint8_t> m_mask;
};

} // namespace Vanadis
} // namespace SST

#endif
