// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_VANADIS_PC_ADDI
#define _H_VANADIS_PC_ADDI

#include "inst/vinst.h"

namespace SST {
namespace Vanadis {

template <typename gpr_format>
class VanadisPCAddImmInstruction : public virtual VanadisInstruction
{
public:
    VanadisPCAddImmInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts, const uint16_t dest,
        const gpr_format immediate) :
        VanadisInstruction(addr, hw_thr, isa_opts, 0, 1, 0, 1, 0, 0, 0, 0),
        imm_value(immediate)
    {

        isa_int_regs_out[0] = dest;
    }

    VanadisPCAddImmInstruction* clone() override { return new VanadisPCAddImmInstruction(*this); }
    VanadisFunctionalUnitType   getInstFuncType() const override { return INST_INT_ARITH; }
    const char*                 getInstCode() const override { 
		if(sizeof(gpr_format) == 8) {
			if(std::is_signed<gpr_format>::value) {
				return "PCADDI64";
			} else {
				return "PCADDIU64";
			}
		} else {
			if(std::is_signed<gpr_format>::value) {
				return "PCADDI32";
			} else {
				return "PCADDIU32";
			}
		}
	}

    void printToBuffer(char* buffer, size_t buffer_size) override
    {
        snprintf(
            buffer, buffer_size,
            "%s %5" PRIu16 " <- 0x%" PRI_ADDR " + imm=%" PRId64 " (phys: %5" PRIu16 " <- 0x%" PRI_ADDR " + %" PRId64 ") = 0x%" PRI_ADDR "",
            getInstCode(), isa_int_regs_out[0], getInstructionAddress(), imm_value, phys_int_regs_out[0], getInstructionAddress(),
            imm_value, getInstructionAddress() + imm_value);
    }

    void log(SST::Output* output, int verboselevel, uint16_t sw_thr, 
                            uint16_t phys_int_regs_out_0)
    {
        #ifdef VANADIS_BUILD_DEBUG
        output->verbose(
            CALL_INFO, verboselevel, 0,
            "hw_thr=%d sw_thr = %d Execute: 0x%" PRI_ADDR " %s phys: out=%" PRIu16 " in=0x%" PRI_ADDR " / imm=%" PRId64 ", isa: out=%" PRIu16
            " = 0x%" PRI_ADDR "\n",
            getHWThread(),sw_thr,getInstructionAddress(), getInstCode(), phys_int_regs_out_0, getInstructionAddress(), imm_value,
            isa_int_regs_out[0], (static_cast<int64_t>(getInstructionAddress()) + imm_value));
        #endif
    }

    void instOp(VanadisRegisterFile* regFile, uint16_t phys_int_regs_out_0)
    {
		const gpr_format pc = static_cast<gpr_format>(getInstructionAddress());
		regFile->setIntReg<gpr_format>(phys_int_regs_out_0, (pc + imm_value) & 0xffffffff);
    }

    void scalarExecute(SST::Output* output, VanadisRegisterFile* regFile) override
    {
        
        uint16_t phys_int_regs_out_0 = getPhysIntRegOut(0);
        instOp(regFile, phys_int_regs_out_0);
        log(output, 16, 65535, phys_int_regs_out_0);
        markExecuted();
    }

private:
    const gpr_format imm_value;
};


template <typename gpr_format>
class VanadisSIMTPCAddImmInstruction : public VanadisSIMTInstruction, public VanadisPCAddImmInstruction<gpr_format>
{
public:
    VanadisSIMTPCAddImmInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts, const uint16_t dest,
        const gpr_format immediate) :
        VanadisInstruction(addr, hw_thr, isa_opts, 0, 1, 0, 1, 0, 0, 0, 0),
        VanadisSIMTInstruction(addr, hw_thr, isa_opts, 0, 1, 0, 1, 0, 0, 0, 0),
        VanadisPCAddImmInstruction<gpr_format>(addr, hw_thr, isa_opts, dest, immediate)
    {
        ;
    }

    VanadisSIMTPCAddImmInstruction* clone() override { return new VanadisSIMTPCAddImmInstruction(*this); }

    void simtExecute(SST::Output* output, VanadisRegisterFile* regFile) override
    {
        uint16_t phys_int_regs_out_0 = getPhysIntRegOut(0, VanadisSIMTInstruction::sw_thread);
        VanadisPCAddImmInstruction<gpr_format>::instOp(regFile, phys_int_regs_out_0);
        VanadisPCAddImmInstruction<gpr_format>::log(output, 16, VanadisSIMTInstruction::sw_thread, phys_int_regs_out_0);
    }
};
} // namespace Vanadis
} // namespace SST

#endif
