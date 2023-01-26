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

#ifndef _H_VANADIS_OS_SYSCALL_UNAME
#define _H_VANADIS_OS_SYSCALL_UNAME

#include "os/syscall/syscall.h"
#include "os/callev/voscalluname.h"

namespace SST {
namespace Vanadis {

class VanadisUnameSyscall : public VanadisSyscall {
public:
    VanadisUnameSyscall( VanadisNodeOSComponent* os, SST::Link* coreLink, OS::ProcessInfo* process, VanadisSyscallUnameEvent* event )
        : VanadisSyscall( os, coreLink, process, event, "uname" ) 
    {
        m_output->verbose(CALL_INFO, 16, 0, "[syscall-uname] ---> uname struct is at address 0x%0llx\n", event->getUnameInfoAddress());

        payload.resize(65 * 5, (uint8_t)'\0');

        const char* sysname = "Linux";
        const char* node = "sim.sstsimulator.org";
        const char* release = "4.19.136";
        const char* version = "#1 Wed Sep 2 23:59:59 MST 2020";
        const char* machine = "mips";

        for (size_t i = 0; i < std::strlen(sysname); ++i) {
            payload[i] = sysname[i];
        }

        for (size_t i = 0; i < std::strlen(node); ++i) {
            payload[65 + i] = node[i];
        }

        for (size_t i = 0; i < std::strlen(release); ++i) {
            payload[65 + 65 + i] = release[i];
        }

        for (size_t i = 0; i < std::strlen(version); ++i) {
            payload[65 + 65 + 65 + i] = version[i];
        }

        for (size_t i = 0; i < std::strlen(machine); ++i) {
            payload[65 + 65 + 65 + 65 + i] = machine[i];
        }

        writeMemory( event->getUnameInfoAddress(), payload );
        setReturnSuccess(0);
    }

 private:  
    std::vector<uint8_t> payload;
};

} // namespace Vanadis
} // namespace SST

#endif
