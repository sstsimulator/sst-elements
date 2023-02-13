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

#ifndef _H_VANADIS_OS_SYSCALL_IOVEC
#define _H_VANADIS_OS_SYSCALL_IOVEC

#include "os/syscall/syscall.h"

namespace SST {
namespace Vanadis {

class VanadisIoVecSyscall : public VanadisSyscall {
    enum State { ReadIoVecTable, IoVecTransfer };

class IoVecTable {
  public:
    IoVecTable( VanadisOSBitType bit_type, unsigned count ) : m_type(bit_type), m_count(count) {

        m_data.resize( 2 * count * ( m_type == VanadisOSBitType::VANADIS_OS_32B ? sizeof(uint32_t): sizeof(uint64_t) ));
    }

    std::vector<uint8_t>& getDataVec() { return m_data; }

    uint64_t getAddr( unsigned pos ) {
        switch(m_type) {
          case VanadisOSBitType::VANADIS_OS_32B:
            return (uint64_t) ((uint32_t*)m_data.data())[pos*2];

          case VanadisOSBitType::VANADIS_OS_64B:
            return (uint64_t) ((uint64_t*)m_data.data())[pos*2];
          default:
            throw -1;
        }
    }

    size_t getLength( unsigned pos ) {
        switch(m_type) {
          case VanadisOSBitType::VANADIS_OS_32B:
            return (uint64_t) ((uint32_t*)m_data.data())[ (pos * 2) + 1];

          case VanadisOSBitType::VANADIS_OS_64B:
            return (uint64_t) ((uint64_t*)m_data.data())[ (pos * 2) + 1];
          default:
            throw -1;
        }
    }

  private:

    std::vector<uint8_t>    m_data;
    VanadisOSBitType        m_type;
    unsigned                m_count;
};

public:
    VanadisIoVecSyscall( VanadisNodeOSComponent* os, SST::Link* coreLink, OS::ProcessInfo* process, VanadisSyscallIoVecEvent* event, std::string name  )
        : VanadisSyscall( os, coreLink, process, event, name ), m_ioVecTable(nullptr), m_currentVec(0), m_totalBytes(0), m_state(ReadIoVecTable)
    {

        m_output->verbose(CALL_INFO, 16, 0,
                            "[syscall-%s] -> call is writev( %" PRId64 ", 0x%0llx, %" PRId64 " )\n",
                            event->getOperation() == SYSCALL_OP_WRITEV ? "writev" : "readv",
                            event->getFileDescriptor(), event->getIOVecAddress(), event->getIOVecCount());

        m_fd = process->getFileDescriptor( event->getFileDescriptor() );
        if ( -1 == m_fd ) {
            m_output->verbose(CALL_INFO, 16, 0,
                                "[syscall-%s] -> file handle %" PRId64
                                " is not currently open, return an error code.\n",
                                event->getOperation() == SYSCALL_OP_WRITEV ? "writev" : "readv",
                                event->getFileDescriptor());

            setReturnFail(-EINVAL);
         } else if (event->getIOVecCount() < 0) {
            setReturnFail(-EINVAL);
         } else if (event->getIOVecCount() == 0) {
            setReturnSuccess(0);
        } else {
            m_dataBuffer.reserve( 4096 );
            m_ioVecTable = new IoVecTable(  event->getOSBitType(), event->getIOVecCount() );
            readMemory( event->getIOVecAddress(), m_ioVecTable->getDataVec() );
        }
    }

    ~VanadisIoVecSyscall() { 
        if ( m_ioVecTable ) { delete m_ioVecTable; }
    }

  protected:

    void memReqIsDone() {
        if ( ReadIoVecTable == m_state ) {
            m_output->verbose(CALL_INFO, 16, 0,"read ioVecTable complete\n");
            m_state = IoVecTransfer;
            for ( int i =0; i < getEvent<VanadisSyscallIoVecEvent*>()->getIOVecCount(); i++ ) {
                m_output->verbose(CALL_INFO, 16, 0,"addr=%#" PRIx64 " length=%zu\n",m_ioVecTable->getAddr(i),m_ioVecTable->getLength(i));
            }
            if ( findNonZeroIoVec() )  {
                startIoVecTransfer();
            } else {
                setReturnSuccess( m_totalBytes );
            }
        } else {
            ioVecWork();
        }
    }

    virtual void startIoVecTransfer() = 0;
    virtual void ioVecWork() = 0;

    size_t calcBuffSize() {
        size_t retval = 4096;
        if ( m_ioVecTable->getLength(m_currentVec) - m_currentVecOffset < 4096 ) { 
            retval = m_ioVecTable->getLength(m_currentVec) - m_currentVecOffset;
        }
        m_output->verbose(CALL_INFO, 16, 0,"buff size %zu\n", retval);
        return retval;
    }

    bool findNonZeroIoVec() { 
        m_output->verbose(CALL_INFO, 16, 0,"count=%" PRIu64 "\n",getEvent<VanadisSyscallIoVecEvent*>()->getIOVecCount());
        while ( m_currentVec < getEvent<VanadisSyscallIoVecEvent*>()->getIOVecCount() ) {
            if ( m_ioVecTable->getLength(m_currentVec) ) {
                m_output->verbose(CALL_INFO, 16, 0,"found pos=%d\n",m_currentVec);
                return true;
            }
            ++m_currentVec;
        }
        return false;
    }

  protected: 
    int                         m_currentVec;
    IoVecTable*                 m_ioVecTable;
    size_t                      m_currentVecOffset;
    int                         m_fd;
    size_t                      m_totalBytes;
    std::vector<uint8_t>        m_dataBuffer;

  private:

    State                       m_state;
};

} // namespace Vanadis
} // namespace SST

#endif
