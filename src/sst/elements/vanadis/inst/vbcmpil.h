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

#ifndef _H_VANADIS_BRANCH_REG_COMPARE_IMM_LINK
#define _H_VANADIS_BRANCH_REG_COMPARE_IMM_LINK

#include "inst/vcmptype.h"
#include "inst/vspeculate.h"

#include "util/vcmpop.h"

namespace SST {
namespace Vanadis {

class VanadisBranchRegCompareImmLinkInstruction : public VanadisSpeculatedInstruction {
public:
    VanadisBranchRegCompareImmLinkInstruction(const uint64_t addr, const uint32_t hw_thr,
                                              const VanadisDecoderOptions* isa_opts, const uint16_t src_1,
                                              const int64_t imm, const int64_t offst, const uint16_t link_reg,
                                              const VanadisDelaySlotRequirement delayT,
                                              const VanadisRegisterCompareType cType, const VanadisRegisterFormat fmt)
        : VanadisSpeculatedInstruction(addr, hw_thr, isa_opts, 1, 1, 1, 1, 0, 0, 0, 0, delayT), compareType(cType),
          imm_value(imm), offset(offst), reg_format(fmt) {

        isa_int_regs_in[0] = src_1;
        isa_int_regs_out[0] = link_reg;
    }

    VanadisBranchRegCompareImmLinkInstruction* clone() { return new VanadisBranchRegCompareImmLinkInstruction(*this); }

    virtual const char* getInstCode() const { return "BCMPIL"; }

    virtual void printToBuffer(char* buffer, size_t buffer_size) {
        snprintf(buffer, buffer_size,
                 "BCMPIL isa-in: %" PRIu16 " / phys-in: %" PRIu16 " / imm: %" PRId64 " / offset: %" PRId64
                 " / isa-link: %" PRIu16 " / phys-link: %" PRIu16 "\n",
                 isa_int_regs_in[0], phys_int_regs_in[0], imm_value, offset, isa_int_regs_out[0], phys_int_regs_out[0]);
    }

    virtual void execute(SST::Output* output, VanadisRegisterFile* regFile) {
#ifdef VANADIS_BUILD_DEBUG
        output->verbose(CALL_INFO, 16, 0,
                        "Execute: (addr=0x%0llx) BCMPIL isa-in: %" PRIu16 " / phys-in: %" PRIu16 " / imm: %" PRId64
                        " / offset: %" PRId64 " / isa-link: %" PRIu16 " / phys-link: %" PRIu16 "\n",
                        getInstructionAddress(), isa_int_regs_in[0], phys_int_regs_in[0], imm_value, offset,
                        isa_int_regs_out[0], phys_int_regs_out[0]);
#endif
        bool compare_result = false;

        switch (reg_format) {
        case VANADIS_FORMAT_INT64: {
            compare_result
                = registerCompareImm<int64_t>(compareType, regFile, this, output, phys_int_regs_in[0], imm_value);
        } break;
        case VANADIS_FORMAT_INT32: {
            compare_result = registerCompareImm<int32_t>(compareType, regFile, this, output, phys_int_regs_in[0],
                                                         static_cast<int32_t>(imm_value));
        } break;
        default: {
            flagError();
        } break;
        }

        if (compare_result) {
            takenAddress = (uint64_t)(((int64_t)getInstructionAddress()) + offset + VANADIS_SPECULATE_JUMP_ADDR_ADD);

            // Update the link address
            // The link address is the address of the second instruction after the
            // branch (so the instruction after the delay slot)
            uint64_t link_address = calculateStandardNotTakenAddress();
            regFile->setIntReg<uint64_t>(phys_int_regs_out[0], link_address);
        } else {
            takenAddress = calculateStandardNotTakenAddress();
        }

        markExecuted();
    }

protected:
    const int64_t offset;
    const int64_t imm_value;
    VanadisRegisterCompareType compareType;
    const VanadisRegisterFormat reg_format;
};

} // namespace Vanadis
} // namespace SST

#endif
