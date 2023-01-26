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

#ifndef _H_VANADIS_OS_SYSCALL_FSTAT
#define _H_VANADIS_OS_SYSCALL_FSTAT

#include "os/voscallev.h"
#include "os/syscall/syscall.h"

namespace SST {
namespace Vanadis {

class VanadisFstatSyscall : public VanadisSyscall {
public:
    VanadisFstatSyscall( Output* output, OS::ProcessInfo* process, VanadisSyscallFstatEvent* event, SST::Link* coreLink )
        : VanadisSyscall( output, process, event, coreLink, "fstat" ) 
    {
        assert(0);
#if 0 // not implemented
        case SYSCALL_OP_FSTAT: {
            new VanadisFstatSyscall( output, core_link, &m_sendMemReqFunc, convertEvent<VanadisSyscallFstatEvent*>( "fstat", sys_ev ) );
            output->verbose(CALL_INFO, 16, 0, "-> call is fstat()\n");
            VanadisSyscallFstatEvent* fstat_ev = dynamic_cast<VanadisSyscallFstatEvent*>(sys_ev);

            if (nullptr == fstat_ev) {
                output->fatal(CALL_INFO, -1, "-> error: unable to case syscall to a fstat event.\n");
            }

            output->verbose(CALL_INFO, 16, 0, "[syscall-fstat] ---> file-descriptor: %" PRId32 "\n",
                            fstat_ev->getFileHandle());
            output->verbose(CALL_INFO, 16, 0, "[syscall-fstat] ---> stat-struct-addr: %" PRIu64 " / 0x%llx\n",
                            fstat_ev->getStructAddress(), fstat_ev->getStructAddress());

            bool success = false;
            struct vanadis_stat32 v32_stat_output;
            struct stat stat_output;
            int fstat_return_code = 0;

            if (fstat_ev->getFileHandle() <= 2) {
                if (0 == fstat(fstat_ev->getFileHandle(), &stat_output)) {
                    vanadis_copy_native_stat(&stat_output, &v32_stat_output);
                    success = true;
                } else {
                    fstat_return_code = -13;
                }
            } else {
                auto fd = process->getFileDescriptor( fstat_ev->getFileHandle() );

                if ( -1 == fd ) {
                    // Fail - cannot find the descriptor, so its not open
                    fstat_return_code = -9;
                } else {

                    if (0 == fstat(fd, &stat_output)) {
                        vanadis_copy_native_stat(&stat_output, &v32_stat_output);
                        success = true;
                    } else {
                        fstat_return_code = -13;
                    }
                }
            }

            if (success) {

                std::vector<uint8_t> stat_write_payload;
                stat_write_payload.reserve(sizeof(v32_stat_output));

                uint8_t* stat_output_ptr = (uint8_t*)(&v32_stat_output);
                for (int i = 0; i < sizeof(stat_output); ++i) {
                    stat_write_payload.push_back(stat_output_ptr[i]);
                }

                SyscallEntry* entry = new SyscallEntry(core_link);
                entry->handler_state = new VanadisFstatHandlerState(output->getVerboseLevel(), fstat_ev->getFileHandle(),
                                                             fstat_ev->getStructAddress());

                sendBlockToMemory(entry, fstat_ev->getStructAddress(), stat_write_payload);
            } else {
                output->verbose(CALL_INFO, 16, 0, "[syscall-fstat] - response is operation failed code: %" PRId32 "\n",
                                fstat_return_code);

                VanadisSyscallResponse* resp = new VanadisSyscallResponse(fstat_return_code);
                resp->markFailed();

                core_link->send(resp);
            }
        } break;
#endif

    }
};

} // namespace Vanadis
} // namespace SST

#endif
