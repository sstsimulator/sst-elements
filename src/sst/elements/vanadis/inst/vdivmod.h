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

#ifndef _H_VANADIS_DIV_REMAIN
#define _H_VANADIS_DIV_REMAIN

#include "inst/vinst.h"

namespace SST {
namespace Vanadis {

template<typename gpr_format>
class VanadisDivideRemainderInstruction : public VanadisInstruction
{
public:
    VanadisDivideRemainderInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts, const uint16_t quo_dest,
        const uint16_t remain_dest, const uint16_t src_1, const uint16_t src_2) :
        VanadisInstruction(addr, hw_thr, isa_opts, 2, 2, 2, 2, 0, 0, 0, 0)
    {

        isa_int_regs_in[0]  = src_1;
        isa_int_regs_in[1]  = src_2;
        isa_int_regs_out[0] = quo_dest;
        isa_int_regs_out[1] = remain_dest;
    }

    VanadisDivideRemainderInstruction* clone() override { return new VanadisDivideRemainderInstruction(*this); }
    VanadisFunctionalUnitType          getInstFuncType() const override { return INST_INT_DIV; }
    const char*                        getInstCode() const override { 
		if(sizeof(gpr_format)==8) {
			if(std::is_signed<gpr_format>::value) {
				return "DIVREM64";
			} else {
				return "DIVREMU64";
			}
		} else {
			if(std::is_signed<gpr_format>::value) {
				return "DIVREM32";
			} else {
				return "DIVREMU32";
			}
		}
	 }

    void printToBuffer(char* buffer, size_t buffer_size) override
    {
        snprintf(
            buffer, buffer_size,
            "%s q: %5" PRIu16 " r: %" PRIu16 " <- %" PRIu16 " \\ %" PRIu16 ", (phys: q: %" PRIu16 " r: %" PRIu16
            " r: %" PRIu16 " %" PRIu16 " )\n",
            getInstCode(), isa_int_regs_out[0], isa_int_regs_out[1], isa_int_regs_in[0],
            isa_int_regs_in[1], phys_int_regs_out[0], phys_int_regs_out[1], phys_int_regs_in[0], phys_int_regs_in[1]);
    }

    void execute(SST::Output* output, VanadisRegisterFile* regFile) override
    {
#ifdef VANADIS_BUILD_DEBUG
        output->verbose(
            CALL_INFO, 16, 0,
            "Execute: 0x%llx %s q: %" PRIu16 " r: %" PRIu16 " <- %" PRIu16 " \\ %" PRIu16 " (phys: q: %" PRIu16
            " r: %" PRIu16 " %" PRIu16 " %" PRIu16 ")\n",
            getInstructionAddress(), getInstCode(), isa_int_regs_out[0], isa_int_regs_out[1],
            isa_int_regs_in[0], isa_int_regs_in[1], phys_int_regs_out[0], phys_int_regs_out[1], phys_int_regs_in[0],
            phys_int_regs_in[1]);
#endif

		const gpr_format src_1 = regFile->getIntReg<gpr_format>(phys_int_regs_in[0]);
		const gpr_format src_2 = regFile->getIntReg<gpr_format>(phys_int_regs_in[1]);

		if(0 == src_2) {
			flagError();
		} else {
			const gpr_format quo = src_1 / src_2;
			const gpr_format mod = src_1 % src_2;

			if(std::is_signed<gpr_format>::value) {
				regFile->setIntReg<gpr_format>(phys_int_regs_out[0], quo, true);
				regFile->setIntReg<gpr_format>(phys_int_regs_out[1], mod, true);
			} else {
				regFile->setIntReg<gpr_format>(phys_int_regs_out[0], quo, false);
				regFile->setIntReg<gpr_format>(phys_int_regs_out[1], mod, false);
			}
		}

        markExecuted();
    }
};

} // namespace Vanadis
} // namespace SST

#endif
