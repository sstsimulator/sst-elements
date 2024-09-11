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

#ifndef _H_VANADIS_FP_FPFLAGS_SET
#define _H_VANADIS_FP_FPFLAGS_SET

#include "inst/vfpinst.h"
#include "inst/vregfmt.h"
#include "util/vfpreghandler.h"

#include <type_traits>

namespace SST {
namespace Vanadis {

template<bool SetFRM, bool SetFFLAGS >
class VanadisFPFlagsSetInstruction : public VanadisFloatingPointInstruction
{
public:
    VanadisFPFlagsSetInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts,
        VanadisFloatingPointFlags* fpflags, const uint16_t src_1, int mode) :
        VanadisInstruction(
            addr, hw_thr, isa_opts, 1, 0, 1, 0, 0, 0, 0, 0),
        VanadisFloatingPointInstruction(
            addr, hw_thr, isa_opts, fpflags, 1, 0, 1, 0, 0, 0, 0, 0), mode(mode)
    {
		isa_int_regs_in[0] = src_1;
    }

    VanadisFPFlagsSetInstruction*  clone() override { return new VanadisFPFlagsSetInstruction(*this); }
    VanadisFunctionalUnitType getInstFuncType() const override { return INST_FP_ARITH; }

    const char* getInstCode() const override
    {
			return "FPFLGSET";
    }

    void printToBuffer(char* buffer, size_t buffer_size) override
    {
        snprintf(buffer, buffer_size, "%s FPFLAGS <- %" PRIu16 " (phys: %" PRIu16 ")\n",
					getInstCode(), isa_int_regs_in[0], phys_int_regs_in[0]);
    }

    void log(SST::Output* output, int verboselevel, uint16_t sw_thr, 
                            uint16_t phys_int_regs_in_0, uint64_t mask_in)
    {
        if(output->getVerboseLevel() >= verboselevel) {
				output->verbose(CALL_INFO, verboselevel, 0, "hw_thr=%d sw_thr = %d Execute: 0x%" PRI_ADDR " %s in-reg: %" PRIu16 " / phys: %" PRIu16 " -> mask = %" PRIu64 " (0x%" PRI_ADDR ")\n",
					getHWThread(),sw_thr, getInstructionAddress(), getInstCode(), isa_int_regs_in[0], phys_int_regs_in_0, mask_in, mask_in);
			}
    }
    
    void instOp(VanadisRegisterFile* regFile, uint16_t phys_int_regs_in_0, uint64_t* mask_in)
    {
        *mask_in = regFile->getIntReg<uint64_t>(phys_int_regs_in_0);    
		updateFP_flags<SetFRM,SetFFLAGS>( *mask_in, mode );
    }

    void scalarExecute(SST::Output* output, VanadisRegisterFile* regFile) override
    {
		if(checkFrontOfROB()) {
            uint16_t phys_int_regs_in_0 = getPhysIntRegIn(0);
            uint64_t mask_in = 0;
			instOp(regFile, phys_int_regs_in_0, &mask_in);
            log(output, 16, 65535, phys_int_regs_in_0, mask_in);
			markExecuted();
		} else {
			output->verbose(CALL_INFO, 16, 0, "hw_thr=%d, sw_thr=%d, not front of ROB for ins: 0x%" PRI_ADDR " %s\n", getHWThread(), 65535,  getInstructionAddress(), getInstCode());
		}
    }
protected:
    const int mode;
};



template<bool SetFRM, bool SetFFLAGS >
class VanadisSIMTFPFlagsSetInstruction : public VanadisSIMTInstruction, public VanadisFPFlagsSetInstruction<SetFRM, SetFFLAGS>
{
public:
    VanadisSIMTFPFlagsSetInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts,
        VanadisFloatingPointFlags* fpflags, const uint16_t src_1, int mode) :
        VanadisInstruction(
            addr, hw_thr, isa_opts, 1, 0, 1, 0, 0, 0, 0, 0),
        VanadisSIMTInstruction(
            addr, hw_thr, isa_opts, 1, 0, 1, 0, 0, 0, 0, 0),
        VanadisFPFlagsSetInstruction<SetFRM, SetFFLAGS>(
            addr, hw_thr, isa_opts, fpflags, src_1, mode)
    {
        ;
    }

    VanadisSIMTFPFlagsSetInstruction*  clone() override { return new VanadisSIMTFPFlagsSetInstruction(*this); }
    
    void simtExecute(SST::Output* output, VanadisRegisterFile* regFile) override
    {
		if(checkFrontOfROB()) {
            uint16_t phys_int_regs_in_0 = getPhysIntRegIn(0, VanadisSIMTInstruction::sw_thread);
            uint64_t mask_in = 0;
			VanadisFPFlagsSetInstruction<SetFRM, SetFFLAGS>::instOp(regFile, phys_int_regs_in_0, &mask_in);
            VanadisFPFlagsSetInstruction<SetFRM, SetFFLAGS>::log(output, 16, VanadisSIMTInstruction::sw_thread, phys_int_regs_in_0, mask_in);
		} else {
			output->verbose(CALL_INFO, 16, 0, "hw_thr=%d, sw_thr=%d, not front of ROB for ins: 0x%" PRI_ADDR " %s\n", getHWThread(), VanadisSIMTInstruction::sw_thread,  getInstructionAddress(), getInstCode());
		}
    }
};

} // namespace Vanadis
} // namespace SST

#endif
