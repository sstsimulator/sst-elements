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

#ifndef _H_VANADIS_SETREG
#define _H_VANADIS_SETREG

#include "inst/vinst.h"

#include <cstdint>
#include <sstream>

namespace SST {
namespace Vanadis {

template<typename reg_format>
class VanadisSetRegisterInstruction : public virtual VanadisInstruction
{
public:
    VanadisSetRegisterInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts, const uint16_t dest,
        const reg_format immediate) :
        VanadisInstruction(addr, hw_thr, isa_opts, 0, 1, 0, 1, 0, 0, 0, 0)
    {

        isa_int_regs_out[0] = dest;
        imm_value           = immediate;
    }

    VanadisSetRegisterInstruction* clone() override { return new VanadisSetRegisterInstruction(*this); }
    VanadisFunctionalUnitType      getInstFuncType() const override { return INST_INT_ARITH; }
    const char*                    getInstCode() const override { return "SETREG"; }

    void printToBuffer(char* buffer, size_t buffer_size) override
    {
        std::ostringstream ss;
        ss << getInstCode();

        ss << " "       << isa_int_regs_out[0]  << " <- imm=" << imm_value;
        ss << " phys: " << phys_int_regs_out[0] << " <- imm=" << imm_value;

        strncpy( buffer, ss.str().c_str(), buffer_size );
    }

    void log (SST::Output* output, int verboselevel, uint16_t sw_thr,
                            uint16_t phys_int_regs_out_0)
    {
        #ifdef VANADIS_BUILD_DEBUG
        if(output->getVerboseLevel() >= verboselevel) {

            std::ostringstream ss;
            ss << "hw_thr="<<getHWThread()<<" sw_thr="<< sw_thr;
            ss << " Execute: 0x" << std::hex << getInstructionAddress() << std::dec << " " << getInstCode();
            ss << " phys: out=" <<  phys_int_regs_out_0 << " imm=" << imm_value;
            ss << ", isa: out=" <<  isa_int_regs_out[0];
            ss << " Result-reg " << phys_int_regs_out_0  << ": " << imm_value;
            output->verbose( CALL_INFO, verboselevel, 0, "%s\n", ss.str().c_str());
        }
        #endif
    }
    void instOp(VanadisRegisterFile* regFile, uint16_t phys_int_regs_out_0)
    {
		regFile->setIntReg<reg_format>(phys_int_regs_out_0, imm_value);
    }

    virtual void scalarExecute(SST::Output* output, VanadisRegisterFile* regFile) override
    {
        uint16_t phys_int_regs_out_0 = getPhysIntRegOut(0);
        log(output, 16, 65535,phys_int_regs_out_0);
        instOp(regFile,phys_int_regs_out_0);
        markExecuted();
    }

private:
    reg_format imm_value;
};

} // namespace Vanadis
} // namespace SST

#endif
