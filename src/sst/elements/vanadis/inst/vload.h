
#ifndef _H_VANADIS_LOAD
#define _H_VANADIS_LOAD

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
		const uint64_t offst,
		const uint16_t tgtReg,
		const uint16_t load_bytes
		) : VanadisInstruction(id, addr, hw_thr, isa_opts, 1, 1, 1, 1, 0, 0, 0, 0),
			offset(offst), load_width(load_bytes) {

		isa_int_regs_out[0] = tgtReg;
		isa_int_regs_in[1]  = memAddrReg;
	}

	VanadisLoadInstruction* clone() {
		return new VanadisLoadInstruction( *this );
	}

	virtual VanadisFunctionalUnitType getInstFuncType() const { return INST_LOAD; }
	virtual const char* getInstCode() const { return "LOAD"; }

	uint16_t getMemoryAddressRegister() const { return phys_int_regs_in[1]; }
	uint16_t getTargetRegister() const { return phys_int_regs_in[0]; }

	virtual void execute( SST::Output* output, VanadisRegisterFile* regFile ) {
		markExecuted();
	}

	virtual void printToBuffer( char* buffer, size_t buffer_size ) {
		snprintf( buffer, buffer_size, "LOAD     %5" PRIu16 " <- memory[ %5" PRIu16 " + %15" PRIu64 " (phys: %5" PRIu16 " <- memory[%5" PRIu16 " + %20" PRIu64 "])\n",
			isa_int_regs_out[0], isa_int_regs_in[1], offset,
			phys_int_regs_out[0], phys_int_regs_in[1], offset);
	}

	uint64_t computeLoadAddress( VanadisRegisterFile* reg ) const {
		return (*((uint64_t*) reg->getIntReg( phys_int_regs_in[0]))) + offset ;
	}

	uint16_t getLoadWidth() const { return load_width; }

	VanadisLoadRegisterType getValueRegisterType() const {
		return LOAD_INT_REGISTER;
	}

protected:
	const uint64_t offset;
	const uint16_t load_width;

};

}
}

#endif
