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

#ifndef _H_VANADIS_OS_SYSCALL_GETAFFINITY
#define _H_VANADIS_OS_SYSCALL_GETAFFINITY

#include "os/syscall/syscall.h"
#include "os/callev/voscallgetaffinity.h"

namespace SST {
namespace Vanadis {

class VanadisGetaffinitySyscall : public VanadisSyscall {
public:
    VanadisGetaffinitySyscall( VanadisNodeOSComponent* os, SST::Link* coreLink, OS::ProcessInfo* process, VanadisSyscallGetaffinityEvent* event )
        : VanadisSyscall( os, coreLink, process, event, "getaffinity" ), m_cpusetSize(event->getCpusetsize())
    {
        m_output->verbose(CALL_INFO, 2, VANADIS_OS_DBG_SYSCALL, "[syscall-getaffinity] uname pid=%" PRIu64 " cpusetsize=%" PRIu64 " maskAddr=%#" PRIx64 "\n",
                                            event->getPid(), event->getCpusetsize(), event->getMaskAddr());

        m_mask.resize(m_cpusetSize, 0);
        std::vector<uint8_t> affinity = m_process->getAffinity();
        m_mask.assign(affinity.begin(), affinity.end());
        writeMemory(event->getMaskAddr(), m_mask);
    }

    void memReqIsDone(bool) {
        setReturnSuccess(m_cpusetSize);
    }

 private:  
    std::vector<uint8_t> m_mask;
    uint64_t m_cpusetSize;
};

} // namespace Vanadis
} // namespace SST

#endif
