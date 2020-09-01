
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
		const uint64_t id,
		const uint64_t addr,
		const uint32_t hw_thr,
		const VanadisDecoderOptions* isa_opts,
		const uint16_t memAddrReg,
		const int64_t offst,
		const uint16_t tgtReg,
		const uint16_t load_bytes,
		const bool extend_sign,
		const VanadisMemoryTransaction accessT
		) : VanadisInstruction(id, addr, hw_thr, isa_opts, 1, 1, 1, 1, 0, 0, 0, 0),
			offset(offst), load_width(load_bytes), signed_extend(extend_sign),
			memAccessType(accessT) {

		isa_int_regs_out[0] = tgtReg;
		isa_int_regs_in[0]  = memAddrReg;
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
			return "LOAD";
		}
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
		snprintf( buffer, buffer_size, "LOAD (%s)  %5" PRIu16 " <- memory[ %5" PRIu16 " + %20" PRId64 " (phys: %5" PRIu16 " <- memory[%5" PRIu16 " + %20" PRId64 "])\n",
			getTransactionTypeString( memAccessType ),
			isa_int_regs_out[0], isa_int_regs_in[0], offset,
			phys_int_regs_out[0], phys_int_regs_in[0], offset);
	}

	virtual void computeLoadAddress( VanadisRegisterFile* reg, uint64_t* out_addr, uint16_t* width ) {
		(*out_addr) = (*((uint64_t*) reg->getIntReg( phys_int_regs_in[0]))) + offset ;
		(*width)    = load_width;
	}

	virtual void computeLoadAddress( SST::Output* output, VanadisRegisterFile* regFile, uint64_t* out_addr, uint16_t* width ) {
		const uint64_t* mem_addr_reg_ptr = (uint64_t*) regFile->getIntReg( phys_int_regs_in[0] );
                const uint64_t mem_addr_reg_val = (*mem_addr_reg_ptr);

		output->verbose(CALL_INFO, 16, 0, "[execute-load]: transaction-type:  %s\n", getTransactionTypeString( memAccessType ) );
                output->verbose(CALL_INFO, 16, 0, "[execute-load]: reg[%5" PRIu16 "]:       %" PRIu64 "\n", phys_int_regs_in[0], mem_addr_reg_val);
                output->verbose(CALL_INFO, 16, 0, "[execute-load]: offset           : %" PRIu64 "\n", offset);
                output->verbose(CALL_INFO, 16, 0, "[execute-load]: (add)            : %" PRIu64 "\n", (mem_addr_reg_val + offset));

		(*out_addr) = (*((uint64_t*) regFile->getIntReg( phys_int_regs_in[0]))) + offset ;
		(*width)    = load_width;
	}

	virtual uint16_t getLoadWidth() const { return load_width; }

	VanadisLoadRegisterType getValueRegisterType() const {
		return LOAD_INT_REGISTER;
	}

	virtual uint16_t getRegisterOffset() const {
		return 0;
	}

protected:
	const bool signed_extend;
	VanadisMemoryTransaction memAccessType;
	const int64_t offset;
	const uint16_t load_width;

};

}
}

#endif
