// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_VANADIS_OP_SYS
#define _H_VANADIS_OP_SYS

#include <sst/core/output.h>
#include <sst/core/subcomponent.h>

#include <functional>

#include "inst/isatable.h"
#include "inst/regfile.h"
#include "inst/vsyscall.h"
#include "os/callev/voscallall.h"

namespace SST {
namespace Vanadis {

enum VanadisCPUOSInitParameter { SYSCALL_INIT_PARAM_INIT_BRK };

class VanadisCPUOSHandler : public SST::SubComponent {
public:
    SST_ELI_REGISTER_SUBCOMPONENT_API(SST::Vanadis::VanadisCPUOSHandler)

    VanadisCPUOSHandler(ComponentId_t id, Params& params) : SubComponent(id) {

        const uint32_t verbosity = params.find<uint32_t>("verbose", 0);
        output = new SST::Output("[os]: ", verbosity, 0, Output::STDOUT);

        regFile = nullptr;
        isaTable = nullptr;
        tls_address = nullptr;

        hw_thr = 0;
        core_id = 0;
        tid = 0;
    }

    virtual ~VanadisCPUOSHandler() { delete output; }

    void setThreadLocalStoragePointer(uint64_t* new_tls_ptr) { tls_address = new_tls_ptr; }

    void setCoreID(const uint32_t newCoreID) { core_id = newCoreID; }
    void setHWThread(const uint32_t newThr) { hw_thr = newThr; }
    void setRegisterFile(VanadisRegisterFile* newFile) { regFile = newFile; }
    void setISATable(VanadisISATable* newTable) { isaTable = newTable; }
    void setHaltThreadCallback(std::function<void(uint32_t, int64_t)> cb) { haltThrCallBack = cb; }

    virtual void handleSysCall(VanadisSysCallInstruction* syscallIns) = 0;

    virtual void registerReturnCallback(std::function<void(uint32_t)>& new_call_back) {
        returnCallbacks.push_back(new_call_back);
    }

    virtual void registerInitParameter(VanadisCPUOSInitParameter paramType, void* param_val) = 0;

    void setThreadID(int64_t new_tid) { tid = new_tid; }
    int64_t getThreadID() const { return tid; }

protected:
    SST::Output* output;
    std::vector<std::function<void(uint32_t)>> returnCallbacks;
    uint32_t core_id;
    uint32_t hw_thr;

    std::function<void(uint32_t, int64_t)> haltThrCallBack;

    VanadisRegisterFile* regFile;
    VanadisISATable* isaTable;

    uint64_t* tls_address;
    int64_t tid;
};

} // namespace Vanadis
} // namespace SST

#endif
