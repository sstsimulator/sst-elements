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

#ifndef _H_VANADIS_BRANCH_FP
#define _H_VANADIS_BRANCH_FP

#include "inst/vspeculate.h"

namespace SST {
namespace Vanadis {

class VanadisBranchFPInstruction : public VanadisSpeculatedInstruction {
public:
    VanadisBranchFPInstruction(const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts,
										 const uint64_t ins_width,
                               const uint16_t cond_reg, const int64_t offst, const bool branch_true,
                               const VanadisDelaySlotRequirement delayT)
        : VanadisSpeculatedInstruction(addr, hw_thr, isa_opts, ins_width, 0, 0, 0, 0, 1, 0, 1, 0, delayT),
          branch_on_true(branch_true), offset(offst) {

        isa_fp_regs_in[0] = cond_reg;
    }

    VanadisBranchFPInstruction* clone() override { return new VanadisBranchFPInstruction(*this); }

    const char* getInstCode() const override {
        if (branch_on_true) {
            return "BFPT";
        } else {
            return "BFPF";
        }
    }

    void printToBuffer(char* buffer, size_t buffer_size) override {
        snprintf(buffer, buffer_size, "BFP%c isa-in: %" PRIu16 ", / phys-in: %" PRIu16 " / offset: %" PRId64 "\n",
                 branch_on_true ? 'T' : 'F', isa_fp_regs_in[0], phys_fp_regs_in[0], offset);
    }

    void execute(SST::Output* output, VanadisRegisterFile* regFile) override {
#ifdef VANADIS_BUILD_DEBUG
        output->verbose(
            CALL_INFO, 16, 0,
            "Execute: (addr=0x%llx) BFP%c isa-in: %" PRIu16 ", / phys-in: %" PRIu16 " / offset: %" PRId64 "\n",
            getInstructionAddress(), branch_on_true ? 'T' : 'F', isa_fp_regs_in[0], phys_fp_regs_in[0], offset);
#endif
        const uint16_t fp_cond_reg = phys_fp_regs_in[0];
        uint32_t fp_cond_val = regFile->getFPReg<uint32_t>(fp_cond_reg);

        // is the CC code set on the compare bit in the status register?
        const bool compare_result = ((fp_cond_val & 0x800000) == (branch_on_true ? 0x800000 : 0));

        if (compare_result) {
            takenAddress = (uint64_t)(((int64_t)getInstructionAddress()) + offset);

            output->verbose(CALL_INFO, 16, 0, "-----> taken-address: 0x%llx + %" PRId64 " = 0x%llx\n",
                            getInstructionAddress(), offset, takenAddress);
        } else {
            takenAddress = calculateStandardNotTakenAddress();

            output->verbose(CALL_INFO, 16, 0, "-----> not-taken-address: 0x%llx\n", takenAddress);
        }

        markExecuted();
    }

protected:
    const int64_t offset;
    const bool branch_on_true;
};

} // namespace Vanadis
} // namespace SST

#endif
