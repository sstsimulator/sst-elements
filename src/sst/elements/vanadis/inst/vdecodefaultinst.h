
#ifndef _H_VANADIS_INSTRUCTION_DECODE_FAULT
#define _H_VANADIS_INSTRUCTION_DECODE_FAULT

#include "inst/vinst.h"
#include "decoder/visaopts.h"
#include "inst/vinsttype.h"

namespace SST {
namespace Vanadis {

class VanadisInstructionDecodeFault : public VanadisInstruction {
public:
	VanadisInstructionDecodeFault(
		const uint64_t ins_id,
		const uint64_t address,
		const uint32_t hw_thr,
		const VanadisDecoderOptions* isa_opts) :
		VanadisInstruction( ins_id, address, hw_thr, isa_opts, 0, 0, 0, 0, 0, 0, 0, 0 ) {

		trapError = true;
	}

	virtual VanadisInstruction* clone() {
		return new VanadisInstructionDecodeFault( id, ins_address, hw_thread, isa_options );
	}

	virtual const char* getInstCode() const {
		return "DECODE_FAULT";
	}

	virtual void printToBuffer( char* buffer, size_t buffer_size ) {
		snprintf( buffer, buffer_size, "%s", "DECODE_FAULT" );
	}

	virtual VanadisFunctionalUnitType getInstFuncType() const {
		return INST_FAULT;
	}

	virtual void execute( SST::Output* output, VanadisRegisterFile* regFile ) {
		
	}

};

}
}

#endif
