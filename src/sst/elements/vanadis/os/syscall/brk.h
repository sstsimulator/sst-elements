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

#ifndef _H_VANADIS_OS_SYSCALL_BRK
#define _H_VANADIS_OS_SYSCALL_BRK

#include "os/syscall/syscall.h"
#include "os/callev/voscallbrk.h"

namespace SST {
namespace Vanadis {

class VanadisBrkSyscall : public VanadisSyscall {
public:
    VanadisBrkSyscall( Output* output, Link* link, OS::ProcessInfo* process, SendMemReqFunc* func, VanadisSyscallBRKEvent* event )
        : VanadisSyscall( output, link, process, func, event, "brk" )
    {
        uint64_t brk = process->getBrk();
        if (event->getUpdatedBRK() < brk) {

            m_output->verbose(CALL_INFO, 16, 0,
                                "[syscall-brl]: brk provided (0x%llx) is less than existing brk "
                                "point (0x%llx), so returning current brk point\n",
                                event->getUpdatedBRK(), brk);
        } else {
            if ( ! process->setBrk( event->getUpdatedBRK() ) ) {
                m_output->fatal(CALL_INFO, -1, "-> error: brk() %#" PRIx64 " ran out of memory\n",event->getUpdatedBRK());
            }

            m_output->verbose(CALL_INFO, 16, 0,
                                "[syscall-brk] old brk: 0x%llx -> new brk: 0x%llx (diff: %" PRIu64 ")\n", brk,
                                event->getUpdatedBRK(), (event->getUpdatedBRK() - brk));
            process->printRegions( "after brk" );
        }

        // Linux syscall for brk returns the BRK point on success
        setReturnSuccess( process->getBrk() );
    }
};

} // namespace Vanadis
} // namespace SST

#endif
