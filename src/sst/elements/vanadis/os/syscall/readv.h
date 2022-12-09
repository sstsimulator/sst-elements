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

#ifndef _H_VANADIS_OS_SYSCALL_READV
#define _H_VANADIS_OS_SYSCALL_READV

#include "os/syscall/syscall.h"
#include "os/callev/voscallreadv.h"
#include "os/syscall/iovec.h"

namespace SST {
namespace Vanadis {

class VanadisReadvSyscall : public VanadisIoVecSyscall {
public:
    VanadisReadvSyscall( Output* output, Link* link, OS::ProcessInfo* process, SendMemReqFunc* func, VanadisSyscallReadvEvent* event )
        : VanadisIoVecSyscall( output, link, process, func, event, "readv" ), m_eof( false )
    { }

    void startIoVecTransfer() {
        m_output->verbose(CALL_INFO, 16, 0,"\n");
        m_currentVecOffset = 0;
        readSomeData();
    } 

    void readSomeData() {
        m_dataBuffer.resize( calcBuffSize() );

        m_output->verbose(CALL_INFO, 16, 0,"%zu\n", m_dataBuffer.size());

        ssize_t numRead = read( m_fd, m_dataBuffer.data(), m_dataBuffer.size());
        assert( numRead >= 0 );
        m_totalBytes += numRead;

        // if we have not filled the vector and have not hit EOF
        if ( numRead < m_dataBuffer.size() ) {
            m_eof = true;
            m_output->verbose(CALL_INFO, 16, 0,"reached EOF %zu\n",m_totalBytes);
        }

        m_currentVecOffset += m_dataBuffer.size();

        m_output->verbose(CALL_INFO, 16, 0,"numRead=%zu totalWritten=%zu currentVecOffset=%zu vecLength=%zu\n",
            numRead,m_totalBytes, m_currentVecOffset, m_ioVecTable->getLength(m_currentVec));

        writeMemory( m_ioVecTable->getAddr(m_currentVec), m_dataBuffer ); 
    }

    void ioVecWork() {

        if ( m_eof ) {
            m_output->verbose(CALL_INFO, 16, 0,"return success total written %zu\n",m_totalBytes);
            setReturnSuccess( m_totalBytes );
        } else if ( m_currentVecOffset < m_ioVecTable->getLength(m_currentVec) ) {
            m_output->verbose(CALL_INFO, 16, 0,"read %zu bytes\n",m_dataBuffer.size());
            readSomeData();
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
  private:
    bool m_eof;
};

} // namespace Vanadis
} // namespace SST

#endif
