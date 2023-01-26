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

#ifndef _H_VANADIS_OS_SYSCALL_OPEN
#define _H_VANADIS_OS_SYSCALL_OPEN

#include "os/syscall/syscall.h"
#include "os/callev/voscallopen.h"

namespace SST {
namespace Vanadis {

class VanadisOpenSyscall : public VanadisSyscall {
public:
    VanadisOpenSyscall( VanadisNodeOSComponent* os, SST::Link* coreLink, OS::ProcessInfo* process, VanadisSyscallOpenEvent* event )
        : VanadisSyscall( os, coreLink, process, event, "open" )
    {
        m_output->verbose( CALL_INFO, 16, 0, "[syscall-open] -> call is open( %" PRId64 " )\n", event->getPathPointer());

        readString(event->getPathPointer(),m_filename);
    }

    void memReqIsDone() {
        m_output->verbose(CALL_INFO, 16, 0, "[syscall-open] path: \"%s\"\n", m_filename.c_str());

        int fd = m_process->openFile( m_filename.c_str(), getEvent<VanadisSyscallOpenEvent*>()->getFlags(), getEvent<VanadisSyscallOpenEvent*>()->getMode() );
        if ( fd < 0 ) {
            m_output->verbose(CALL_INFO, 16, 0, "[syscall-open] open of %s failed\n", m_filename.c_str() );
            setReturnFail(fd);
        }else{
            m_output->verbose(CALL_INFO, 16, 0, "[syscall-open] open of %s succeeded\n", m_filename.c_str() );
            setReturnSuccess(fd);
        }
    }

 private:
    std::string                 m_filename;
};

} // namespace Vanadis
} // namespace SST

#endif
