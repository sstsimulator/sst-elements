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

#ifndef _H_VANADIS_OS_SYSCALL_STATX
#define _H_VANADIS_OS_SYSCALL_STATX

#include "os/syscall/syscall.h"
#include "os/callev/voscallstatx.h"

namespace SST {
namespace Vanadis {

class VanadisStatxSyscall : public VanadisSyscall {
public:
    VanadisStatxSyscall( VanadisNodeOSComponent* os, SST::Link* coreLink, OS::ProcessInfo* process, VanadisSyscallStatxEvent* event )
        : VanadisSyscall( os, coreLink, process, event, "statx" ) 
    {
        m_output->verbose( CALL_INFO, 16, 0, "[syscall-statx] -> dirFd=%d pathPtr=%#" PRIx32 ", flags=%#" PRIx32 ", mask=%#" PRIx32 ",stackPtr=%#" PRIx64 "\n", 
            event->getDirectoryFileDescriptor(), event->getPathPointer(), event->getFlags(), event->getMask(), event->getStackPtr() );
  
        if ( AT_FDCWD == event->getDirectoryFileDescriptor() ) {
            m_output->fatal(CALL_INFO, -1, "Error: statx does not support AT_FDCWD\n");
        }

        readString(event->getPathPointer(),m_filename);
    }

    void memReqIsDone() {
        m_output->verbose(CALL_INFO, 16, 0, "[syscall-open] path: \"%s\"\n", m_filename.c_str());

        if ( 0 == m_filename.compare("") ) {
            m_output->fatal(CALL_INFO, -1, "Error: statx does not support use of path\n");
        }
        if ( getEvent<VanadisSyscallStatxEvent*>()->getFlags() != 0x1000 ) {
            m_output->fatal(CALL_INFO, -1, "Error: statx only support AT_EMPTY_PATH\n");
        }
        setReturnFail( -EINVAL);
    }

private:
    std::string             m_filename;
    std::vector<uint8_t>    m_statBuf;
};

} // namespace Vanadis
} // namespace SST

#endif
