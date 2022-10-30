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

#ifndef _H_VANADIS_CONDITIONAL_MOVE
#define _H_VANADIS_CONDITIONAL_MOVE

#include "inst/vinst.h"
#include "inst/vcmptype.h"
#include "util/vcmpop.h"

namespace SST {
namespace Vanadis {

template<typename compare_type, typename reg_type, VanadisRegisterCompareType compare_op>
class VanadisConditionalMoveImmInstruction : public VanadisInstruction
{
public:
    VanadisConditionalMoveImmInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts, const uint16_t dest,
        const uint16_t src_1, const uint16_t src_2, const compare_type imm_value) :
        VanadisInstruction(addr, hw_thr, isa_opts, 2, 1, 2, 1, 0, 0, 0, 0), imm(imm_value)
    {
        isa_int_regs_in[0]  = src_1;
        isa_int_regs_in[1]  = src_2;
        isa_int_regs_out[0] = dest;
    }

    virtual VanadisConditionalMoveImmInstruction* clone() { return new VanadisConditionalMoveImmInstruction(*this); }

    virtual VanadisFunctionalUnitType getInstFuncType() const { return INST_INT_ARITH; }

    virtual const char* getInstCode() const { return "CMOVI"; }

    virtual void printToBuffer(char* buffer, size_t buffer_size)
    {
        snprintf(
            buffer, buffer_size,
            "CMOVI    %5" PRIu16 " <- %" PRIu16 " if %" PRIu16 " == %" PRId64 " { %" PRIu16 " <- %" PRIu16 " if %" PRIu16 " == %" PRId64 " }\n",
            isa_int_regs_out[0], isa_int_regs_in[0], isa_int_regs_in[1], imm,
            phys_int_regs_out[0], phys_int_regs_in[0],
            phys_int_regs_in[1], imm);
    }

    virtual void execute(SST::Output* output, VanadisRegisterFile* regFile)
    {
#ifdef VANADIS_BUILD_DEBUG
        output->verbose(
            CALL_INFO, 16, 0,
            "Execute: CMOVI    inst: 0x%llx / %5" PRIu16 " <- %" PRIu16 " if %" PRIu16 " == %" PRId64 " { %" PRIu16 " <- %" PRIu16 " if %" PRIu16 " == %" PRId64 " }\n",
            getInstructionAddress(), isa_int_regs_out[0], isa_int_regs_in[0], isa_int_regs_in[1], imm,
            phys_int_regs_out[0], phys_int_regs_in[0],
            phys_int_regs_in[1], imm);
#endif
        const reg_type src_1 = regFile->getIntReg<reg_type>(phys_int_regs_in[0]);
        const bool compare_result = registerCompareImm<compare_op, compare_type>(
            regFile, this, output, phys_int_regs_in[1], imm);

        if(compare_result) {
            regFile->setIntReg<reg_type>(phys_int_regs_out[0], src_1);
        }

        markExecuted();
    }

protected:
    const compare_type imm;
};

} // namespace Vanadis
} // namespace SST

#endif
