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

#ifndef _H_VANADIS_OS_SYSCALL_GETPID
#define _H_VANADIS_OS_SYSCALL_GETPID

#include "os/syscall/syscall.h"
#include "os/callev/voscallgetx.h"

namespace SST {
namespace Vanadis {

class VanadisGetpidSyscall : public VanadisSyscall {
public:
    VanadisGetpidSyscall( VanadisNodeOSComponent* os, SST::Link* coreLink, OS::ProcessInfo* process, VanadisSyscallGetxEvent* event )
        : VanadisSyscall( os, coreLink, process, event, "getpid" )
    {
        m_output->verbose(CALL_INFO, 16, 0, "[syscall-getpid] pid=%d\n",process->getpid());
        setReturnSuccess(process->getpid());
    }
};

} // namespace Vanadis
} // namespace SST

#endif
