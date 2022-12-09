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

#ifndef _H_VANADIS_OS_SYSCALL_MMAP
#define _H_VANADIS_OS_SYSCALL_MMAP

#include "os/syscall/syscall.h"
#include "os/callev/voscallmmap.h"

namespace SST {
namespace Vanadis {

class VanadisMmapSyscall : public VanadisSyscall {
public:
    VanadisMmapSyscall( Output* output, Link* link, OS::ProcessInfo* process, SendMemReqFunc* func, VanadisSyscallMemoryMapEvent* event )
        : VanadisSyscall( output, link, process, func, event, "mmap" )
    {
        uint64_t address = event->getAllocationAddress();
        uint64_t length = event->getAllocationLength();
        int64_t protect = event->getProtectionFlags();
        int64_t flags = event->getAllocationFlags();
        uint64_t call_stack = event->getStackPointer();
        uint64_t offset_units = event->getOffsetUnits();

        m_output->verbose(CALL_INFO, 16, 0, "[syscall-mmap] addr=%#" PRIx64 " length=%" PRIu64 " prot=%#" PRIx64 " flags=%#" PRIx64 " call_stack=%#" PRIx64 " offset_units=%" PRIu64 "\n",
                    address,length,protect,flags,call_stack,offset_units);

        if ( flags & ~( MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED ) ) {
            m_output->fatal(CALL_INFO, -1, "-> error: mmap() flag %#" PRIx64 " not supported\n",flags);
        }

        uint64_t addr = process->mmap( address, length, protect, flags, offset_units );

        if ( 0 == addr ) {
            setReturnFail( 0 );
        } else {
            setReturnSuccess( addr );
        }
    }
};

} // namespace Vanadis
} // namespace SST

#endif
