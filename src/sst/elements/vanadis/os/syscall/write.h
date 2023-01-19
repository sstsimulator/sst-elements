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

#ifndef _H_VANADIS_OS_SYSCALL_WRITE
#define _H_VANADIS_OS_SYSCALL_WRITE

#include "os/syscall/syscall.h"
#include "os/callev/voscallwrite.h"

namespace SST {
namespace Vanadis {

class VanadisWriteSyscall : public VanadisSyscall {
public:
    VanadisWriteSyscall( VanadisNodeOSComponent* os, SST::Link* coreLink, OS::ProcessInfo* process, VanadisSyscallWriteEvent* event )
        : VanadisSyscall( os, coreLink, process, event, "write" ), m_numWritten(0)
    {
        m_output->verbose(CALL_INFO, 16, 0, "[syscall-write] -> call is write( %" PRId64 ", 0x%0llx, %" PRId64 " )\n",
                            event->getFileDescriptor(), event->getBufferAddress(), event->getBufferCount());

        m_fd = process->getFileDescriptor( event->getFileDescriptor());
        if ( -1 == m_fd ) {
            m_output->verbose(CALL_INFO, 16, 0,
                            "[syscall-write] -> file handle %" PRId64
                            " is not currently open, return an error code.\n",
                            event->getFileDescriptor());

            setReturnFail(-EINVAL);
        } else if (event->getBufferCount() == 0) {
            setReturnSuccess(0);
        } else {

            // get the length of the first memory access, it cannot span a cacheline  
            uint64_t length = vanadis_line_remainder(event->getBufferAddress(),64);
            length = event->getBufferCount() < length ? event->getBufferCount() : length;

            m_data.resize( length );
            readMemory( event->getBufferAddress(), m_data );
        }
    }

    void memReqIsDone() {

        int retval = write( m_fd, m_data.data(), m_data.size() );
        assert( retval >= 0 );
        m_numWritten += m_data.size();

        if ( m_numWritten == getEvent<VanadisSyscallWriteEvent*>()->getBufferCount() ) {
            setReturnSuccess( m_numWritten );
        } else {
            size_t length = getEvent<VanadisSyscallWriteEvent*>()->getBufferCount() - m_numWritten;
            if ( length > 64 ) { length = 64; }

            m_data.resize(length);
            readMemory( getEvent<VanadisSyscallWriteEvent*>()->getBufferAddress() + m_numWritten, m_data );
        }
    } 

 private:
    std::vector<uint8_t>    m_data;
    int                     m_fd;
    size_t                  m_numWritten;
};

} // namespace Vanadis
} // namespace SST

#endif
