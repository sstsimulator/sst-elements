
#ifndef _H_VANADIS_STORE
#define _H_VANADIS_STORE

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
		const uint64_t offst,
		const uint16_t valueReg,
		const uint16_t store_bytes) :
		VanadisInstruction(id, addr, hw_thr, isa_opts, 2, 0, 2, 0, 0, 0, 0, 0),
		store_width(store_bytes), offset(offst) {

		isa_int_regs_in[0] = memoryAddr;
		isa_int_regs_in[1] = valueReg;
	}

	VanadisStoreInstruction* clone() {
		return new VanadisStoreInstruction( *this );
	}

	virtual VanadisFunctionalUnitType getInstFuncType() const {
		return INST_STORE;
	}

	virtual const char* getInstCode() const {
		return "STORE";
	}

	virtual void printToBuffer(char* buffer, size_t buffer_size) {
		snprintf(buffer, buffer_size, "STORE    %5" PRIu16 " -> memory[%5" PRIu16 " + %" PRIu64 "] (phys: %5" PRIu16 " -> memory[%5" PRIu16 " + %" PRIu64 "])\n",
			isa_int_regs_in[1], isa_int_regs_in[0], offset,
			phys_int_regs_in[1], phys_int_regs_in[0], offset);
        }

	virtual void execute( SST::Output* output, VanadisRegisterFile* regFile ) {
		markExecuted();
	}

	uint64_t computeStoreAddress( VanadisRegisterFile* reg ) const {
		return (*((uint64_t*) reg->getIntReg( phys_int_regs_in[0] ))) + offset;
	}

	uint16_t getStoreWidth() const {
		return store_width;
	}

	uint16_t getMemoryAddressRegister() const { return phys_int_regs_in[0]; }
	uint16_t getValueRegister() const { return phys_int_regs_in[1]; }
	VanadisStoreRegisterType getValueRegisterType() const { return STORE_INT_REGISTER; }

protected:
	const uint64_t offset;
	const uint16_t store_width;

};

}
}

#endif
