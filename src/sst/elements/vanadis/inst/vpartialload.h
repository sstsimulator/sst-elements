
#ifndef _H_VANADIS_PARTIAL_LOAD
#define _H_VANADIS_PARTIAL_LOAD

namespace SST {
namespace Vanadis {

class VanadisPartialLoadInstruction : public VanadisLoadInstruction {

public:
	VanadisPartialLoadInstruction(
		const uint64_t id,
		const uint64_t addr,
		const uint32_t hw_thr,
		const VanadisDecoderOptions* isa_opts,
		const uint16_t memAddrReg,
		const int64_t offst,
		const uint16_t tgtReg,
		const uint16_t load_bytes,
		const bool extend_sign,
		const bool isLeftLoad
		) : VanadisLoadInstruction(id, addr, hw_thr, isa_opts, memAddrReg, offst, tgtReg, load_bytes, extend_sign),
			is_load_lower(isLeftLoad) {

		// We need an extra in register here

		delete[] phys_int_regs_in;
		delete[] isa_int_regs_in;

		count_isa_int_reg_in  = 2;
		count_phys_int_reg_in = 2;

		phys_int_regs_in = new uint16_t[ count_phys_int_reg_in ];
		isa_int_regs_in  = new uint16_t[ count_isa_int_reg_in  ];

		isa_int_regs_out[0] = tgtReg;
		isa_int_regs_in[0]  = memAddrReg;
		isa_int_regs_in[1]  = tgtReg;

		register_offset = 0;
	}

	VanadisPartialLoadInstruction* clone() {
		return new VanadisPartialLoadInstruction( *this );
	}

	bool isPartialLoad() const { return true; }
	bool performSignExtension() const { return signed_extend; }

	virtual VanadisFunctionalUnitType getInstFuncType() const { return INST_LOAD; }
	virtual const char* getInstCode() const { return "PARTLOAD"; }

	uint16_t getMemoryAddressRegister() const { return phys_int_regs_in[1]; }
	uint16_t getTargetRegister() const { return phys_int_regs_in[0]; }

	virtual void execute( SST::Output* output, VanadisRegisterFile* regFile ) {
		markExecuted();
	}

	virtual void printToBuffer( char* buffer, size_t buffer_size ) {
		snprintf( buffer, buffer_size, "PARTLOAD  %5" PRIu16 " <- memory[ %5" PRIu16 " + %20" PRId64 " (phys: %5" PRIu16 " <- memory[%5" PRIu16 " + %20" PRId64 "]) / width: %" PRIu16 "\n",
			isa_int_regs_out[0], isa_int_regs_in[0], offset,
			phys_int_regs_out[0], phys_int_regs_in[0], offset, load_width);
	}

	void computeLoadAddress( VanadisRegisterFile* reg, uint64_t* out_addr, uint16_t* width ) {

		const uint64_t width_64       = (uint64_t) load_width;
		const uint64_t base_address   = (*((uint64_t*) reg->getIntReg( phys_int_regs_in[0]))) + offset;
		const uint64_t right_read_len = (base_address % width_64) == 0 ? width_64 : (base_address % width_64);
		const uint64_t left_read_len  = (width_64 - right_read_len) == 0 ? width_64 : (width_64 - right_read_len);

		if( is_load_lower ) {
			(*out_addr) = base_address;
			(*width)    = left_read_len;
			register_offset = 0;
		} else {
			// if LWR and LWL are both performing full loads, then ensure they load from the same address
			if( width_64 == left_read_len ) {
				(*out_addr) = base_address;
				register_offset = 0;
			} else {
				(*out_addr) = base_address + left_read_len;
				register_offset = left_read_len;
			}

			(*width)    = right_read_len;
		}
	}

	void computeLoadAddress( SST::Output* output, VanadisRegisterFile* regFile, uint64_t* out_addr, uint16_t* width ) {
		const uint64_t mem_addr_reg_val = *((uint64_t*) regFile->getIntReg( phys_int_regs_in[0] ));

                output->verbose(CALL_INFO, 16, 0, "[execute-partload]: reg[%5" PRIu16 "]: %" PRIu64 "\n", phys_int_regs_in[0], mem_addr_reg_val);
                output->verbose(CALL_INFO, 16, 0, "[execute-partload]: offset           : %" PRIu64 "\n", offset);
                output->verbose(CALL_INFO, 16, 0, "[execute-partload]: (add)            : %" PRIu64 "\n", (mem_addr_reg_val + offset));

		computeLoadAddress( regFile, out_addr, width );

		output->verbose(CALL_INFO, 16, 0, "[execute-partload]: full width: %" PRIu16 "\n", load_width);
		output->verbose(CALL_INFO, 16, 0, "[execute-partload]: (left/right load ? %s)\n", is_load_lower ? "left" : "right");
		output->verbose(CALL_INFO, 16, 0, "[execute-partload]: load-addr: 0x%0llx / load-width: %" PRIu16 "\n",
			(*out_addr), (*width) );
		output->verbose(CALL_INFO, 16, 0, "[execute-partload]: register-offset: %" PRIu16 "\n", register_offset);
	}

	uint16_t getLoadWidth() const { return load_width; }

	VanadisLoadRegisterType getValueRegisterType() const {
		return LOAD_INT_REGISTER;
	}

	uint16_t getRegisterOffset() const {
		return register_offset;
	}

protected:
	uint16_t register_offset;
	const bool is_load_lower;

};

}
}

#endif
