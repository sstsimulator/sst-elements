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

#ifndef _H_VANADIS_OS_SYSCALL_READLINKAT
#define _H_VANADIS_OS_SYSCALL_READLINKAT

#include "os/syscall/syscall.h"
#include "os/callev/voscallreadlinkat.h"

namespace SST {
namespace Vanadis {

class VanadisReadlinkatSyscall : public VanadisSyscall {
public:
    VanadisReadlinkatSyscall( VanadisNodeOSComponent* os, SST::Link* coreLink, OS::ProcessInfo* process, VanadisSyscallReadLinkAtEvent* event )
        : VanadisSyscall( os, coreLink, process, event, "readlinkat" ), m_data( event->getBufsize()), m_state(READ)
    {
        m_output->verbose(CALL_INFO, 2, VANADIS_OS_DBG_SYSCALL, "[syscall-readlinkat] -> readlinkat( dirfd=%d pathname=%#" PRIx64 " buf=%#" PRIx64 " size=%" PRIu64 ")\n",
                                event->getDirfd(), event->getPathname(), event->getBuf(), event->getBufsize());

        m_dirFd  = event->getDirfd();
        // if the directory fd passed by the syscall is positive it should point to a entry in the file_descriptor table
        // if the directory fd is negative pass that to to the openat handler ( AT_FDCWD is negative )
        if ( m_dirFd > -1 ) {
            m_dirFd = process->getFileDescriptor( m_dirFd );

            if ( -1 == m_dirFd) {
                m_output->verbose(CALL_INFO, 2, VANADIS_OS_DBG_SYSCALL, "can't find m_dirFd=%d in open file descriptor table\n", m_dirFd);
                setReturnFail(-LINUX_EBADF);
                return;
            }
            // get the FD that SST will use
            m_output->verbose(CALL_INFO, 2, VANADIS_OS_DBG_SYSCALL, "sst fd=%d pathname=%s\n", m_dirFd, process->getFilePath(m_dirFd).c_str());
        }

        readString(event->getPathname(),m_filename);
    }

    void memReqIsDone(bool) {
        if ( READ == m_state) {
            m_output->verbose(CALL_INFO, 2, VANADIS_OS_DBG_SYSCALL, "[syscall-readlinkat] path: \"%s\"\n", m_filename.c_str());

            if ( 0 == strncmp( "/proc/self/exe", m_filename.c_str(), strlen("/proc/self/exe") ) ) {
                m_retval = strlen("/vanadis/exe");
                m_data.resize(m_retval);
                memcpy( m_data.data(), "/vanadis/exe", m_retval );
                m_state = WRITE;
                writeMemory( getEvent<VanadisSyscallReadLinkAtEvent*>()->getBuf(), m_data );
            } else {
                m_retval = readlinkat( m_dirFd, m_filename.c_str(), (char*) m_data.data(), m_data.size() );
                if ( -1 == m_retval ) {
                    assert(0);
                    setReturnFail(-errno);
                } else {
                    // terminate it for the debug printout
                    m_data[m_retval] = 0;
                    m_output->verbose(CALL_INFO, 2, VANADIS_OS_DBG_SYSCALL, "[syscall-readlinkat] buf: \"%s\"\n", m_data.data() );
                    m_data.resize(m_retval);
                    m_state = WRITE;
                    writeMemory( getEvent<VanadisSyscallReadLinkAtEvent*>()->getBuf(), m_data );
                }
            }
        } else {
            setReturnSuccess(m_retval);
        }
    }

private:
    int                             m_retval;
    int                             m_dirFd;
    std::string                     m_filename;
    std::vector<uint8_t>            m_data;
    enum { READ, WRITE }            m_state;
};

} // namespace Vanadis
} // namespace SST

#endif
