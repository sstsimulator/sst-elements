// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_VANADIS_OS_SYSCALL_MMAP
#define _H_VANADIS_OS_SYSCALL_MMAP

#include "os/syscall/syscall.h"
#include "os/callev/voscallmmap.h"
#include "os/include/device.h"

namespace SST {
namespace Vanadis {

class VanadisMmapSyscall : public VanadisSyscall {
public:
    VanadisMmapSyscall( VanadisNodeOSComponent* os, SST::Link* coreLink, OS::ProcessInfo* process, VanadisSyscallMemoryMapEvent* event )
        : VanadisSyscall( os, coreLink, process, event, "mmap" )
    {
        uint64_t address = event->getAllocationAddress();
        uint64_t length = event->getAllocationLength();
        int64_t protect = event->getProtectionFlags();
        int64_t flags = event->getAllocationFlags();
        auto fd = event->getFd();
        auto offset = event->getOffset();
        uint64_t call_stack = event->getStackPointer();
        uint64_t offset_units = event->getOffsetUnits();

        m_output->verbose(CALL_INFO, 2, VANADIS_OS_DBG_SYSCALL, "[syscall-mmap] addr=%#" PRIx64 " length=%#" PRIx64 " prot=%#" PRIx64 " flags=%#" PRIx64
                " fd=%d offset=%" PRIu64 " call_stack=%#" PRIx64 " offset_units=%" PRIu64 "\n",
                    address,length,protect,flags,fd,offset,call_stack,offset_units);

        // if that stackAddr is valid  we need to get mmap() arguments that are not in registers
        if ( event->getStackPointer() ) {
            if ( event->getOSBitType() == VanadisOSBitType::VANADIS_OS_32B ) {
                // int fd, off_t offset
                m_buffer.resize(sizeof(uint32_t)*2);
            } else {
                m_buffer.resize(sizeof(uint64_t)*2);
            }
            readMemory( event->getStackPointer() + 16, m_buffer );

        } else {
            mmap( address, length, protect, flags, fd, offset_units );
        }
    }

    void mmap( uint64_t address, uint64_t length, int protect, int flags, int fd, uint64_t offset ) {

        if ( flags & ~( MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED ) ) {
            m_output->fatal(CALL_INFO, -1, "-> error: mmap() flag %#" PRIx32 " not supported\n",flags);
        }

        OS::Device* dev = nullptr;
        if ( -1000 == fd ) {
            dev = m_os->getDevice( fd );
	    length = dev->getLength();
        }

        uint64_t addr = m_process->mmap( address, length, protect, flags, dev, offset );

        if ( 0 == addr ) {
            setReturnFail( 0 );
        } else {
            setReturnSuccess( addr );
        }
    }

    void memReqIsDone(bool) {
        auto event = getEvent<VanadisSyscallMemoryMapEvent*>();
        uint32_t* tmp  = (uint32_t*)m_buffer.data();
        uint64_t address = event->getAllocationAddress();
        uint64_t length = event->getAllocationLength();
        int64_t protect = event->getProtectionFlags();
        int64_t flags = event->getAllocationFlags();
        m_output->verbose(CALL_INFO, 16, VANADIS_OS_DBG_SYSCALL, "mmap memReqIsDone [syscall-mmap] fd=%d offset=%" PRIu32 "\n", tmp[0],tmp[1]);

        mmap( address, length, protect, flags, tmp[0], tmp[1] );
    }

private:
    std::vector<uint8_t> m_buffer;
};

} // namespace Vanadis
} // namespace SST

#endif
