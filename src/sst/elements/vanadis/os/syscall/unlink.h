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

#ifndef _H_VANADIS_OS_SYSCALL_UNLINK
#define _H_VANADIS_OS_SYSCALL_UNLINK

#include "os/syscall/syscall.h"
#include "os/callev/voscallunlink.h"

namespace SST {
namespace Vanadis {

class VanadisUnlinkSyscall : public VanadisSyscall {
public:
    VanadisUnlinkSyscall( Output* output, Link* link, OS::ProcessInfo* process, SendMemReqFunc* func, VanadisSyscallUnlinkEvent* event )
        : VanadisSyscall( output, link, process, func, event, "unlink" ) 
    {
        m_output->verbose( CALL_INFO, 16, 0, "[syscall-unlink] -> call is unlink( %" PRId64 " )\n", event->getPathPointer());

        readString(event->getPathPointer(),filename);
    }

    void memReqIsDone() {
        m_output->verbose(CALL_INFO, 16, 0, "[syscall-unlink] path: \"%s\"\n", filename.c_str());

        if ( unlink( filename.c_str() ) ) {
            setReturnFail( -errno );
            char buf[100];
            strerror_r(errno,buf,100);
            m_output->verbose(CALL_INFO, 16, 0, "[syscall-unlink] unlink of %s failed, `%s`\n", filename.c_str(), buf );
        } else {
            setReturnSuccess( 0 );
        }
    }

 private:
    std::string filename;
};

} // namespace Vanadis
} // namespace SST

#endif
