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

#ifndef _H_VANADIS_OS_SYSCALL_WRITEV
#define _H_VANADIS_OS_SYSCALL_WRITEV

#include "os/syscall/syscall.h"
#include "os/callev/voscallwritev.h"
#include "os/syscall/iovec.h"

namespace SST {
namespace Vanadis {

class VanadisWritevSyscall : public VanadisIoVecSyscall {
    enum State { ReadIoVecTable, ReadIoVec };
public:
    VanadisWritevSyscall( VanadisNodeOSComponent* os, SST::Link* coreLink, OS::ProcessInfo* process, VanadisSyscallWritevEvent* event )
        : VanadisIoVecSyscall( os, coreLink, process, event, "writev" )
    { }

    void startIoVecTransfer() { 
        m_currentVecOffset = 0;

        m_dataBuffer.resize( calcBuffSize() );
        m_output->verbose(CALL_INFO, 16, 0,"pos=%i length=%zu buffer length=%zu\n", m_currentVec, m_ioVecTable->getLength(m_currentVec)  ,m_dataBuffer.size());

        readMemory( m_ioVecTable->getAddr(m_currentVec), m_dataBuffer ); 
    }

    void ioVecWork() { 
        ssize_t numWritten = write( m_fd, m_dataBuffer.data(), m_dataBuffer.size());
        assert( numWritten >= 0 );
        m_totalBytes += numWritten;
        m_currentVecOffset += m_dataBuffer.size();

        m_output->verbose(CALL_INFO, 16, 0,"numWriten=%zu totalWritten=%zu currentVecOffset=%zu vecLength=%zu\n",
            numWritten,m_totalBytes, m_currentVecOffset, m_ioVecTable->getLength(m_currentVec));

        if ( m_currentVecOffset < m_ioVecTable->getLength(m_currentVec)  ) {
            m_dataBuffer.resize( calcBuffSize() );
            m_output->verbose(CALL_INFO, 16, 0,"read %zu bytes\n", m_dataBuffer.size());
            readMemory( m_ioVecTable->getAddr(m_currentVec) + m_currentVecOffset, m_dataBuffer ); 
        } else {
            m_output->verbose(CALL_INFO, 16, 0,"vector %d is complete \n",m_currentVec);
            ++m_currentVec;
            if ( findNonZeroIoVec() ) {
                startIoVecTransfer(); 
            } else {
                m_output->verbose(CALL_INFO, 16, 0,"return success total written %zu\n",m_totalBytes);
                setReturnSuccess( m_totalBytes );
            }
        }
    }
};

} // namespace Vanadis
} // namespace SST

#endif
