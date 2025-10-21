// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_VANADIS_OP_SYS
#define _H_VANADIS_OP_SYS

#include <cstdint>
#include <sst/core/output.h>
#include <sst/core/link.h>
#include <sst/core/subcomponent.h>

#include <sys/fcntl.h>
#include <sys/mman.h>

#include <functional>
#include <tuple>

#include "inst/isatable.h"
#include "inst/regfile.h"
#include "inst/vsyscall.h"
#include "os/callev/voscallall.h"
#include "os/vstartthreadreq.h"
#include "os/resp/voscallresp.h"

namespace SST {
namespace Vanadis {

enum VanadisCPUOSInitParameter { SYSCALL_INIT_PARAM_INIT_BRK };

class VanadisCPUOSHandler : public SST::SubComponent {
public:
    SST_ELI_REGISTER_SUBCOMPONENT_API(SST::Vanadis::VanadisCPUOSHandler)

    VanadisCPUOSHandler(ComponentId_t id, Params& params) : SubComponent(id) {

        const uint32_t verbosity = params.find<uint32_t>("verbose", 0);
        output_ = new SST::Output("[os_hdlr]:@p() ", verbosity, 1, Output::STDOUT);

        reg_file_ = nullptr;
        isa_table_ = nullptr;
        tls_address_ = nullptr;

        hw_thr_ = 0;
        core_id_ = 0;
    }

    virtual ~VanadisCPUOSHandler() { delete output_; }

    void setThreadLocalStoragePointer(uint64_t* new_tls_ptr) { tls_address_ = new_tls_ptr; }

    void setCoreID(const uint32_t newCoreID) { core_id_ = newCoreID; }
    void setHWThread(const uint32_t newThr) { hw_thr_ = newThr; }
    void setRegisterFile(VanadisRegisterFile* newFile) { reg_file_ = newFile; }
    void setISATable(VanadisISATable* newTable) { isa_table_ = newTable; }

    virtual std::tuple<bool,bool> handleSysCall(VanadisSysCallInstruction* syscallIns) = 0;
    virtual void recvSyscallResp( VanadisSyscallResponse* os_resp ) = 0;

    void setOS_link( SST::Link* link ) {
        os_link_ = link;
    }

protected:

    void sendSyscallEvent( VanadisSyscallEvent* ev ) {
        os_link_->send( ev );
    }

    void sendEvent( Event* ev ) {
        os_link_->send( ev );
    }

    SST::Output* output_;
    uint32_t core_id_;
    uint32_t hw_thr_;

    VanadisRegisterFile* reg_file_;
    VanadisISATable* isa_table_;

    uint64_t* tls_address_;

private:
    SST::Link* os_link_;
};

} // namespace Vanadis
} // namespace SST

#endif
