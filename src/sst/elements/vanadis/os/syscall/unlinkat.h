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

#ifndef _H_VANADIS_OS_SYSCALL_UNLINKAT
#define _H_VANADIS_OS_SYSCALL_UNLINKAT

#include "os/syscall/syscall.h"
#include "os/callev/voscallunlinkat.h"

namespace SST {
namespace Vanadis {

class VanadisUnlinkatSyscall : public VanadisSyscall {
public:
    VanadisUnlinkatSyscall( VanadisNodeOSComponent* os, SST::Link* coreLink, OS::ProcessInfo* process, VanadisSyscallUnlinkatEvent* event ) 
        : VanadisSyscall( os, coreLink, process, event, "unlinkat" ) 
    {
        m_output->verbose( CALL_INFO, 16, 0, "[syscall-unlinkat] -> call is unlinkat( %" PRId64 " )\n", event->getPathPointer());

        m_dirFd  = event->getDirectoryFileDescriptor();
        // if the directory fd passed by the syscall is positive it should point to a entry in the file_descriptor table
        // if the directory fd is negative pass that to to the unlinkat handler ( AT_FDCWD is negative )
        if ( m_dirFd > -1 ) {
            m_dirFd = process->getFileDescriptor( m_dirFd );

            if ( -1 == m_dirFd) {
                m_output->verbose(CALL_INFO, 16, 0, "can't find m_dirFd=%d in unlink file descriptor table\n", m_dirFd);
                setReturnFail(-LINUX_EBADF);
                return;
            }
            // get the FD that SST will use
            m_output->verbose(CALL_INFO, 16, 0, "sst fd=%d pathname=%s\n", m_dirFd, process->getFilePath(m_dirFd).c_str());
        }

        m_output->verbose( CALL_INFO, 16, 0, "[syscall-unlinkat] -> call is unlinkat( %" PRId64 ", 0x%0" PRI_ADDR ", %" PRId64 " )\n",
                event->getDirectoryFileDescriptor(), event->getPathPointer(), event->getFlags());

        readString(event->getPathPointer(),m_filename);
    }

    void memReqIsDone(bool) {
        m_output->verbose(CALL_INFO, 16, 0, "[syscall-unlinkat] path: \"%s\"\n", m_filename.c_str());

        if ( unlinkat( m_dirFd, m_filename.c_str(), getEvent<VanadisSyscallUnlinkatEvent*>()->getFlags() ) ) {
            auto myErrno = errno; 
            char buf[100];
            char* str = buf;
#ifdef SST_COMPILE_MACOSX
            if ( myErrno == EBADF ) {
                myErrno = ENOENT;
            }
            strerror_r(errno,buf,100);
#else
            str = strerror_r(errno,buf,100);
#endif
            m_output->verbose(CALL_INFO, 16, 0, "[syscall-unlinkat] unlink of %s failed, errno=%d `%s`\n", m_filename.c_str(), errno, str );
            setReturnFail( -myErrno );
        } else {
            setReturnSuccess(0);
        }
    }

 private:
    int m_dirFd;
    std::string m_filename;
};

} // namespace Vanadis
} // namespace SST

#endif
