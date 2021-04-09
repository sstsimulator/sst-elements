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

#ifndef _H_VANADIS_STORE
#define _H_VANADIS_STORE

#include "inst/vmemflagtype.h"
#include "inst/vinst.h"

namespace SST {
namespace Vanadis {

enum VanadisStoreRegisterType {
	STORE_INT_REGISTER,
	STORE_FP_REGISTER
};

class VanadisStoreInstruction : public VanadisInstruction {

public:
	VanadisStoreInstruction(
		const uint64_t addr,
		const uint32_t hw_thr,
		const VanadisDecoderOptions* isa_opts,
		const uint16_t memoryAddr,
		const int64_t offst,
		const uint16_t valueReg,
		const uint16_t store_bytes,
		VanadisMemoryTransaction accessT,
		VanadisStoreRegisterType regT) :
		VanadisInstruction(addr, hw_thr, isa_opts,
			regT == STORE_INT_REGISTER ? 2 : 1,
			accessT == MEM_TRANSACTION_LLSC_STORE ? 1 : 0,
			regT == STORE_INT_REGISTER ? 2 : 1,
			accessT == MEM_TRANSACTION_LLSC_STORE ? 1 : 0,
			regT == STORE_FP_REGISTER ? 1 : 0, 0,
			regT == STORE_FP_REGISTER ? 1 : 0, 0),
		store_width(store_bytes), offset(offst),
		memAccessType(accessT), regType(regT) {

		switch( regT ) {
		case STORE_INT_REGISTER:
			{
				isa_int_regs_in[0] = memoryAddr;
				isa_int_regs_in[1] = valueReg;

				if( MEM_TRANSACTION_LLSC_STORE == accessT ) {
					isa_int_regs_out[0] = valueReg;
				}

			}
			break;
		case STORE_FP_REGISTER:
			{
				isa_int_regs_in[0] = memoryAddr;
				isa_fp_regs_in[0] = valueReg;

				if( MEM_TRANSACTION_LLSC_STORE == accessT ) {
					isa_fp_regs_out[0] = valueReg;
				}
			}
			break;
		}

	}

	VanadisStoreInstruction* clone() {
		return new VanadisStoreInstruction( *this );
	}

	virtual bool isPartialStore() {
		return false;
	}

	virtual VanadisMemoryTransaction getTransactionType() const {
		return memAccessType;
	}

	virtual VanadisFunctionalUnitType getInstFuncType() const {
		return INST_STORE;
	}

	virtual const char* getInstCode() const {
		switch( memAccessType ) {
		case MEM_TRANSACTION_LLSC_STORE:
			return "LLSC_STORE";
		case MEM_TRANSACTION_LOCK:
			return "LOCK_STORE";
		case MEM_TRANSACTION_NONE:
		default:
			{
				switch( regType ) {
				case STORE_INT_REGISTER:
					return "STORE";
					break;
				case STORE_FP_REGISTER:
					return "STOREFP";
					break;
				}
			}
		}

		return "STOREUNK";
	}

	virtual void printToBuffer(char* buffer, size_t buffer_size) {
		switch( regType ) {
		case STORE_INT_REGISTER:
			{
				snprintf(buffer, buffer_size, "STORE (%s)   %5" PRIu16 " -> memory[%5" PRIu16 " + %" PRId64 "] (phys: %5" PRIu16 " -> memory[%5" PRIu16 " + %" PRId64 "])",
					getTransactionTypeString(memAccessType), isa_int_regs_in[1], isa_int_regs_in[0], offset,
					phys_int_regs_in[1], phys_int_regs_in[0], offset);
			}
			break;
		case STORE_FP_REGISTER:
			{
				snprintf(buffer, buffer_size, "STOREFP (%s)   %5" PRIu16 " -> memory[%5" PRIu16 " + %" PRId64 "] (phys: %5" PRIu16 " -> memory[%5" PRIu16 " + %" PRId64 "])",
					getTransactionTypeString(memAccessType), isa_fp_regs_in[0], isa_int_regs_in[0], offset,
					phys_fp_regs_in[0], phys_int_regs_in[0], offset);
			}
			break;
		}
        }

	virtual void execute( SST::Output* output, VanadisRegisterFile* regFile ) {
		if( memAccessType != MEM_TRANSACTION_LLSC_STORE ) {
			markExecuted();
		}
	}

	virtual void computeStoreAddress( SST::Output* output, VanadisRegisterFile* reg, uint64_t* store_addr, uint16_t* op_width ) {
		const int64_t reg_tmp = reg->getIntReg<int64_t>( phys_int_regs_in[0] );

                (*store_addr)  = (uint64_t)( reg_tmp + offset );
		(*op_width)    = store_width;

		switch( regType ) {
		case STORE_INT_REGISTER:
			{
				output->verbose(CALL_INFO, 16, 0, "Execute: (addr=0x%llx) STORE addr-reg: %" PRIu16 " / val-reg: %" PRIu16 " / offset: %" PRId64 " / width: %" PRIu16 " / store-addr: %" PRIu64 " (0x%llx)\n",
					getInstructionAddress(), phys_int_regs_in[0], phys_int_regs_in[1], offset, store_width, (*store_addr), (*store_addr) );
			}
			break;
		case STORE_FP_REGISTER:
			{
				output->verbose(CALL_INFO, 16, 0, "Execute: (addr=0x%llx) STOREFP addr-reg: %" PRIu16 " / val-reg: %" PRIu16 " / offset: %" PRId64 " / width: %" PRIu16 " / store-addr: %" PRIu64 " (0x%llx)\n",
					getInstructionAddress(), phys_int_regs_in[0], phys_fp_regs_in[0], offset, store_width, (*store_addr), (*store_addr) );
			}
			break;
		}
        }

	uint16_t getStoreWidth() const {
		return store_width;
	}

	virtual uint16_t getRegisterOffset() const {
		return 0;
	}

	uint16_t getMemoryAddressRegister() const { return phys_int_regs_in[0]; }
	uint16_t getValueRegister() const {
			switch( regType ) {
			case STORE_INT_REGISTER:
				return phys_int_regs_in[1];
			case STORE_FP_REGISTER:
				return phys_fp_regs_in[0];
			}
		}

	VanadisStoreRegisterType getValueRegisterType() const { return regType; }

protected:
	const int64_t offset;
	const uint16_t store_width;
	VanadisMemoryTransaction memAccessType;
	VanadisStoreRegisterType regType;

};

}
}

#endif
