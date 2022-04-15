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

#ifndef _H_VANADIS_SPECULATE
#define _H_VANADIS_SPECULATE

#include "inst/vdelaytype.h"
#include "inst/vinst.h"

#define VANADIS_SPECULATE_JUMP_ADDR_ADD 4

namespace SST {
namespace Vanadis {

class VanadisSpeculatedInstruction : public VanadisInstruction
{

public:
    VanadisSpeculatedInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts, const uint64_t ins_w,
        const uint16_t c_phys_int_reg_in, const uint16_t c_phys_int_reg_out, const uint16_t c_isa_int_reg_in,
        const uint16_t c_isa_int_reg_out, const uint16_t c_phys_fp_reg_in, const uint16_t c_phys_fp_reg_out,
        const uint16_t c_isa_fp_reg_in, const uint16_t c_isa_fp_reg_out, const VanadisDelaySlotRequirement delayT) :
        VanadisInstruction(
            addr, hw_thr, isa_opts, c_phys_int_reg_in, c_phys_int_reg_out, c_isa_int_reg_in, c_isa_int_reg_out,
            c_phys_fp_reg_in, c_phys_fp_reg_out, c_isa_fp_reg_in, c_isa_fp_reg_out),
        ins_width(ins_w)
    {

        delayType = delayT;

        // default speculated, branch not-taken address
        speculatedAddress = getInstructionAddress() + ins_width;

        // speculatedAddress = (addr + 4);
        takenAddress = UINT64_MAX;
    }

    virtual uint64_t getSpeculatedAddress() const { return speculatedAddress; }
    virtual void     setSpeculatedAddress(const uint64_t spec_ad) { speculatedAddress = spec_ad; }
    virtual uint64_t getTakenAddress() const { return takenAddress; }
    virtual bool     isSpeculated() const { return true; }

    virtual VanadisFunctionalUnitType getInstFuncType() const { return INST_BRANCH; }

    virtual VanadisDelaySlotRequirement getDelaySlotType() const { return delayType; }
    uint64_t                            getInstructionWidth() const { return ins_width; }

protected:
    uint64_t calculateStandardNotTakenAddress()
    {
        uint64_t new_addr = getInstructionAddress();

        switch ( delayType ) {
        case VANADIS_CONDITIONAL_SINGLE_DELAY_SLOT:
        case VANADIS_SINGLE_DELAY_SLOT:
            new_addr += (ins_width * 2);
            break;
        case VANADIS_NO_DELAY_SLOT:
            new_addr += ins_width;
            break;
        }

        return new_addr;
    }

    VanadisDelaySlotRequirement delayType;
    uint64_t                    speculatedAddress;
    uint64_t                    takenAddress;
    uint64_t                    ins_width;
};

} // namespace Vanadis
} // namespace SST

#endif
