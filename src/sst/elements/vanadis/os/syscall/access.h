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

#ifndef _H_VANADIS_OS_SYSCALL_ACCESS
#define _H_VANADIS_OS_SYSCALL_ACCESS

#include "os/syscall/syscall.h"
#include "os/callev/voscallaccessev.h"

namespace SST {
namespace Vanadis {

class VanadisAccessSyscall : public VanadisSyscall {
public:
    VanadisAccessSyscall( VanadisNodeOSComponent* os, SST::Link* coreLink, OS::ProcessInfo* process, VanadisSyscallAccessEvent* event )
        : VanadisSyscall( os, coreLink, process, event, "access" ) 
    {
        m_output->verbose(CALL_INFO, 16, 0, "[syscall-access] access( 0x%llx, %" PRIu64 " )\n", event->getPathPointer(), event->getAccessMode());

        readString(event->getPathPointer(),m_filename);
    }

    void memReqIsDone() {
        m_output->verbose(CALL_INFO, 16, 0, "-> [syscall-access] path is: \"%s\"\n", m_filename.c_str());

        int ret = access( m_filename.c_str(), getEvent<VanadisSyscallAccessEvent*>()->getAccessMode() );
        if ( ret ) {
            setReturnFail( -errno );
        } else {
            setReturnSuccess( 0 );
        }
    }

 private:
    std::string m_filename;
};

} // namespace Vanadis
} // namespace SST

#endif
