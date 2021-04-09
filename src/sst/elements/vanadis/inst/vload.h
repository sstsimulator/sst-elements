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

#ifndef _H_VANADIS_LOAD
#define _H_VANADIS_LOAD

#include "inst/vmemflagtype.h"

namespace SST {
namespace Vanadis {

enum VanadisLoadRegisterType {
	LOAD_INT_REGISTER,
	LOAD_FP_REGISTER
};

class VanadisLoadInstruction : public VanadisInstruction {

public:
	VanadisLoadInstruction(
		const uint64_t addr,
		const uint32_t hw_thr,
		const VanadisDecoderOptions* isa_opts,
		const uint16_t memAddrReg,
		const int64_t offst,
		const uint16_t tgtReg,
		const uint16_t load_bytes,
		const bool extend_sign,
		const VanadisMemoryTransaction accessT,
		VanadisLoadRegisterType regT
		) : VanadisInstruction(addr, hw_thr, isa_opts,
			1,
			regT == LOAD_INT_REGISTER ? 1 : 0,
			1,
			regT == LOAD_INT_REGISTER ? 1 : 0,
			0,
			regT == LOAD_FP_REGISTER ? 1 : 0,
			0,
			regT == LOAD_FP_REGISTER ? 1 : 0),
			offset(offst), load_width(load_bytes), signed_extend(extend_sign),
			memAccessType(accessT), regType(regT) {

		isa_int_regs_in[0]  = memAddrReg;

		switch( regT ) {
		case LOAD_INT_REGISTER:
			{
				isa_int_regs_out[0] = tgtReg;
			}
			break;
		case LOAD_FP_REGISTER:
			{
				isa_fp_regs_out[0] = tgtReg;
			}
			break;
		}
	}

	VanadisLoadInstruction* clone() {
		return new VanadisLoadInstruction( *this );
	}

	bool performSignExtension() const { return signed_extend; }

	virtual bool isPartialLoad() const { return false; }
	virtual VanadisFunctionalUnitType getInstFuncType() const { return INST_LOAD; }
	virtual const char* getInstCode() const {
		switch(memAccessType) {
		case MEM_TRANSACTION_LLSC_LOAD:
			return "LLSC_LOAD";
		case MEM_TRANSACTION_LOCK:
			return "LOCK_LOAD";
		case MEM_TRANSACTION_NONE:
		default:
			switch( regType ) {
			case LOAD_INT_REGISTER:
				return "LOAD";
			case LOAD_FP_REGISTER:
				return "LOADFP";
			}
		}

		return "LOADUNK";
	}

	uint16_t getMemoryAddressRegister() const { return phys_int_regs_in[1]; }
	uint16_t getTargetRegister() const { return phys_int_regs_in[0]; }

	virtual void execute( SST::Output* output, VanadisRegisterFile* regFile ) {
		markExecuted();
	}

	virtual VanadisMemoryTransaction getTransactionType() const {
		return memAccessType;
	}

	virtual void printToBuffer( char* buffer, size_t buffer_size ) {
		switch( regType ) {
		case LOAD_INT_REGISTER:
			{
				snprintf( buffer, buffer_size, "LOAD (%s)  %5" PRIu16 " <- memory[ %5" PRIu16 " + %20" PRId64 " (phys: %5" PRIu16 " <- memory[%5" PRIu16 " + %20" PRId64 "])\n",
					getTransactionTypeString( memAccessType ),
					isa_int_regs_out[0], isa_int_regs_in[0], offset,
					phys_int_regs_out[0], phys_int_regs_in[0], offset);
			}
			break;
		case LOAD_FP_REGISTER:
			{
				snprintf( buffer, buffer_size, "LOADFP (%s)  %5" PRIu16 " <- memory[ %5" PRIu16 " + %20" PRId64 " (phys: %5" PRIu16 " <- memory[%5" PRIu16 " + %20" PRId64 "])\n",
					getTransactionTypeString( memAccessType ),
					isa_fp_regs_out[0], isa_int_regs_in[0], offset,
					phys_fp_regs_out[0], phys_int_regs_in[0], offset);
			}
			break;
		}
	}

	virtual void computeLoadAddress( VanadisRegisterFile* reg, uint64_t* out_addr, uint16_t* width ) {
		int64_t tmp = reg->getIntReg<int64_t>( phys_int_regs_in[0] );

		(*out_addr) = (uint64_t) (tmp + offset);
		(*width)    = load_width;
	}

	virtual void computeLoadAddress( SST::Output* output, VanadisRegisterFile* regFile, uint64_t* out_addr, uint16_t* width ) {
		uint64_t mem_addr_reg_val = regFile->getIntReg<uint64_t>( phys_int_regs_in[0] );

		switch( regType ) {
		case LOAD_INT_REGISTER:
			{
				output->verbose(CALL_INFO, 16, 0, "Execute: (0x%llx) LOAD addr-reg: %" PRIu16 " phys: %" PRIu16 " / offset: %" PRId64 " / target: %" PRIu16 " phys: %" PRIu16 "\n",
					getInstructionAddress(), isa_int_regs_in[0], phys_int_regs_in[0], offset, isa_int_regs_out[0], phys_int_regs_out[0] );
			}
			break;
		case LOAD_FP_REGISTER:
			{
				output->verbose(CALL_INFO, 16, 0, "Execute: (0x%llx) LOAD addr-reg: %" PRIu16 " phys: %" PRIu16 " / offset: %" PRId64 " / target: %" PRIu16 " phys: %" PRIu16 "\n",
					getInstructionAddress(), isa_int_regs_in[0], phys_int_regs_in[0], offset, isa_fp_regs_out[0], phys_fp_regs_out[0] );
			}
			break;
		}

#ifdef VANADIS_BUILD_DEBUG
		output->verbose(CALL_INFO, 16, 0, "[execute-load]: transaction-type:  %s / ins: 0x%llx\n", getTransactionTypeString( memAccessType ),
			getInstructionAddress() );
                output->verbose(CALL_INFO, 16, 0, "[execute-load]: reg[%5" PRIu16 "]:       %" PRIu64 "\n", phys_int_regs_in[0], mem_addr_reg_val);
                output->verbose(CALL_INFO, 16, 0, "[execute-load]: offset           : %" PRId64 "\n", offset);
                output->verbose(CALL_INFO, 16, 0, "[execute-load]: (add)            : %" PRIu64 "\n", (mem_addr_reg_val + offset));
#endif
		int64_t tmp_val = regFile->getIntReg<int64_t>( phys_int_regs_in[0] );

		(*out_addr) = (uint64_t) (tmp_val + offset);
		(*width)    = load_width;
	}

	virtual uint16_t getLoadWidth() const { return load_width; }

	VanadisLoadRegisterType getValueRegisterType() const {
		return regType;
	}

	virtual uint16_t getRegisterOffset() const {
		return 0;
	}

protected:
	const bool signed_extend;
	VanadisMemoryTransaction memAccessType;
	const int64_t offset;
	const uint16_t load_width;
	VanadisLoadRegisterType regType;

};

}
}

#endif
