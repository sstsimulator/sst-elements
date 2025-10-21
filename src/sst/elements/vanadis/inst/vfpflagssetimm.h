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

#ifndef _H_VANADIS_FP_FPFLAGS_SET_IMM
#define _H_VANADIS_FP_FPFLAGS_SET_IMM

#include "inst/vfpinst.h"
#include "inst/vregfmt.h"
#include "util/vfpreghandler.h"

#include <cstdint>
#include <type_traits>

namespace SST {
namespace Vanadis {


template<bool SetFRM, bool SetFFLAGS>
class VanadisFPFlagsSetImmInstruction : public VanadisFloatingPointInstruction
{
public:
    VanadisFPFlagsSetImmInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts,
        VanadisFloatingPointFlags* fpflags, const uint64_t imm, int mode) :
        VanadisInstruction(
            addr, hw_thr, isa_opts, 0, 0, 0, 0, 0, 0, 0, 0),
        VanadisFloatingPointInstruction(
            addr, hw_thr, isa_opts, fpflags, 0, 0, 0, 0, 0, 0, 0, 0),
			   imm_value(imm), mode(mode)
    {}

    VanadisFPFlagsSetImmInstruction*  clone() override { return new VanadisFPFlagsSetImmInstruction(*this); }
    VanadisFunctionalUnitType getInstFuncType() const override { return INST_FP_ARITH; }

    const char* getInstCode() const override
    {
			return "FPFLGSETI";
    }

    void printToBuffer(char* buffer, size_t buffer_size) override
    {
        snprintf(buffer, buffer_size, "%s FPFLAGS <- imm: %" PRIu64 "\n",
					getInstCode(), imm_value);
    }

    void log(SST::Output* output, int verboselevel, uint16_t sw_thr)
    {
        if(output->getVerboseLevel() >= verboselevel) {
				output->verbose(CALL_INFO, verboselevel, 0, "hw_thr=%d sw_thr = %d Execute: 0x%" PRI_ADDR " %s FPFLAGS <- mask = %" PRIu64 " (0x%" PRI_ADDR ")\n",
					getHWThread(),sw_thr, getInstructionAddress(), getInstCode(), imm_value, imm_value);
			}
    }




    void instOp()
    {
			updateFP_flags<SetFRM,SetFFLAGS>( imm_value, mode );
    }

    void scalarExecute(SST::Output* output, VanadisRegisterFile* regFile) override
    {
		if(checkFrontOfROB()) {
			log(output, 16, 65535);

			instOp();

			markExecuted();
		}
    }

protected:
	const uint64_t imm_value;
    const int mode;

};

} // namespace Vanadis
} // namespace SST

#endif
