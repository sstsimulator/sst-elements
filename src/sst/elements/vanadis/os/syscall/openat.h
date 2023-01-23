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

#ifndef _H_VANADIS_OS_SYSCALL_OPENAT
#define _H_VANADIS_OS_SYSCALL_OPENAT

#include "os/syscall/syscall.h"
#include "os/callev/voscallopenat.h"

namespace SST {
namespace Vanadis {

class VanadisOpenatSyscall : public VanadisSyscall {
public:
    VanadisOpenatSyscall( VanadisNodeOSComponent* os, SST::Link* coreLink, OS::ProcessInfo* process, VanadisSyscallOpenatEvent* event ) 
        : VanadisSyscall( os, coreLink, process, event, "openat" )
    {
        m_output->verbose( CALL_INFO, 16, 0, "[syscall-openat] -> call is openat( %" PRId64 " )\n", event->getPathPointer());

        m_dirFd  = event->getDirectoryFileDescriptor();
        // if the directory fd passed by the syscall is positive it should point to a entry in the file_descriptor table
        // if the directory fd is negative pass that to to the openat handler ( AT_FDCWD is negative )
        if ( m_dirFd > -1 ) {
            m_dirFd = process->getFileDescriptor( m_dirFd );

            if ( -1 == m_dirFd) {
                m_output->verbose(CALL_INFO, 16, 0, "can't find m_dirFd=%d in open file descriptor table\n", m_dirFd);
                setReturnFail(-EBADF);
                return;
            }
            // get the FD that SST will use
            m_output->verbose(CALL_INFO, 16, 0, "sst fd=%d pathname=%s\n", m_dirFd, process->getFilePath(m_dirFd).c_str());
        }

        m_output->verbose( CALL_INFO, 16, 0, "[syscall-openat] -> call is openat( %" PRId64 ", 0x%0llx, %" PRId64 " )\n",
                event->getDirectoryFileDescriptor(), event->getPathPointer(), event->getFlags());

        readString(event->getPathPointer(),m_filename);
    }

    void memReqIsDone() {
        m_output->verbose(CALL_INFO, 16, 0, "[syscall-openat] path: \"%s\"\n", m_filename.c_str());

        int fd = m_process->openFile( m_filename.c_str(), m_dirFd, getEvent<VanadisSyscallOpenatEvent*>()->getFlags(), getEvent<VanadisSyscallOpenatEvent*>()->getMode() );
        // we need to save this because any other library calls could change it
        auto myErrno = errno;

        if ( fd < 0 ) {
#ifdef SST_COMPILE_MACOSX
            if ( myErrno == EROFS ) {
                myErrno = EACCES;
            }
#endif
            char buf[100];
            auto str = strerror_r(myErrno,buf,100); 
            m_output->verbose(CALL_INFO, 16, 0, "[syscall-openat] open of %s failed, errno=%d `%s`\n", m_filename.c_str(), myErrno, str );

            setReturnFail( -myErrno );
        } else {
            setReturnSuccess( fd ); 
        }

    }

 private:
    int m_dirFd;
    std::string m_filename;
};

} // namespace Vanadis
} // namespace SST

#endif
