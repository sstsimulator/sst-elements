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

#ifndef _H_VANADIS_TRUNCATE
#define _H_VANADIS_TRUNCATE

#include "inst/vinst.h"

namespace SST {
namespace Vanadis {

template <VanadisRegisterFormat input_format, VanadisRegisterFormat output_format>
class VanadisTruncateInstruction : public VanadisInstruction
{
public:
    VanadisTruncateInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts, const uint16_t dest,
        const uint16_t src) :
        VanadisInstruction(addr, hw_thr, isa_opts, 1, 1, 1, 1, 0, 0, 0, 0)
    {

        isa_int_regs_in[0]  = src;
        isa_int_regs_out[0] = dest;
    }

    VanadisTruncateInstruction* clone() override { return new VanadisTruncateInstruction(*this); }
    VanadisFunctionalUnitType   getInstFuncType() const override { return INST_INT_ARITH; }
    const char*                 getInstCode() const override { return "TRUNC"; }

    void printToBuffer(char* buffer, size_t buffer_size) override
    {
        snprintf(
            buffer, buffer_size, "%s    %5" PRIu16 " <- %5" PRIu16 " (phys: %5" PRIu16 " <- %5" PRIu16 ")\n",
            getInstCode(), isa_int_regs_out[0], isa_int_regs_in[0], phys_int_regs_out[0], phys_int_regs_in[0]);
    }

    void scalarExecute(SST::Output* output, VanadisRegisterFile* regFile) override
    {
        #ifdef VANADIS_BUILD_DEBUG
        output->verbose(
            CALL_INFO, 16, 0,
            "Execute: (addr=%p) %s phys: out=%" PRIu16 " in=%" PRIu16 ", isa: out=%" PRIu16 " / in=%" PRIu16 "\n",
            (void*)getInstructionAddress(), getInstCode(), phys_int_regs_out[0], phys_int_regs_in[0],
            isa_int_regs_out[0], isa_int_regs_in[0]);
        #endif
        switch ( input_format ) {
        case VanadisRegisterFormat::VANADIS_FORMAT_INT64:
        {
            switch ( output_format ) {
            case VanadisRegisterFormat::VANADIS_FORMAT_INT64:
            {
                const int64_t src = regFile->getIntReg<int64_t>(phys_int_regs_in[0]);
                regFile->setIntReg<int64_t>(phys_int_regs_out[0], src);
            } break;
            case VanadisRegisterFormat::VANADIS_FORMAT_INT32:
            {
                const uint32_t src = regFile->getIntReg<int32_t>(phys_int_regs_in[0]);
                regFile->setIntReg<int32_t>(phys_int_regs_out[0], src);
            } break;
            default:
            {
                flagError();
            } break;
            }
        } break;
        case VanadisRegisterFormat::VANADIS_FORMAT_INT32:
        {
            switch ( output_format ) {
            case VanadisRegisterFormat::VANADIS_FORMAT_INT64:
            {
                const int64_t src = regFile->getIntReg<int64_t>(phys_int_regs_in[0]);
                regFile->setIntReg<int64_t>(phys_int_regs_out[0], src);
            } break;
            case VanadisRegisterFormat::VANADIS_FORMAT_INT32:
            {
                const int32_t src = regFile->getIntReg<int32_t>(phys_int_regs_in[0]);
                regFile->setIntReg<int32_t>(phys_int_regs_out[0], src);
            } break;
            default:
            {
                flagError();
            } break;
            }
        } break;
        default:
        {
            flagError();
        } break;
        }

        markExecuted();
    }
};

} // namespace Vanadis
} // namespace SST

#endif
