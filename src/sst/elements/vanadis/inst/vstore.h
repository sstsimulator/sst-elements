
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
		const uint64_t id,
		const uint64_t addr,
		const uint32_t hw_thr,
		const VanadisDecoderOptions* isa_opts,
		const uint16_t memoryAddr,
		const int64_t offst,
		const uint16_t valueReg,
		const uint16_t store_bytes,
		VanadisMemoryTransaction accessT,
		VanadisStoreRegisterType regT) :
		VanadisInstruction(id, addr, hw_thr, isa_opts,
			2, accessT == MEM_TRANSACTION_LLSC_STORE ? 1 : 0,
			2, accessT == MEM_TRANSACTION_LLSC_STORE ? 1 : 0,
			0, 0, 0, 0),
		store_width(store_bytes), offset(offst),
		memAccessType(accessT), regType(regT) {

		isa_int_regs_in[0] = memoryAddr;
		isa_int_regs_in[1] = valueReg;

		if( MEM_TRANSACTION_LLSC_STORE == accessT ) {
			isa_int_regs_out[0] = valueReg;
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
			return "STORE";
		}
	}

	virtual void printToBuffer(char* buffer, size_t buffer_size) {
		snprintf(buffer, buffer_size, "STORE (%s)   %5" PRIu16 " -> memory[%5" PRIu16 " + %" PRId64 "] (phys: %5" PRIu16 " -> memory[%5" PRIu16 " + %" PRId64 "])",
			getTransactionTypeString(memAccessType), isa_int_regs_in[1], isa_int_regs_in[0], offset,
			phys_int_regs_in[1], phys_int_regs_in[0], offset);
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

		output->verbose(CALL_INFO, 16, 0, "Execute: (addr=0x%llx) STORE addr-reg: %" PRIu16 " / val-reg: %" PRIu16 " / offset: %" PRId64 " / width: %" PRIu16 " / store-addr: %" PRIu64 " (0x%llx)\n",
			getInstructionAddress(), phys_int_regs_in[0], phys_int_regs_in[1], offset, store_width, (*store_addr), (*store_addr) );
        }

	uint16_t getStoreWidth() const {
		return store_width;
	}

	virtual uint16_t getRegisterOffset() const {
		return 0;
	}

	uint16_t getMemoryAddressRegister() const { return phys_int_regs_in[0]; }
	uint16_t getValueRegister() const { return phys_int_regs_in[1]; }
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
