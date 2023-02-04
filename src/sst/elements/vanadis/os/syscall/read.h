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

#ifndef _H_VANADIS_OS_SYSCALL_READ
#define _H_VANADIS_OS_SYSCALL_READ

#include "os/syscall/syscall.h"
#include "os/callev/voscallread.h"

namespace SST {
namespace Vanadis {

class VanadisReadSyscall : public VanadisSyscall {
public:
    VanadisReadSyscall( VanadisNodeOSComponent* os, SST::Link* coreLink, OS::ProcessInfo* process, VanadisSyscallReadEvent* event  )
        : VanadisSyscall( os, coreLink, process, event, "read" ), m_numRead(0), m_eof(false)
    {
        m_output->verbose(CALL_INFO, 16, 0, "-> call is read( %" PRId64 ", 0x%0llx, %" PRId64 " )\n",
                            event->getFileDescriptor(), event->getBufferAddress(), event->getBufferCount());

        m_fd = process->getFileDescriptor( event->getFileDescriptor());
        if (-1 == m_fd ) {
            m_output->verbose(CALL_INFO, 16, 0,
                                "[syscall-read] -> file handle %" PRId64
                                " is not currently open, return an error code.\n",
                                event->getFileDescriptor());

            setReturnFail( -EINVAL);
        } else if (event->getBufferCount() == 0) {
            setReturnSuccess(0);
        } else {
            // get the length of the first memory access, it cannot span a cacheline  
            uint64_t length = vanadis_line_remainder(event->getBufferAddress(),64);
            length = event->getBufferCount() < length ? event->getBufferCount() : length;
            readChunk( length ); 
        }
    }

    void readChunk(size_t length) {

        m_data.resize(length);
        ssize_t retval = read( m_fd, m_data.data(), length );
        assert(retval>=0);
        m_data.resize( retval );

        writeMemory( getEvent<VanadisSyscallReadEvent*>()->getBufferAddress() + m_numRead, m_data );

        m_numRead += retval;

        if ( retval < length ) {
            m_eof = true;
        }
    }

    bool handleMemResp( StandardMem::Request* req ) {
        delete req; // we don't use the request

        if ( m_eof || m_numRead == getEvent<VanadisSyscallReadEvent*>()->getBufferCount() ) {
            setReturnSuccess( m_numRead );
        } else {
            size_t length = getEvent<VanadisSyscallReadEvent*>()->getBufferCount() - m_numRead;  
            if ( length > 64 ) { length = 64; }
            readChunk( length );
        }
        return false;
    } 

 private:
    std::vector<uint8_t>    m_data;
    bool                    m_eof;
    int                     m_fd;
    size_t                  m_numRead;
};

} // namespace Vanadis
} // namespace SST

#endif
