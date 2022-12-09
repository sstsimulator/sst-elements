// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
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
    VanadisIoctlSyscall( Output* output, Link* link, OS::ProcessInfo* process, SendMemReqFunc* func, VanadisSyscallIoctlEvent* event )
        : VanadisSyscall( output, link, process, func, event, "ioctl" ) 
    {
        m_output->verbose(CALL_INFO, 16, 0,
                            "[syscall-ioctl] ioctl( %" PRId64 ", r: %c / w: %c / ptr: 0x%llx / size: %" PRIu64
                            " / op: %" PRIu64 " / drv: %" PRIu64 " )\n",
                            event->getFileDescriptor(), event->isRead() ? 'y' : 'n',
                            event->isWrite() ? 'y' : 'n', event->getDataPointer(), event->getDataLength(),
                            event->getIOOperation(), event->getIODriver());

        setReturnFail(-25 );
    }
};

} // namespace Vanadis
} // namespace SST

#endif
