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

#ifndef _H_VANADIS_FP_SIGN_LOGIC
#define _H_VANADIS_FP_SIGN_LOGIC

#include "inst/vfpinst.h"
#include "inst/vregfmt.h"
#include "util/vfpreghandler.h"

#include <cmath>

namespace SST {
namespace Vanadis {

enum class VanadisFPSignLogicOperation { SIGN_COPY, SIGN_XOR, SIGN_NEG };

template <typename fp_format, VanadisFPSignLogicOperation sign_op>
class VanadisFPSignLogicInstruction : public VanadisFloatingPointInstruction
{
public:
    VanadisFPSignLogicInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts, VanadisFloatingPointFlags* fpflags, const uint16_t dest,
        const uint16_t src_1, const uint16_t src_2) :
        VanadisInstruction(
            addr, hw_thr, isa_opts, 0, 0, 0, 0,
				((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) ? 4 : 2,
				((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) ? 2 : 1,
				((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) ? 4 : 2,
				((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) ? 2 : 1),
        VanadisFloatingPointInstruction(
            addr, hw_thr, isa_opts, fpflags, 0, 0, 0, 0,
				((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) ? 4 : 2,
				((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) ? 2 : 1,
				((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) ? 4 : 2,
				((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) ? 2 : 1) {

		  if((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) {
            isa_fp_regs_in[0]  = src_1;
            isa_fp_regs_in[1]  = src_1 + 1;
            isa_fp_regs_in[2]  = src_2;
            isa_fp_regs_in[3]  = src_2 + 1;
            isa_fp_regs_out[0] = dest;
            isa_fp_regs_out[1] = dest + 1;
        }
        else {
            isa_fp_regs_in[0]  = src_1;
            isa_fp_regs_in[1]  = src_2;
            isa_fp_regs_out[0] = dest;
        }
    }

    VanadisFPSignLogicInstruction* clone() override { return new VanadisFPSignLogicInstruction(*this); }
    VanadisFunctionalUnitType      getInstFuncType() const override { return INST_FP_ARITH; }

    const char* getInstCode() const override
    {
        if(8 == sizeof(fp_format)) {
            return "FPSIGN64";
        } else if(4 == sizeof(fp_format)) {
            return "FPSIGN32";
        } else {
            return "FPSIGN";
        }
    }

    const char* getSignOpStr( VanadisFPSignLogicOperation op ) {
        switch ( op ) {
            case VanadisFPSignLogicOperation::SIGN_COPY: return "COPY";
            case VanadisFPSignLogicOperation::SIGN_NEG: return "NEG";
            case VanadisFPSignLogicOperation::SIGN_XOR: return "XOR";
        }
        assert(0);
    }

    void printToBuffer(char* buffer, size_t buffer_size) override
    {
        snprintf(
            buffer, buffer_size,
            "%s %5" PRIu16 " <- %5" PRIu16 " * %5" PRIu16 " (phys: %5" PRIu16 " <- %5" PRIu16 " * %5" PRIu16 ")",
            getInstCode(), isa_fp_regs_out[0], isa_fp_regs_in[0], isa_fp_regs_in[1], phys_fp_regs_out[0], phys_fp_regs_in[0],
            phys_fp_regs_in[1]);
    }

    template <typename T>
    T signInject( T src1, T src2, VanadisFPSignLogicOperation op ) {

        if constexpr ( std::is_same_v<T,uint64_t> || std::is_same_v<T,uint32_t>) {
            switch( op ) {
                case VanadisFPSignLogicOperation::SIGN_COPY:
                    if ( isSignBitSet(src2) ) {
                        src1 = setSignBit(src1);
                    } else {
                        src1 = clearSignBit(src1);
                    }
                    break;
                case VanadisFPSignLogicOperation::SIGN_NEG:
                    if ( isSignBitSet(src2) ) {
                        src1 = clearSignBit(src1);
                    } else {
                        src1 = setSignBit(src1);
                    }
                    break;
                case VanadisFPSignLogicOperation::SIGN_XOR:
                    if ( isSignBitSet(src1) ^ isSignBitSet(src2) ) {
                        src1 = setSignBit(src1);
                    } else {
                        src1 = clearSignBit(src1);
                    }
                    break;
            }
        } else {
            assert(0);
        }
        return src1;
    }

    void log(SST::Output* output,int verboselevel, uint16_t sw_thr,  uint16_t phys_fp_regs_in_0, 
                uint16_t phys_fp_regs_in_1, uint16_t phys_fp_regs_in_2, uint16_t phys_fp_regs_in_3, 
                uint16_t phys_fp_regs_out_0,uint16_t phys_fp_regs_out_1)

    {
        #ifdef VANADIS_BUILD_DEBUG
        if(output->getVerboseLevel() >= verboselevel ) 
        {
            if((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_options->getFPRegisterMode())) 
            {
                output->verbose(CALL_INFO, verboselevel, 0, "hw_thr=%d sw_thr = %d Execute: (addr=0x%" PRI_ADDR ") %s / phys-in: { %" PRIu16 ", %" PRIu16 " } / { %" PRIu16 ", %" PRIu16 " } -> phys-out: { %" PRIu16 ", %" PRIu16 " }\n",
                    getHWThread(),sw_thr, getInstructionAddress(), getInstCode(), phys_fp_regs_in_0, phys_fp_regs_in_1,
                    phys_fp_regs_in_2, phys_fp_regs_in_3, phys_fp_regs_out_0, phys_fp_regs_out_1);
            } 
            else 
            {
                output->verbose(CALL_INFO, verboselevel, 0, "hw_thr=%d sw_thr = %d Execute: (addr=0x%" PRI_ADDR ") %s / phys-in: %" PRIu16 " / %" PRIu16 " -> phys-out: %" PRIu16 "\n",
                    getHWThread(),sw_thr, getInstructionAddress(), getInstCode(), phys_fp_regs_in_0, phys_fp_regs_in_1, phys_fp_regs_out_0);
            }
		}
        #endif
    }

    void instOp(VanadisRegisterFile* regFile, uint16_t phys_fp_regs_in_0, uint16_t phys_fp_regs_in_1, 
                        uint16_t phys_fp_regs_in_2, uint16_t phys_fp_regs_in_3, 
                        uint16_t phys_fp_regs_out_0,uint16_t phys_fp_regs_out_1)
    {
        uint64_t src_1; 
        uint64_t src_2; 
        if ((sizeof(fp_format)==8) && (VANADIS_REGISTER_MODE_FP32 == isa_options->getFPRegisterMode()))  
        {            
			src_1 = combineFromRegisters<uint64_t>(regFile, phys_fp_regs_in_0, phys_fp_regs_in_1);
			src_2 = combineFromRegisters<uint64_t>(regFile, phys_fp_regs_in_2, phys_fp_regs_in_3);
        } else {
            if ( 8 == regFile->getFPRegWidth() )  {
                src_1 = regFile->getFPReg<uint64_t>(phys_fp_regs_in_0);
                src_2 = regFile->getFPReg<uint64_t>(phys_fp_regs_in_1);
            } else {
                src_1 = regFile->getFPReg<uint32_t>(phys_fp_regs_in_0);
                src_2 = regFile->getFPReg<uint32_t>(phys_fp_regs_in_1);
            }
        }

        uint64_t result;

        if constexpr (std::is_same_v<fp_format,double> ) {

            result = signInject( src_1, src_2, sign_op );

        } else if constexpr (std::is_same_v<fp_format,float> ) {

            if ( 8 == regFile->getFPRegWidth() ) {
                result = 0xffffffff00000000;
                if ( isNaN_boxed(src_1) ) {
                    if ( 0 != src_2 && isNaN_boxed(src_2) ) {
                        result |= signInject( static_cast<uint32_t>(src_1), static_cast<uint32_t>(src_2), sign_op );
                    } else {
                        result |= signInject( static_cast<uint32_t>(src_1), NaN<uint32_t>(), sign_op );
                    }
                } else {
                    if ( 0 != src_2 && isNaN_boxed(src_2) ) {
                        result |= signInject( NaN<uint32_t>(), static_cast<uint32_t>(src_2), sign_op );
                    } else {
                        result |= signInject( NaN<uint32_t>(), NaN<uint32_t>(), sign_op );
                    }
                }
            } else {
                assert(0);
            }
        } else {
            assert(0);
        }

        // Sign-injection instructions do not set floating-point exception flags

        if ((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_options->getFPRegisterMode())) {
            
            fractureToRegisters<uint64_t>(regFile, phys_fp_regs_out_0, phys_fp_regs_out_1, result);
        } else {
            if ( 8 == regFile->getFPRegWidth() ) {
				regFile->setFPReg<uint64_t>(phys_fp_regs_out_0, result);
            } else {
				regFile->setFPReg<uint32_t>(phys_fp_regs_out_0, static_cast<uint32_t>(result));
            }
        
        }
    }

    void scalarExecute(SST::Output* output, VanadisRegisterFile* regFile) override
    {
        uint16_t phys_fp_regs_out_0 = getPhysFPRegOut(0);
        uint16_t phys_fp_regs_out_1 = 0;

        uint16_t phys_fp_regs_in_0 = getPhysFPRegIn(0);
        uint16_t phys_fp_regs_in_1 = getPhysFPRegIn(1);
        uint16_t phys_fp_regs_in_2 =0;
        uint16_t phys_fp_regs_in_3 =0;

        if ((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_options->getFPRegisterMode()))  
        {
            phys_fp_regs_in_2 =getPhysFPRegIn(2);
            phys_fp_regs_in_3 =getPhysFPRegIn(3);
            phys_fp_regs_out_1 = getPhysFPRegOut(1);
        }
        log(output, 16, 65535, phys_fp_regs_in_0,phys_fp_regs_in_1,phys_fp_regs_in_2,phys_fp_regs_in_3,phys_fp_regs_out_0,phys_fp_regs_out_1);
        instOp(regFile,phys_fp_regs_in_0, phys_fp_regs_in_1,phys_fp_regs_in_2,phys_fp_regs_in_3,phys_fp_regs_out_0,phys_fp_regs_out_1);
        markExecuted();
    }
};


template <typename fp_format, VanadisFPSignLogicOperation sign_op>
class VanadisSIMTFPSignLogicInstruction : public VanadisSIMTInstruction, public VanadisFPSignLogicInstruction<fp_format,sign_op>
{
public:
    VanadisSIMTFPSignLogicInstruction(
        const uint64_t addr, const uint32_t hw_thr, const VanadisDecoderOptions* isa_opts, VanadisFloatingPointFlags* fpflags, const uint16_t dest,
        const uint16_t src_1, const uint16_t src_2) :
        VanadisInstruction(
            addr, hw_thr, isa_opts, 0, 0, 0, 0,
				((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) ? 4 : 2,
				((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) ? 2 : 1,
				((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) ? 4 : 2,
				((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) ? 2 : 1),
        VanadisSIMTInstruction(
            addr, hw_thr, isa_opts, 0, 0, 0, 0,
				((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) ? 4 : 2,
				((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) ? 2 : 1,
				((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) ? 4 : 2,
				((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_opts->getFPRegisterMode())) ? 2 : 1),
        VanadisFPSignLogicInstruction<fp_format,sign_op>(addr, hw_thr, isa_opts, fpflags, dest, src_1, src_2)        
    {
             ;    
    }

    VanadisSIMTFPSignLogicInstruction* clone() override { return new VanadisSIMTFPSignLogicInstruction(*this); }

    void simtExecute(SST::Output* output, VanadisRegisterFile* regFile) override
    {
        uint16_t phys_fp_regs_out_0 = getPhysFPRegOut(0, VanadisSIMTInstruction::sw_thread);
        uint16_t phys_fp_regs_out_1 = 0;

        uint16_t phys_fp_regs_in_0 = getPhysFPRegIn(0, VanadisSIMTInstruction::sw_thread);
        uint16_t phys_fp_regs_in_1 = getPhysFPRegIn(1, VanadisSIMTInstruction::sw_thread);
        uint16_t phys_fp_regs_in_2 =0;
        uint16_t phys_fp_regs_in_3 =0;

        if ((sizeof(fp_format) == 8) && (VANADIS_REGISTER_MODE_FP32 == isa_options->getFPRegisterMode()))  
        {
            phys_fp_regs_in_2 =getPhysFPRegIn(2, VanadisSIMTInstruction::sw_thread);
            phys_fp_regs_in_3 =getPhysFPRegIn(3, VanadisSIMTInstruction::sw_thread);
            phys_fp_regs_out_1 = getPhysFPRegOut(1, VanadisSIMTInstruction::sw_thread);
        }
        VanadisFPSignLogicInstruction<fp_format,sign_op>::log(output, 16, VanadisSIMTInstruction::sw_thread, phys_fp_regs_in_0,phys_fp_regs_in_1,phys_fp_regs_in_2,phys_fp_regs_in_3,phys_fp_regs_out_0,phys_fp_regs_out_1);
        VanadisFPSignLogicInstruction<fp_format,sign_op>::instOp(regFile,phys_fp_regs_in_0, phys_fp_regs_in_1,phys_fp_regs_in_2,phys_fp_regs_in_3,phys_fp_regs_out_0,phys_fp_regs_out_1);
    }
};

} // namespace Vanadis
} // namespace SST

#endif
