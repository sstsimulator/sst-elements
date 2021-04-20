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

#ifndef _H_VANADIS_MUL_SPLIT
#define _H_VANADIS_MUL_SPLIT

#include "inst/vinst.h"

namespace SST {
namespace Vanadis {

class VanadisMultiplySplitInstruction : public VanadisInstruction {
public:
	VanadisMultiplySplitInstruction(
		const uint64_t addr,
		const uint32_t hw_thr,
		const VanadisDecoderOptions* isa_opts,
		const uint16_t dest_lo,
		const uint16_t dest_hi,
		const uint16_t src_1,
		const uint16_t src_2,
		bool performSgnd,
		VanadisRegisterFormat fmt ) :
		VanadisInstruction(addr, hw_thr, isa_opts, 2, 2, 2, 2, 0, 0, 0, 0),
			performSigned(performSgnd), reg_format(fmt) {

		isa_int_regs_in[0]  = src_1;
		isa_int_regs_in[1]  = src_2;
		isa_int_regs_out[0] = dest_lo;
		isa_int_regs_out[1] = dest_hi;
	}

	VanadisMultiplySplitInstruction* clone() {
		return new VanadisMultiplySplitInstruction( *this );
	}

	virtual VanadisFunctionalUnitType getInstFuncType() const {
		return INST_INT_ARITH;
	}

	virtual const char* getInstCode() const {
		return "MULSPLIT";
	}

	virtual void printToBuffer(char* buffer, size_t buffer_size) {
                snprintf(buffer, buffer_size, "MULSPLIT lo: %5" PRIu16 " hi: %" PRIu16 " <- %5" PRIu16 " * %5" PRIu16 " (phys: lo: %5" PRIu16 " hi: %" PRIu16 " <- %5" PRIu16 " * %5" PRIu16 ")",
			isa_int_regs_out[0], isa_int_regs_out[1],
			isa_int_regs_in[0], isa_int_regs_in[1],
			phys_int_regs_out[0], phys_int_regs_out[1],
			phys_int_regs_in[0], phys_int_regs_in[1] );
        }

	virtual void execute( SST::Output* output, VanadisRegisterFile* regFile ) {
#ifdef VANADIS_BUILD_DEBUG
		output->verbose(CALL_INFO, 16, 0, "Execute: (addr=%p) MULSPLIT phys: out-lo: %" PRIu16 " out-hi: %" PRIu16 " in=%" PRIu16 ", %" PRIu16 ", isa: out-lo: %" PRIu16 " out-hi: %" PRIu16 " / in=%" PRIu16 ", %" PRIu16 "\n",
			(void*) getInstructionAddress(), phys_int_regs_out[0], phys_int_regs_out[1],
			phys_int_regs_in[0], phys_int_regs_in[1],
			isa_int_regs_out[0], isa_int_regs_out[1], isa_int_regs_in[0], isa_int_regs_in[1] );
#endif
		if( performSigned ) {
			switch( reg_format ) {
			case VANADIS_FORMAT_INT64:
				{
					const int64_t src_1 = regFile->getIntReg<int64_t>( phys_int_regs_in[0] );
					const int64_t src_2 = regFile->getIntReg<int64_t>( phys_int_regs_in[1] );

					const int64_t multiply_result = (src_1) * (src_2);
					const int64_t result_lo       = (int32_t)( multiply_result & 0xFFFFFFFF );
					const int64_t result_hi       = (int32_t)( multiply_result >> 32 );

					output->verbose(CALL_INFO, 16, 0, "-> Execute: (detail, signed, MULSPLIT64) %" PRId64 " * %" PRId64 " = %" PRId64 " = (lo: %" PRId64 ", hi: %" PRId64 " )\n",
						src_1, src_2, multiply_result, result_lo, result_hi );

					regFile->setIntReg<int64_t>( phys_int_regs_out[0], result_lo );
					regFile->setIntReg<int64_t>( phys_int_regs_out[1], result_hi );
				}
				break;
			case VANADIS_FORMAT_INT32:
				{
					const int32_t src_1 = regFile->getIntReg<int32_t>( phys_int_regs_in[0] );
					const int32_t src_2 = regFile->getIntReg<int32_t>( phys_int_regs_in[1] );

					const int64_t multiply_result = static_cast<int64_t>(src_1) * static_cast<int64_t>(src_2);
					const int32_t result_lo       = (int32_t)( multiply_result & 0xFFFFFFFF );
					const int32_t result_hi       = (int32_t)( multiply_result >> 32UL );

					output->verbose(CALL_INFO, 16, 0, "-> Execute: (detail, signed, MULSPLIT32) %" PRId32 " * %" PRId32 " = %" PRId64 " = (lo: %" PRId32 ", hi: %" PRId32 " )\n",
						src_1, src_2, multiply_result, result_lo, result_hi );

					regFile->setIntReg<int32_t>( phys_int_regs_out[0], result_lo );
					regFile->setIntReg<int32_t>( phys_int_regs_out[1], result_hi );
				}
				break;
			default:
				{
					flagError();
				}
				break;
			}
		} else {
			switch( reg_format ) {
			case VANADIS_FORMAT_INT64:
				{
					const uint64_t src_1 = regFile->getIntReg<uint64_t>( phys_int_regs_in[0] );
					const uint64_t src_2 = regFile->getIntReg<uint64_t>( phys_int_regs_in[1] );

					const uint64_t multiply_result = (src_1) * (src_2);
					const uint64_t result_lo       = (uint32_t)( multiply_result & 0xFFFFFFFF );
					const uint64_t result_hi       = (uint32_t)( multiply_result >> 32UL );

					output->verbose(CALL_INFO, 16, 0, "-> Execute: (detail, unsigned, MULSPLIT64) %" PRIu64 " * %" PRIu64 " = %" PRIu64 " = (lo: %" PRIu64 ", hi: %" PRIu64 " )\n",
							src_1, src_2, multiply_result, result_lo, result_hi );

					regFile->setIntReg<uint64_t>( phys_int_regs_out[0], result_lo );
					regFile->setIntReg<uint64_t>( phys_int_regs_out[1], result_hi );
				}
				break;
			case VANADIS_FORMAT_INT32:
				{
					const uint32_t src_1 = regFile->getIntReg<uint32_t>( phys_int_regs_in[0] );
					const uint32_t src_2 = regFile->getIntReg<uint32_t>( phys_int_regs_in[1] );

					const uint64_t multiply_result = static_cast<uint64_t>(src_1) * static_cast<uint64_t>(src_2);
					const uint32_t result_lo       = (uint32_t)( multiply_result & 0xFFFFFFFF );
					const uint32_t result_hi       = (uint32_t)( multiply_result >> 32UL );

					output->verbose(CALL_INFO, 16, 0, "-> Execute: (detail, unsigned, MULSPLIT32) %" PRIu32 " * %" PRIu32 " = %" PRIu64 " = (lo: %" PRIu32 ", hi: %" PRIu32 " )\n",
							src_1, src_2, multiply_result, result_lo, result_hi );

					regFile->setIntReg<uint32_t>( phys_int_regs_out[0], result_lo );
					regFile->setIntReg<uint32_t>( phys_int_regs_out[1], result_hi );
				}
				break;
			default:
				{
					flagError();
				}
				break;
			}
		}
		markExecuted();
	}

protected:
	VanadisRegisterFormat reg_format;
	bool performSigned;

};

}
}

#endif
