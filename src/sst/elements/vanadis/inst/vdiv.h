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

#ifndef _H_VANADIS_DIV
#define _H_VANADIS_DIV

#include "inst/vinst.h"

namespace SST {
namespace Vanadis {

template <VanadisRegisterFormat register_format, bool perform_signed>
class VanadisDivideInstruction : public VanadisInstruction
{
public:
    VanadisDivideInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts, const uint16_t dest,
        const uint16_t src_1, const uint16_t src_2) :
        VanadisInstruction(addr, hw_thr, isa_opts, 2, 1, 2, 1, 0, 0, 0, 0)
    {

        isa_int_regs_in[0]  = src_1;
        isa_int_regs_in[1]  = src_2;
        isa_int_regs_out[0] = dest;
    }

    VanadisDivideInstruction* clone() override { return new VanadisDivideInstruction(*this); }
    VanadisFunctionalUnitType getInstFuncType() const override { return INST_INT_DIV; }

    const char* getInstCode() const override
    {
        if ( perform_signed ) { return "DIV"; }
        else {
            return "DIVU";
        }
    }

    void printToBuffer(char* buffer, size_t buffer_size) override
    {
        snprintf(
            buffer, buffer_size,
            "%s    %5" PRIu16 " <- %5" PRIu16 " + %5" PRIu16 " (phys: %5" PRIu16 " <- %5" PRIu16 " + %5" PRIu16 ")",
            getInstCode(), isa_int_regs_out[0], isa_int_regs_in[0], isa_int_regs_in[1], phys_int_regs_out[0],
            phys_int_regs_in[0], phys_int_regs_in[1]);
    }

    void execute(SST::Output* output, VanadisRegisterFile* regFile) override
    {
#ifdef VANADIS_BUILD_DEBUG
        output->verbose(
            CALL_INFO, 16, 0,
            "Execute: (addr=%p) %s phys: out=%" PRIu16 " in=%" PRIu16 ", %" PRIu16 ", isa: out=%" PRIu16
            " / in=%" PRIu16 ", %" PRIu16 "\n",
            (void*)getInstructionAddress(), getInstCode(), phys_int_regs_out[0], phys_int_regs_in[0],
            phys_int_regs_in[1], isa_int_regs_out[0], isa_int_regs_in[0], isa_int_regs_in[1]);
#endif

        if ( VanadisRegisterFormat::VANADIS_FORMAT_INT64 == register_format ) {
            if ( perform_signed ) {
                const int64_t src_1 = regFile->getIntReg<int64_t>(phys_int_regs_in[0]);
                const int64_t src_2 = regFile->getIntReg<int64_t>(phys_int_regs_in[1]);

                if ( 0 == src_2 ) { flagError(); }
                else {
                    regFile->setIntReg<int64_t>(phys_int_regs_out[0], ((src_1) / (src_2)));
                }
            }
            else {
                const uint64_t src_1 = regFile->getIntReg<uint64_t>(phys_int_regs_in[0]);
                const uint64_t src_2 = regFile->getIntReg<uint64_t>(phys_int_regs_in[1]);

                if ( 0 == src_2 ) { flagError(); }
                else {
                    regFile->setIntReg<uint64_t>(phys_int_regs_out[0], ((src_1) / (src_2)));
                }
            }
        }
        else if ( VanadisRegisterFormat::VANADIS_FORMAT_INT32 == register_format ) {
            if ( perform_signed ) {
                const int32_t src_1 = regFile->getIntReg<int32_t>(phys_int_regs_in[0]);
                const int32_t src_2 = regFile->getIntReg<int32_t>(phys_int_regs_in[1]);

                if ( 0 == src_2 ) { flagError(); }
                else {
                    regFile->setIntReg<int32_t>(phys_int_regs_out[0], ((src_1) / (src_2)));
                }
            }
            else {
                const uint32_t src_1 = regFile->getIntReg<uint32_t>(phys_int_regs_in[0]);
                const uint32_t src_2 = regFile->getIntReg<uint32_t>(phys_int_regs_in[1]);

                if ( 0 == src_2 ) { flagError(); }
                else {
                    regFile->setIntReg<uint32_t>(phys_int_regs_out[0], ((src_1) / (src_2)));
                }
            }
        }
        else {
            flagError();
        }

        markExecuted();
    }
};

} // namespace Vanadis
} // namespace SST

#endif
