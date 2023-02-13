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

#ifndef _H_VANADIS_SYSCALL
#define _H_VANADIS_SYSCALL

#include "inst/vspeculate.h"

namespace SST {
namespace Vanadis {

class VanadisSysCallInstruction : public VanadisInstruction
{
public:
    VanadisSysCallInstruction(const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts) :
        VanadisInstruction(
            addr, hw_thr, isa_opts, isa_opts->countISAIntRegisters(), isa_opts->countISAIntRegisters(),
            isa_opts->countISAIntRegisters(), isa_opts->countISAIntRegisters(), isa_opts->countISAFPRegisters(),
            isa_opts->countISAFPRegisters(), isa_opts->countISAFPRegisters(), isa_opts->countISAFPRegisters())
    {

        for ( uint16_t i = 0; i < isa_opts->countISAIntRegisters(); ++i ) {
            isa_int_regs_in[i]  = i;
            isa_int_regs_out[i] = i;
        }

        for ( uint16_t i = 0; i < isa_opts->countISAFPRegisters(); ++i ) {
            isa_fp_regs_in[i]  = i;
            isa_fp_regs_out[i] = i;
        }
    }

    VanadisSysCallInstruction* clone() { return new VanadisSysCallInstruction(*this); }

    virtual VanadisFunctionalUnitType getInstFuncType() const { return INST_SYSCALL; }

    virtual const char* getInstCode() const { return "SYSCALL"; }

    virtual void printToBuffer(char* buffer, size_t buffer_size)
    {
        snprintf(
            buffer, buffer_size, "SYSCALL (syscall-code-isa: %" PRIu16 ", phys: %" PRIu16 ")\n",
            isa_options->getISASysCallCodeReg(), phys_int_regs_in[isa_options->getISASysCallCodeReg()]);
    }

    virtual void execute(SST::Output* output, VanadisRegisterFile* regFile)
    {
        const uint64_t code_reg_ptr = regFile->getIntReg<uint64_t>(isa_options->getISASysCallCodeReg());
#ifdef VANADIS_BUILD_DEBUG
        output->verbose(
            CALL_INFO, 16, 0, "Execute: (addr=0x%0llx) SYSCALL (isa: %" PRIu16 ", os-code: %" PRIu64 ")\n",
            getInstructionAddress(), isa_options->getISASysCallCodeReg(), code_reg_ptr);
#endif
        markExecuted();
    }

    virtual bool performIntRegisterRecovery() const { return false; }
    virtual bool performFPRegisterRecovery() const { return false; }
};

} // namespace Vanadis
} // namespace SST

#endif
