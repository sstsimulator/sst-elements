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

#ifndef _H_VANADIS_OS_SYSCALL_READLINK
#define _H_VANADIS_OS_SYSCALL_READLINK

#include "os/syscall/syscall.h"
#include "os/callev/voscallreadlink.h"

namespace SST {
namespace Vanadis {

class VanadisReadlinkSyscall : public VanadisSyscall {
public:
    VanadisReadlinkSyscall( VanadisNodeOSComponent* os, SST::Link* coreLink, OS::ProcessInfo* process, VanadisSyscallReadLinkEvent* event )
        : VanadisSyscall( os, coreLink, process, event, "readlink" ), m_data( event->getBufferSize() ), m_done(false)
    {
        m_output->verbose(CALL_INFO, 16, 0, "[syscall-readlink] -> readlink( 0x%0llx, 0x%llx, %" PRId64 " )\n",
                                event->getPathPointer(), event->getBufferPointer(),
                                event->getBufferSize());

        readString(event->getPathPointer(),m_filename);
    }

    void memReqIsDone() {
        if ( ! m_done ) {
            m_done = true;
            m_output->verbose(CALL_INFO, 16, 0, "[syscall-readlink] path: \"%s\"\n", m_filename.c_str());

            int ret = readlink( m_filename.c_str(), (char*) m_data.data(), m_data.size() );
            if ( -1 == ret ) {
                setReturnFail(-errno);
            } else {
                m_data.resize(ret);
                writeMemory( getEvent<VanadisSyscallReadLinkEvent*>()->getBufferPointer(), m_data );
                setReturnSuccess(ret);
            }
        }
    }

private:
    std::string                     m_filename;
    std::vector<uint8_t>            m_data;
    bool                            m_done;
};

} // namespace Vanadis
} // namespace SST

#endif
