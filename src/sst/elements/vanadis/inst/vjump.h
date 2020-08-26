
#ifndef _H_VANADIS_JUMP
#define _H_VANADIS_JUMP

#include <cstdio>

#include "inst/vinst.h"
#include "inst/vspeculate.h"

namespace SST {
namespace Vanadis {

class VanadisJumpInstruction : public VanadisSpeculatedInstruction {

public:
	VanadisJumpInstruction(
		const uint64_t id,
                const uint64_t addr,
                const uint32_t hw_thr,
                const VanadisDecoderOptions* isa_opts,
		const uint64_t pc,
		const VanadisDelaySlotRequirement delayT
		) :
		VanadisSpeculatedInstruction(id, addr, hw_thr, isa_opts,
			0,0,0,0,0,0,0,0, delayT ), new_pc(pc) {}

		virtual uint64_t getSpeculatePCAddress( const VanadisRegisterFile* reg_file ) {
			return new_pc;
		}

		virtual const char* getInstCode() const {
                	return "JMP";
        	}

		virtual void printToBuffer(char* buffer, size_t buffer_size) {
			snprintf(buffer, buffer_size, "JUMP    %" PRIu64 "", new_pc);
		}

		virtual void execute( SST::Output* output, VanadisRegisterFile* regFile ) {
			markExecuted();
		}
protected:
	const uint64_t new_pc;

};

}
}

#endif