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

#ifndef _H_VANADIS_OS_SYSCALL_IOCTL
#define _H_VANADIS_OS_SYSCALL_IOCTL

#include "os/syscall/syscall.h"
#include "os/callev/voscallioctl.h"

namespace SST {
namespace Vanadis {

class VanadisIoctlSyscall : public VanadisSyscall {
public:
    VanadisIoctlSyscall( VanadisNodeOSComponent* os, SST::Link* coreLink, OS::ProcessInfo* process, VanadisSyscallIoctlEvent* event )
        : VanadisSyscall( os, coreLink, process, event, "ioctl" ) 
    {
        m_output->verbose(CALL_INFO, 2, VANADIS_OS_DBG_SYSCALL,
                            "[syscall-ioctl] ioctl( %" PRId64 ", r: %c / w: %c / ptr: 0x%" PRI_ADDR " / size: %" PRIu64
                            " / op: %" PRIu64 " / drv: %" PRIu64 " )\n",
                            event->getFileDescriptor(), event->isRead() ? 'y' : 'n',
                            event->isWrite() ? 'y' : 'n', event->getDataPointer(), event->getDataLength(),
                            event->getIOOperation(), event->getIODriver());

        setReturnFail(-LINUX_ENOTTY );
    }
};

} // namespace Vanadis
} // namespace SST

#endif
