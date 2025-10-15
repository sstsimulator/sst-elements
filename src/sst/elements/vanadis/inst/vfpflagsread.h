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

#ifndef _H_VANADIS_FP_FPFLAGS_READ
#define _H_VANADIS_FP_FPFLAGS_READ

#include "inst/vfpinst.h"
#include "inst/vregfmt.h"
#include "util/vfpreghandler.h"

#include <cstdint>
#include <type_traits>

namespace SST {
namespace Vanadis {

template<bool copy_round_mode, bool shift_round_mode, bool copy_fp_flags>
class VanadisFPFlagsReadInstruction : public VanadisFloatingPointInstruction
{
public:
    VanadisFPFlagsReadInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts,
        VanadisFloatingPointFlags* fpflags, const uint16_t dest) :
        VanadisInstruction(
            addr, hw_thr, isa_opts, 0, 1, 0, 1, 0, 0, 0, 0),
		VanadisFloatingPointInstruction(
            addr, hw_thr, isa_opts, fpflags, 0, 1, 0, 1, 0, 0, 0, 0)
    {
			static_assert( !((copy_round_mode && (!shift_round_mode)) && copy_fp_flags), "Cannot copy round, not shift and copy FP flags\n");

			isa_int_regs_out[0] = dest;
    }

    VanadisFPFlagsReadInstruction*  clone() override { return new VanadisFPFlagsReadInstruction(*this); }
    VanadisFunctionalUnitType getInstFuncType() const override { return INST_FP_ARITH; }

    const char* getInstCode() const override
    {
			return "FPFLGRD";
    }

    void printToBuffer(char* buffer, size_t buffer_size) override
    {
        snprintf(buffer, buffer_size, "%s %" PRIu16 " <- FPFLAGS (phys: %" PRIu16 ")\n",
					getInstCode(), isa_int_regs_out[0], phys_int_regs_out[0]);
    }

	void log(SST::Output* output, int verboselevel, uint16_t sw_thr, uint64_t flags_out, uint16_t phys_int_regs_out_0 )
	{
		if(output->getVerboseLevel() >= verboselevel) {
				output->verbose(CALL_INFO, verboselevel, 0, "hw_thr=%d sw_thr = %d Execute: 0x%" PRI_ADDR " %s out-reg: %" PRIu16 " / out-mask: 0x%" PRI_ADDR " / copy_round: %c / shift_round: %c / copy_fp: %c\n",
						getHWThread(),sw_thr, getInstructionAddress(), getInstCode(), phys_int_regs_out_0, flags_out,
						copy_round_mode ? 'y' : 'n', shift_round_mode ? 'y' : 'n', copy_fp_flags ? 'y' : 'n');
			}
	}

	void instOp( VanadisRegisterFile* regFile, uint64_t* flags_out, uint16_t phys_int_regs_out_0)
	{


		if(copy_round_mode)
		{
				*flags_out = convertRoundingToInteger(pipeline_fpflags->getRoundingMode());

				if(shift_round_mode) {
					*flags_out <<= 5;
				}
		}

		if(copy_fp_flags) {
			*flags_out |= pipeline_fpflags->inexact() ? 0x1 : 0x0;
			*flags_out |= pipeline_fpflags->underflow() ? 0x2 : 0x0;
			*flags_out |= pipeline_fpflags->overflow() ? 0x4 : 0x0;
			*flags_out |= pipeline_fpflags->divZero() ? 0x8 : 0x0;
			*flags_out |= pipeline_fpflags->invalidOp() ? 0x10 : 0x0;
		}
		regFile->setIntReg<uint64_t>(phys_int_regs_out_0, *flags_out);
	}

    void scalarExecute(SST::Output* output, VanadisRegisterFile* regFile) override
    {
		if(checkFrontOfROB())
		{
		  	uint64_t flags_out = 0;
			uint16_t phys_int_regs_out_0 = getPhysIntRegOut(0);
			instOp(regFile, &flags_out, phys_int_regs_out_0);
			log(output, 16, 65535, flags_out, phys_int_regs_out_0);
			markExecuted();
		}
    }
};


} // namespace Vanadis
} // namespace SST

#endif
