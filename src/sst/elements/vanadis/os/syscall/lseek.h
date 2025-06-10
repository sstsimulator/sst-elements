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

#ifndef _H_VANADIS_OS_SYSCALL_LSEEK
#define _H_VANADIS_OS_SYSCALL_LSEEK

#include "os/syscall/syscall.h"
#include "os/callev/voscalllseek.h"

namespace SST {
namespace Vanadis {

class VanadisLseekSyscall : public VanadisSyscall {
public:
    VanadisLseekSyscall( VanadisNodeOSComponent* os, SST::Link* coreLink, OS::ProcessInfo* process, VanadisSyscallLseekEvent* event )
        : VanadisSyscall( os, coreLink, process, event, "lseek" )
    {
        m_output->verbose(CALL_INFO, 16, 0, "[syscall-lseek] -> call is lseek( %" PRId32 " %" PRId64 " %" PRId32 " )\n",
                event->getFileDescriptor(), event->getOffset(), event->getWhence() );

        int fd = process->getFileDescriptor( event->getFileDescriptor() );
        if ( -1 == fd ) {
            m_output->verbose(CALL_INFO, 3, VANADIS_OS_DBG_SYSCALL,
                                "[syscall-%s] file handle %" PRId32
                                " is not currently open, return an error code.\n",
                                getName().c_str(), event->getFileDescriptor());

            setReturnFail(-LINUX_EINVAL);
         } else {

            off_t ret = lseek( fd, (off_t) event->getOffset(), (int) event->getWhence() );
            m_output->verbose(CALL_INFO, 16, 0, "[syscall-lseek] ret=%" PRIu64 "\n",ret );
            if ( -1 == ret  ) {
                // need to map host errno values to vanadis exe errno values
                assert(0);
                //setReturnFail(-LINUX_EINVAL);
            } else {
                setReturnSuccess( ret );
            }
         }
    }
};

} // namespace Vanadis
} // namespace SST

#endif
