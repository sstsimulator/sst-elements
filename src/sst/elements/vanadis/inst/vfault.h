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

#ifndef _H_VANADIS_INSTRUCTION_FAULT
#define _H_VANADIS_INSTRUCTION_FAULT

#include "decoder/visaopts.h"
#include "inst/vinst.h"
#include "inst/vinsttype.h"

namespace SST {
namespace Vanadis {

class VanadisInstructionFault : public VanadisInstruction
{
public:
    VanadisInstructionFault(
        const uint64_t address, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts, std::string msg) :
        VanadisInstruction(address, hw_thr, isa_opts, 0, 0, 0, 0, 0, 0, 0, 0)
    {

        flagError();
        fault_msg = msg;
    }

    VanadisInstructionFault(const uint64_t address, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts) :
        VanadisInstruction(address, hw_thr, isa_opts, 0, 0, 0, 0, 0, 0, 0, 0)
    {

        flagError();
        fault_msg = "";
    }

    VanadisInstruction* clone() override { return new VanadisInstructionFault(ins_address, hw_thread, isa_options); }

    const char* getInstCode() const override { return "FAULT"; }

    void printToBuffer(char* buffer, size_t buffer_size) override
    {
        snprintf(
            buffer, buffer_size, "%s (ins-addr: 0x%llx, %s)", getInstCode(), getInstructionAddress(),
            fault_msg.c_str());
    }

    VanadisFunctionalUnitType getInstFuncType() const override { return INST_FAULT; }
    void                      execute(SST::Output* output, VanadisRegisterFile* regFile) override {}

protected:
    std::string fault_msg;
};

} // namespace Vanadis
} // namespace SST

#endif
