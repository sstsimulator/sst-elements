
#ifndef _H_VANADIS_STORE_PARTIAL
#define _H_VANADIS_STORE_PARTIAL

#include "inst/vinst.h"

namespace SST {
namespace Vanadis {

class VanadisPartialStoreInstruction : public VanadisStoreInstruction {

public:
	VanadisPartialStoreInstruction(
		const uint64_t id,
		const uint64_t addr,
		const uint32_t hw_thr,
		const VanadisDecoderOptions* isa_opts,
		const uint16_t memoryAddr,
		const uint64_t offst,
		const uint16_t valueReg,
		const uint16_t store_bytes,
		const bool isLeftStore) :
		VanadisStoreInstruction(id, addr, hw_thr, isa_opts, memoryAddr, offst, valueReg, store_bytes),
			is_left_store(isLeftStore) {

		register_offset = 0;
	}

	virtual bool isPartialStore() {
		return true;
	}

	VanadisPartialStoreInstruction* clone() {
		return new VanadisPartialStoreInstruction( *this );
	}

	virtual VanadisFunctionalUnitType getInstFuncType() const {
		return INST_STORE;
	}

	virtual const char* getInstCode() const {
		return "PARTSTORE";
	}

	virtual void printToBuffer(char* buffer, size_t buffer_size) {
		snprintf(buffer, buffer_size, "PARTSTORE    %5" PRIu16 " -> memory[%5" PRIu16 " + %" PRIu64 "] (phys: %5" PRIu16 " -> memory[%5" PRIu16 " + %" PRIu64 "])\n",
			isa_int_regs_in[1], isa_int_regs_in[0], offset,
			phys_int_regs_in[1], phys_int_regs_in[0], offset);
        }

	virtual void execute( SST::Output* output, VanadisRegisterFile* regFile ) {
		markExecuted();
	}

	virtual void computeStoreAddress( SST::Output* output, VanadisRegisterFile* reg, uint64_t* store_addr, uint16_t* op_width ) {
		output->verbose(CALL_INFO, 16, 0, "[partial-store]: compute base address: phys-reg: %" PRIu16 " / offset: %" PRIu64 " / 0x%0llx\n",
			phys_int_regs_in[0], offset, offset);
		const uint64_t base_addr = (*((uint64_t*) reg->getIntReg( phys_int_regs_in[0] ))) + offset;
		const uint64_t width_64  = (uint64_t) store_width;

		const uint64_t right_len = (base_addr % width_64) == 0 ? width_64 : (base_addr % width_64);
		const uint64_t left_len  = (width_64 - right_len) == 0 ? width_64 : (width_64 - right_len);

		output->verbose(CALL_INFO, 16, 0, "[partial-store]: base_addr: 0x%0llx full-width: %" PRIu64 "\n", base_addr, width_64);
		output->verbose(CALL_INFO, 16, 0, "[partial-store]: store-type: %s\n", (is_left_store) ? "left" : "right");
		output->verbose(CALL_INFO, 16, 0, "[partial-store]: partial-width: %" PRIu64 "\n",
			(is_left_store) ? left_len : right_len );

		if( is_left_store ) {
			(*store_addr) = base_addr;
			(*op_width) = left_len;
			register_offset = 0;
		} else {
			if( left_len == width_64 ) {
				(*store_addr) = base_addr;
				register_offset = 0;
			} else {
				(*store_addr) = base_addr + left_len;
				register_offset = left_len;
			}

			(*op_width) = right_len;
		}

		output->verbose(CALL_INFO, 16, 0, "[partial-store]: store-addr: 0x%0llu / store-width: %" PRIu16 " / reg-offset: %" PRIu16 "\n",
			(*store_addr), (*op_width), register_offset);
	}

	uint16_t getStoreWidth() const {
		return store_width;
	}

	virtual uint16_t getRegisterOffset() const {
		return register_offset;
	}

	uint16_t getMemoryAddressRegister() const { return phys_int_regs_in[0]; }
	uint16_t getValueRegister() const { return phys_int_regs_in[1]; }
	VanadisStoreRegisterType getValueRegisterType() const { return STORE_INT_REGISTER; }

protected:
	uint16_t register_offset;
	const bool is_left_store;

};

}
}

#endif
