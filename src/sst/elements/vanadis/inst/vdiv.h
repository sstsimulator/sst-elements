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

#ifndef _H_VANADIS_DIV
#define _H_VANADIS_DIV

#include "inst/vinst.h"

namespace SST {
namespace Vanadis {

template<typename gpr_format>
class VanadisDivideInstruction : public virtual VanadisInstruction
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
		  if(sizeof(gpr_format) == 8) {
				if(std::is_signed<gpr_format>::value) {
					return "DIV64";
				} else {
					return "DIVU64";
				}
		  } else {
				if(std::is_signed<gpr_format>::value) {
					return "DIV32";
				} else {
					return "DIVU32";
				}
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

    void instOp(VanadisRegisterFile* regFile, uint16_t phys_int_regs_out_0,uint16_t phys_int_regs_in_0,
                uint16_t phys_int_regs_in_1)
    {
        const gpr_format src_1 = regFile->getIntReg<gpr_format>(phys_int_regs_in_0);
        const gpr_format src_2 = regFile->getIntReg<gpr_format>(phys_int_regs_in_1);

        if ( ( UNLIKELY( 0 == src_2 ) ) ) {
            regFile->setIntReg<gpr_format>(phys_int_regs_out_0, -1);

            auto str = getenv("VANADIS_NO_FAULT");
            if ( nullptr == str ) {
                flagError();
            }

        } else {
            if constexpr (std::is_signed<gpr_format>::value) {
                if ( 8 == regFile->getIntRegWidth() ) {
                    if constexpr ( std::is_same_v<gpr_format,int32_t> ) {
                        if ( (1<< 31) == src_1 && -1 == src_2 ) {
                            regFile->setIntReg<gpr_format>(phys_int_regs_out_0, 1 << 31);
                        } else {
                            regFile->setIntReg<gpr_format>(phys_int_regs_out_0, src_1 / src_2);
                        }
                    } else if constexpr ( std::is_same_v<gpr_format,int64_t> ) {
                        if ( ((uint64_t)1<< 63) == src_1 && -1 == src_2 ) {
                            regFile->setIntReg<gpr_format>(phys_int_regs_out_0, (uint64_t) 1 << 63);
                        } else {
                            regFile->setIntReg<gpr_format>(phys_int_regs_out_0, src_1 / src_2);
                        }
                    } else {
                        assert(0);
                    }
                }
            } else {
                regFile->setIntReg<gpr_format>(phys_int_regs_out_0, src_1 / src_2);
            }
		}
    }

    void scalarExecute(SST::Output* output, VanadisRegisterFile* regFile) override
    {
        uint16_t phys_int_regs_out_0 = phys_int_regs_out[0];
        uint16_t phys_int_regs_in_0 = phys_int_regs_in[0];
        uint16_t phys_int_regs_in_1 = phys_int_regs_in[1];
        log(output, 16, 65535, phys_int_regs_out_0,phys_int_regs_in_0,
                phys_int_regs_in_1);
        instOp(regFile, phys_int_regs_out_0,phys_int_regs_in_0,
                phys_int_regs_in_1);
        markExecuted();
    }
};



} // namespace Vanadis
} // namespace SST

#endif
