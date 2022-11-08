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

#ifndef _H_VANADIS_ADDI
#define _H_VANADIS_ADDI

#include "inst/vinst.h"

namespace SST {
namespace Vanadis {

template<typename gpr_format>
class VanadisAddImmInstruction : public VanadisInstruction
{
public:
    VanadisAddImmInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts, const uint16_t dest,
        const uint16_t src_1, const gpr_format immediate) :
        VanadisInstruction(addr, hw_thr, isa_opts, 1, 1, 1, 1, 0, 0, 0, 0),
        imm_value(immediate)
    {
        isa_int_regs_in[0]  = src_1;
        isa_int_regs_out[0] = dest;
    }

    VanadisAddImmInstruction* clone() override { return new VanadisAddImmInstruction(*this); }
    VanadisFunctionalUnitType getInstFuncType() const override { return INST_INT_ARITH; }
    const char*               getInstCode() const override {
			if(sizeof(gpr_format) == 8) {
				if(std::is_signed<gpr_format>::value) {
					return "ADDI64";
				} else{
					return "ADDIU64";
				}
			} else if(sizeof(gpr_format) == 4) {
				if(std::is_signed<gpr_format>::value) {
					return "ADDI32";
				} else{
					return "ADDIU32";
				}
			} else {
				return "ADDIUNK";
			}
	}

    void printToBuffer(char* buffer, size_t buffer_size) override
    {
        snprintf(
            buffer, buffer_size,
            "%s %5" PRIu16 " <- %5" PRIu16 " + imm=%" PRId64 " (phys: %5" PRIu16 " <- %5" PRIu16 " + %" PRId64 ")",
            getInstCode(), isa_int_regs_out[0], isa_int_regs_in[0], imm_value, phys_int_regs_out[0], phys_int_regs_in[0], imm_value);
    }

    void execute(SST::Output* output, VanadisRegisterFile* regFile) override
    {
        const gpr_format src_1 = regFile->getIntReg<gpr_format>(phys_int_regs_in[0]);
        const gpr_format result = src_1 + imm_value;

#ifdef VANADIS_BUILD_DEBUG
        if(std::is_unsigned<gpr_format>::value) {
            output->verbose(
                CALL_INFO, 16, 0,
                "Execute: 0x%llx %s phys: out=%" PRIu16 " in=%" PRIu16 " imm=%" PRId64 ", isa: out=%" PRIu16
                " / in=%" PRIu16 " (%" PRIu64 " + %" PRIu64 " = %" PRIu64 ")\n",
                getInstructionAddress(), getInstCode(), phys_int_regs_out[0], phys_int_regs_in[0], imm_value, isa_int_regs_out[0],
                isa_int_regs_in[0], src_1, imm_value, result);
        } else {
            output->verbose(
                CALL_INFO, 16, 0,
                "Execute: 0x%llx %s phys: out=%" PRIu16 " in=%" PRIu16 " imm=%" PRId64 ", isa: out=%" PRIu16
                " / in=%" PRIu16 " (%" PRId64 " + %" PRId64 " = %" PRId64 ")\n",
                getInstructionAddress(), getInstCode(), phys_int_regs_out[0], phys_int_regs_in[0], imm_value, isa_int_regs_out[0],
                isa_int_regs_in[0], src_1, imm_value, result);
        }
#endif

		regFile->setIntReg<gpr_format>(phys_int_regs_out[0], result);

      markExecuted();
    }

private:
    const gpr_format imm_value;
};

} // namespace Vanadis
} // namespace SST

#endif
