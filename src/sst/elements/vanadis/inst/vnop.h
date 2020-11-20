
#ifndef _H_VANADIS_NOP
#define _H_VANADIS_NOP

#include "inst/vinst.h"

namespace SST {
namespace Vanadis {

class VanadisNoOpInstruction : public VanadisInstruction {
public:
	VanadisNoOpInstruction(
		const uint64_t addr,
		const uint32_t hw_thr,
		const VanadisDecoderOptions* isa_opts) :
		VanadisInstruction(addr, hw_thr, isa_opts, 0, 0, 0, 0, 0, 0, 0, 0) {}

	VanadisNoOpInstruction* clone() {
		return new VanadisNoOpInstruction( *this );
	}

	virtual VanadisFunctionalUnitType getInstFuncType() const {
		return INST_NOOP;
	}

	virtual const char* getInstCode() const {
		return "NOP";
	}

	virtual void printToBuffer(char* buffer, size_t buffer_size) {
                snprintf(buffer, buffer_size, "NOP");
        }

	virtual void execute( SST::Output* output, VanadisRegisterFile* regFile ) {
		markExecuted();
	}

};

}
}

#endif
