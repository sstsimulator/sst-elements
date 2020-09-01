
#ifndef _H_VANADIS_DIV_REMAIN
#define _H_VANADIS_DIV_REMAIN

#include "inst/vinst.h"

namespace SST {
namespace Vanadis {

class VanadisDivideRemainderInstruction : public VanadisInstruction {
public:
	VanadisDivideRemainderInstruction(
		const uint64_t id,
		const uint64_t addr,
		const uint32_t hw_thr,
		const VanadisDecoderOptions* isa_opts,
		const uint16_t quo_dest,
		const uint16_t remain_dest,
		const uint16_t src_1,
		const uint16_t src_2,
		bool treatSignd) :
		VanadisInstruction(id, addr, hw_thr, isa_opts, 2, 2, 2, 2, 0, 0, 0, 0),
			performSigned(treatSignd) {

		isa_int_regs_in[0]  = src_1;
		isa_int_regs_in[1]  = src_2;
		isa_int_regs_out[0] = quo_dest;
		isa_int_regs_out[1] = remain_dest;
	}

	VanadisDivideRemainderInstruction* clone() {
		return new VanadisDivideRemainderInstruction( *this );
	}

	virtual VanadisFunctionalUnitType getInstFuncType() const {
		return INST_INT_DIV;
	}

	virtual const char* getInstCode() const {
		return "DIVREM";
	}

	virtual void printToBuffer(char* buffer, size_t buffer_size) {
		snprintf(buffer, buffer_size, "DIVREM%c q: %5" PRIu16 " r: %" PRIu16 " <- %" PRIu16 " \\ %" PRIu16 ", (phys: q: %" PRIu16 " r: %" PRIu16 " r: %" PRIu16 " %" PRIu16" )\n",
			performSigned ? ' ' :  'U', isa_int_regs_out[0], isa_int_regs_out[1], isa_int_regs_in[0], isa_int_regs_in[1],
			phys_int_regs_out[0], phys_int_regs_out[1], phys_int_regs_in[0], phys_int_regs_in[1]);
        }

	virtual void execute( SST::Output* output, VanadisRegisterFile* regFile ) {
		output->verbose(CALL_INFO, 16, 0, "Execute: (addr=%p) DIVREM%c q: %" PRIu16 " r: %" PRIu16 " <- %" PRIu16 " \\ %" PRIu16 " (phys: q: %" PRIu16 " r: %" PRIu16 " %" PRIu16 " %" PRIu16 ")\n",
			(void*) getInstructionAddress(), performSigned ? ' ' : 'U',
			isa_int_regs_out[0], isa_int_regs_out[1], isa_int_regs_in[0], isa_int_regs_in[1],
			phys_int_regs_out[0], phys_int_regs_out[1], phys_int_regs_in[0], phys_int_regs_in[1]);

		if( performSigned ) {
	                int64_t src_1 = 0;
	       	        int64_t src_2 = 0;

	                regFile->getIntReg( phys_int_regs_in[0], &src_1 );
	                regFile->getIntReg( phys_int_regs_in[1], &src_2 );

			if( 0 == src_2 ) {
				flagError();
			} else {
				const int64_t quo = (src_1) / (src_2);
				const int64_t mod = (src_1) % (src_2);

				regFile->setIntReg( phys_int_regs_out[0], quo );
				regFile->setIntReg( phys_int_regs_out[1], mod );
			}
		} else {
	                uint64_t src_1 = 0;
	       	        uint64_t src_2 = 0;

	                regFile->getIntReg( phys_int_regs_in[0], &src_1 );
	                regFile->getIntReg( phys_int_regs_in[1], &src_2 );

			if( 0 == src_2 ) {
				flagError();
			} else {
				const uint64_t quo = (src_1) / (src_2);
				const uint64_t mod = (src_1) % (src_2);

				regFile->setIntReg( phys_int_regs_out[0], quo );
				regFile->setIntReg( phys_int_regs_out[1], mod );
			}
		}

		markExecuted();
	}

protected:
	const bool performSigned;

};

}
}

#endif
