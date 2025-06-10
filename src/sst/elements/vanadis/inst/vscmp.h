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

#ifndef _H_VANADIS_SET_REG_COMPARE
#define _H_VANADIS_SET_REG_COMPARE

#include "inst/vcmptype.h"
#include "inst/vinst.h"
#include "util/vcmpop.h"

namespace SST {
namespace Vanadis {

template <VanadisRegisterCompareType compare_type, typename register_format>
class VanadisSetRegCompareInstruction : public virtual VanadisInstruction
{
public:
    VanadisSetRegCompareInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts, const uint16_t dest,
        const uint16_t src_1, const uint16_t src_2) :
        VanadisInstruction(addr, hw_thr, isa_opts, 2, 1, 2, 1, 0, 0, 0, 0)
    {
        isa_int_regs_in[0]  = src_1;
        isa_int_regs_in[1]  = src_2;
        isa_int_regs_out[0] = dest;
    }

    VanadisSetRegCompareInstruction* clone() override { return new VanadisSetRegCompareInstruction(*this); }

    VanadisFunctionalUnitType getInstFuncType() const override { return INST_INT_ARITH; }
    const char*               getInstCode() const override {
        if(std::is_same<register_format, uint64_t>::value) {
            return "CMPSETU64";
        } else if(std::is_same<register_format, int64_t>::value) {
            return "CMPSETI64";
        } else if(std::is_same<register_format, uint32_t>::value) {
            return "CMPSETU32";
        } else if(std::is_same<register_format, int32_t>::value) {
            return "CMPSETI32";
        } else {
            return "CMPSET";
        }
    }

    void printToBuffer(char* buffer, size_t buffer_size) override
    {
        snprintf(
            buffer, buffer_size,
            "CMPSET (op: %s) isa-out: %" PRIu16 " isa-in: %" PRIu16 ", %" PRIu16 " / phys-out: %" PRIu16
            " phys-in: %" PRIu16 ", %" PRIu16 "\n",
            convertCompareTypeToString(compare_type), isa_int_regs_out[0],
            isa_int_regs_in[0], isa_int_regs_in[1], phys_int_regs_out[0], phys_int_regs_in[0], phys_int_regs_in[1]);
    }

    void instOp(SST::Output* output,VanadisRegisterFile* regFile,
                            uint16_t phys_int_regs_out_0, uint16_t phys_int_regs_in_0,
                            uint16_t phys_int_regs_in_1)
    {

        const bool compare_result = registerCompare<compare_type, register_format>(
                    regFile, this, output, phys_int_regs_in_0, phys_int_regs_in_1);
        // always write result in unsigned 64b so we completely set register
        regFile->setIntReg<uint64_t>(phys_int_regs_out_0, compare_result ? 1 : 0);
    }

    virtual void scalarExecute(SST::Output* output, VanadisRegisterFile* regFile) override
    {

        uint16_t phys_int_regs_out_0 = getPhysIntRegOut(0);
        uint16_t phys_int_regs_in_0 = getPhysIntRegIn(0);
        uint16_t phys_int_regs_in_1 = getPhysIntRegIn(1);
        log(output, 16, 65535,phys_int_regs_out_0,phys_int_regs_in_0,phys_int_regs_in_1);
        instOp(output, regFile,phys_int_regs_out_0, phys_int_regs_in_0, phys_int_regs_in_1);
        markExecuted();
    }
};


} // namespace Vanadis
} // namespace SST

#endif
