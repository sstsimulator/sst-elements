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

#ifndef _H_VANADIS_SETREG_BY_CALL
#define _H_VANADIS_SETREG_BY_CALL

#include "inst/vinst.h"

#include <functional>

namespace SST {
namespace Vanadis {

template<typename reg_format>
class VanadisSetRegisterByCallInstruction : public virtual VanadisInstruction
{
    typedef std::function<reg_format()> SetRegisterCallable;

public:
    VanadisSetRegisterByCallInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts, const uint16_t dest,
        SetRegisterCallable call) :
        VanadisInstruction(addr, hw_thr, isa_opts, 0, 1, 0, 1, 0, 0, 0, 0), call_func(call)
    {
        isa_int_regs_out[0] = dest;
    }

    VanadisSetRegisterByCallInstruction* clone() override { return new VanadisSetRegisterByCallInstruction(*this); }
    VanadisFunctionalUnitType      getInstFuncType() const override { return INST_INT_ARITH; }
    const char*                    getInstCode() const override { return "SETREG"; }

    void printToBuffer(char* buffer, size_t buffer_size) override
    {
        snprintf(
            buffer, buffer_size, "SETREG  %5" PRIu16 " <- imm=function() (phys: %5" PRIu16 " <- function())",
            isa_int_regs_out[0], phys_int_regs_out[0]);
    }

    void log (SST::Output* output, int verboselevel, uint16_t sw_thr, 
                            uint16_t phys_int_regs_out_0)
    {
        #ifdef VANADIS_BUILD_DEBUG
        if(output->getVerboseLevel() >= verboselevel) {

            std::ostringstream ss;
            ss << "hw_thr="<<getHWThread()<<" sw_thr="<< sw_thr;
            ss << " Execute: 0x" << std::hex << getInstructionAddress() << std::dec << " " << getInstCode();
            ss << " phys: out= " << phys_int_regs_out_0  << " imm=" << reg_value << ", isa: out=" << isa_int_regs_out[0]; 
            ss << " Result-reg " << phys_int_regs_out_0  << ": " << reg_value;  
            output->verbose( CALL_INFO, verboselevel, 0, "%s\n", ss.str().c_str());
        }
        #endif
    }

    void instOp(VanadisRegisterFile* regFile, uint16_t phys_int_regs_out_0)
    {
        reg_format reg_value = call_func();
		regFile->setIntReg<reg_format>(phys_int_regs_out_0, reg_value);
    }

    virtual void scalarExecute(SST::Output* output, VanadisRegisterFile* regFile) override
    {
        uint16_t phys_int_regs_out_0 = getPhysIntRegOut(0);
        instOp(regFile,phys_int_regs_out_0);
        log(output, 16, 65535,phys_int_regs_out_0);
        markExecuted();
    }
private:
    SetRegisterCallable call_func;
    reg_format reg_value;
};
template<typename reg_format>
class VanadisSIMTSetRegisterByCallInstruction : public VanadisSIMTInstruction, public VanadisSetRegisterByCallInstruction<reg_format>
{
    typedef std::function<reg_format()> SetRegisterCallable;
public:
    VanadisSIMTSetRegisterByCallInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts, const uint16_t dest,
        SetRegisterCallable call) :
        VanadisInstruction(addr, hw_thr, isa_opts, 0, 1, 0, 1, 0, 0, 0, 0),
        VanadisSIMTInstruction(addr, hw_thr, isa_opts, 0, 1, 0, 1, 0, 0, 0, 0),
        VanadisSetRegisterByCallInstruction<reg_format>(addr, hw_thr, isa_opts, dest, call)
    {
        ;
    }

    VanadisSIMTSetRegisterByCallInstruction* clone() override { return new VanadisSIMTSetRegisterByCallInstruction(*this); }

    virtual void simtExecute(SST::Output* output, VanadisRegisterFile* regFile) override
    {
        uint16_t phys_int_regs_out_0 = getPhysIntRegOut(0, VanadisSIMTInstruction::sw_thread);
        
        VanadisSetRegisterByCallInstruction<reg_format>::instOp(regFile,phys_int_regs_out_0);
        VanadisSetRegisterByCallInstruction<reg_format>::log(output, 16, VanadisSIMTInstruction::sw_thread,phys_int_regs_out_0);
        
    }


};
} // namespace Vanadis
} // namespace SST

#endif
