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

#ifndef _H_VANADIS_SRLI
#define _H_VANADIS_SRLI

#include "inst/vinst.h"
#include "inst/vregfmt.h"

namespace SST {
namespace Vanadis {

template <VanadisRegisterFormat register_format>
class VanadisShiftRightLogicalImmInstruction : public virtual VanadisInstruction
{
public:
    VanadisShiftRightLogicalImmInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts, const uint16_t dest,
        const uint16_t src_1, const uint64_t immediate) :
        VanadisInstruction(addr, hw_thr, isa_opts, 1, 1, 1, 1, 0, 0, 0, 0)
    {

        isa_int_regs_in[0]  = src_1;
        isa_int_regs_out[0] = dest;

        imm_value = immediate;
    }

    VanadisShiftRightLogicalImmInstruction* clone() override
    {
        return new VanadisShiftRightLogicalImmInstruction(*this);
    }

    VanadisFunctionalUnitType getInstFuncType() const override { return INST_INT_ARITH; }
    const char*               getInstCode() const override
    {
        if ( register_format == VanadisRegisterFormat::VANADIS_FORMAT_INT32 ) { return "SRLI32"; }
        else if ( register_format == VanadisRegisterFormat::VANADIS_FORMAT_INT64 ) {
            return "SRLI64";
        }
        else {

            return "SRLI";
        }
    }

    void printToBuffer(char* buffer, size_t buffer_size) override
    {
        snprintf(
            buffer, buffer_size,
            "%6s   %5" PRIu16 " <- %5" PRIu16 " >> imm=%" PRId64 " (phys: %5" PRIu16 " <- %5" PRIu16 " >> %" PRId64 ")",
            getInstCode(), isa_int_regs_out[0], isa_int_regs_in[0], imm_value, phys_int_regs_out[0],
            phys_int_regs_in[0], imm_value);
    }

    void log(SST::Output* output, int verboselevel, uint16_t sw_thr, 
                uint16_t phys_int_regs_out_0,uint16_t phys_int_regs_in_0) override
    {
        #ifdef VANADIS_BUILD_DEBUG
        if(output->getVerboseLevel() >= verboselevel) {

            std::ostringstream ss;
            ss << "hw_thr="<<getHWThread()<<" sw_thr=" <<sw_thr;
            ss << " Execute: 0x" << std::hex << getInstructionAddress() << std::dec << " " << getInstCode();
            ss << " phys: out=" <<  phys_int_regs_out_0 << " in=" << phys_int_regs_in_0;
            ss << " imm=" << imm_value;
            ss << ", isa: out=" <<  isa_int_regs_out[0]  << " in=" << isa_int_regs_in[0];
            output->verbose( CALL_INFO, verboselevel, 0, "%s\n", ss.str().c_str());
        }
        #endif
    }

    void instOp(VanadisRegisterFile* regFile, 
                                uint16_t phys_int_regs_out_0, uint16_t phys_int_regs_in_0) override
    {
        if constexpr ( sizeof( register_format ) == 4 ) {
            // imm cannot be 0 for RV32 or for RV64 when working on 32 bit values
            if ( UNLIKELY( 0 == imm_value ) ) {
                auto str = getenv("VANADIS_NO_FAULT");
                if ( nullptr == str ) {
                    flagError();
                }
            }
        }

        switch ( register_format ) {
        case VanadisRegisterFormat::VANADIS_FORMAT_INT64:
        {
            const uint64_t src_1 = regFile->getIntReg<uint64_t>(phys_int_regs_in_0);
            regFile->setIntReg<uint64_t>(phys_int_regs_out_0, src_1 >> imm_value);
        } break;
        case VanadisRegisterFormat::VANADIS_FORMAT_INT32:
        {
            const uint32_t src_1        = regFile->getIntReg<uint32_t>(phys_int_regs_in_0);
            const uint32_t imm_value_32 = static_cast<uint32_t>(imm_value);

            regFile->setIntReg<uint32_t>(phys_int_regs_out_0, src_1 >> imm_value_32);
        } break;
        default:
        {
            flagError();
        } break;
        }
    }

    void scalarExecute(SST::Output* output, VanadisRegisterFile* regFile) override
    {
        uint16_t phys_int_regs_in_0 = getPhysIntRegIn(0);
        uint16_t phys_int_regs_out_0 = getPhysIntRegOut(0);
        instOp(regFile, phys_int_regs_out_0, phys_int_regs_in_0);
        log(output, 16, 65535, phys_int_regs_out_0, phys_int_regs_in_0);
        markExecuted();
    }

protected:
    uint64_t imm_value;
};

template <VanadisRegisterFormat register_format>
class VanadisSIMTShiftRightLogicalImmInstruction : public VanadisSIMTInstruction, public VanadisShiftRightLogicalImmInstruction<register_format>
{
public:
    VanadisSIMTShiftRightLogicalImmInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts, const uint16_t dest,
        const uint16_t src_1, const uint64_t immediate) :
        VanadisInstruction(addr, hw_thr, isa_opts, 1, 1, 1, 1, 0, 0, 0, 0),
        VanadisSIMTInstruction(addr, hw_thr, isa_opts, 1, 1, 1, 1, 0, 0, 0, 0),
        VanadisShiftRightLogicalImmInstruction<register_format>(addr, hw_thr, isa_opts, dest, src_1, immediate)
    {
        ;
    }

    VanadisSIMTShiftRightLogicalImmInstruction* clone() override
    {
        return new VanadisSIMTShiftRightLogicalImmInstruction(*this);
    }

    void simtExecute(SST::Output* output, VanadisRegisterFile* regFile) override
    {
        uint16_t phys_int_regs_in_0 = getPhysIntRegIn(0,VanadisSIMTInstruction::sw_thread);
        uint16_t phys_int_regs_out_0 = getPhysIntRegOut(0,VanadisSIMTInstruction::sw_thread);
        instOp(regFile, phys_int_regs_out_0, phys_int_regs_in_0);
        log(output, 16, VanadisSIMTInstruction::sw_thread, phys_int_regs_out_0, phys_int_regs_in_0);
    }

};
} // namespace Vanadis
} // namespace SST

#endif
