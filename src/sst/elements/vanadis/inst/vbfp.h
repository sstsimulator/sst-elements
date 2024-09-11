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

#ifndef _H_VANADIS_BRANCH_FP
#define _H_VANADIS_BRANCH_FP

#include "inst/vspeculate.h"

namespace SST {
namespace Vanadis {

class VanadisBranchFPInstruction : public virtual VanadisSpeculatedInstruction
{
public:
    VanadisBranchFPInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts, const uint64_t ins_width,
        const uint16_t cond_reg, const int64_t offst, const bool branch_true,
        const VanadisDelaySlotRequirement delayT) :
        VanadisInstruction(addr, hw_thr, isa_opts, 0, 0, 0, 0, 1, 0, 1, 0),
        VanadisSpeculatedInstruction(addr, hw_thr, isa_opts, ins_width, 0, 0, 0, 0, 1, 0, 1, 0, delayT),
        branch_on_true(branch_true),
        offset(offst)
    {
        isa_fp_regs_in[0] = cond_reg;
    }

    VanadisBranchFPInstruction* clone() override { return new VanadisBranchFPInstruction(*this); }

    const char* getInstCode() const override
    {
        if ( branch_on_true ) { return "BFPT"; }
        else {
            return "BFPF";
        }
    }

    void log(SST::Output* output, int verboselevel, uint32_t sw_thr, bool compare_result, uint16_t phys_fp_regs_in_0)
    {
        #ifdef VANADIS_BUILD_DEBUG
        if(output->getVerboseLevel() >= verboselevel) {
            output->verbose(
                CALL_INFO, verboselevel, 0,
                "hw_thr=%d sw_thr = %d Execute: (addr=0x%" PRI_ADDR ") BFP%c isa-in: %" PRIu16 ", / phys-in: %" PRIu16 " / offset: %" PRId64 " -----> Taken? %c branch addr= 0x%" PRI_ADDR " \n",
                getHWThread(),sw_thr, getInstructionAddress(), branch_on_true ? 'T' : 'F', isa_fp_regs_in[0], phys_fp_regs_in_0, offset, (compare_result==true) ? 'Y' : 'N', takenAddress);
        }
        #endif
    }

    void printToBuffer(char* buffer, size_t buffer_size) override
    {
        snprintf(
            buffer, buffer_size, "BFP%c isa-in: %" PRIu16 ", / phys-in: %" PRIu16 " / offset: %" PRId64 "\n",
            branch_on_true ? 'T' : 'F', isa_fp_regs_in[0], phys_fp_regs_in[0], offset);
    }

    void instOp(VanadisRegisterFile* regFile, const uint16_t phys_fp_regs_in_0, bool* compare_result)
    {
        uint32_t       fp_cond_val = regFile->getFPReg<uint32_t>(phys_fp_regs_in_0);

        // is the CC code set on the compare bit in the status register?
        *compare_result = ((fp_cond_val & 0x800000) == (branch_on_true ? 0x800000 : 0));

        if ( *compare_result ) {
            takenAddress = (uint64_t)(((int64_t)getInstructionAddress()) + offset);
        }
        else {
            takenAddress = calculateStandardNotTakenAddress();
        }
    }

    void scalarExecute(SST::Output* output, VanadisRegisterFile* regFile) override
    {
        const uint16_t fp_cond_reg = phys_fp_regs_in[0];
        bool compare_result = false;
        instOp(regFile, fp_cond_reg, &compare_result);
        log(output, 16, 65535,compare_result,fp_cond_reg);
        markExecuted();
    }

protected:
    const int64_t offset;
    const bool    branch_on_true;
};

class VanadisSIMTBranchFPInstruction : public VanadisSIMTInstruction, public VanadisBranchFPInstruction
{
public:
    VanadisSIMTBranchFPInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts, const uint64_t ins_width,
        const uint16_t cond_reg, const int64_t offst, const bool branch_true,
        const VanadisDelaySlotRequirement delayT) :
        VanadisInstruction(addr, hw_thr, isa_opts, 0, 0, 0, 0, 1, 0, 1, 0),
        VanadisSIMTInstruction(addr, hw_thr, isa_opts, 0, 0, 0, 0, 1, 0, 1, 0),
        VanadisSpeculatedInstruction(addr, hw_thr, isa_opts, ins_width, 0, 0, 0, 0, 1, 0, 1, 0, delayT),
        VanadisBranchFPInstruction(addr, hw_thr, isa_opts, ins_width, cond_reg, offst, branch_true, delayT)
    {
        // isa_fp_regs_in[0] = cond_reg; 
    }

    VanadisSIMTBranchFPInstruction* clone() override { return new VanadisSIMTBranchFPInstruction(*this); }

    void simtExecute(SST::Output* output, VanadisRegisterFile* regFile) override
    {
        const uint16_t fp_cond_reg = getPhysFPRegIn(0, VanadisSIMTInstruction::sw_thread);
        bool compare_result = false;
        instOp(regFile,fp_cond_reg, &compare_result);
        log(output, 16, 65535,compare_result,fp_cond_reg);
    }
};

} // namespace Vanadis
} // namespace SST

#endif
