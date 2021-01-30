
#ifndef _H_VANADIS_DIV_REMAIN
#define _H_VANADIS_DIV_REMAIN

#include "inst/vinst.h"

namespace SST {
namespace Vanadis {

class VanadisDivideRemainderInstruction : public VanadisInstruction {
public:
	VanadisDivideRemainderInstruction(
		const uint64_t addr,
		const uint32_t hw_thr,
		const VanadisDecoderOptions* isa_opts,
		const uint16_t quo_dest,
		const uint16_t remain_dest,
		const uint16_t src_1,
		const uint16_t src_2,
		bool treatSignd,
		VanadisRegisterFormat fmt) :
		VanadisInstruction(addr, hw_thr, isa_opts, 2, 2, 2, 2, 0, 0, 0, 0),
			performSigned(treatSignd), reg_format(fmt) {

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
#ifdef VANADIS_BUILD_DEBUG
		output->verbose(CALL_INFO, 16, 0, "Execute: (addr=%p) DIVREM%c q: %" PRIu16 " r: %" PRIu16 " <- %" PRIu16 " \\ %" PRIu16 " (phys: q: %" PRIu16 " r: %" PRIu16 " %" PRIu16 " %" PRIu16 ")\n",
			(void*) getInstructionAddress(), performSigned ? ' ' : 'U',
			isa_int_regs_out[0], isa_int_regs_out[1], isa_int_regs_in[0], isa_int_regs_in[1],
			phys_int_regs_out[0], phys_int_regs_out[1], phys_int_regs_in[0], phys_int_regs_in[1]);
#endif
		if( performSigned ) {
			switch( reg_format ) {
			case VANADIS_FORMAT_INT64:
				{
			                const int64_t src_1 = regFile->getIntReg<int64_t>( phys_int_regs_in[0] );
			       	        const int64_t src_2 = regFile->getIntReg<int64_t>( phys_int_regs_in[1] );

					if( 0 == src_2 ) {
						flagError();
					} else {
						const int64_t quo = (src_1) / (src_2);
						const int64_t mod = (src_1) % (src_2);
#ifdef VANADIS_BUILD_DEBUG
						output->verbose(CALL_INFO, 16, 0, "--> Execute: (detailed, signed, DIVREM64) %" PRId64 " / %" PRId64 " = (q: %" PRId64 ", r: %" PRId64 ")\n",
							src_1, src_2, quo, mod);
#endif
						regFile->setIntReg<int64_t>( phys_int_regs_out[0], quo, true );
						regFile->setIntReg<int64_t>( phys_int_regs_out[1], mod, true );
					}
				}
				break;
			case VANADIS_FORMAT_INT32:
				{
			                const int32_t src_1 = regFile->getIntReg<int32_t>( phys_int_regs_in[0] );
			       	        const int32_t src_2 = regFile->getIntReg<int32_t>( phys_int_regs_in[1] );

					if( 0 == src_2 ) {
						flagError();
					} else {
						const int32_t quo = (src_1) / (src_2);
						const int32_t mod = (src_1) % (src_2);
#ifdef VANADIS_BUILD_DEBUG
						output->verbose(CALL_INFO, 16, 0, "--> Execute: (detailed, signed, DIVREM32) %" PRId32 " / %" PRId32 " = (q: %" PRId32 ", r: %" PRId32 ")\n",
							src_1, src_2, quo, mod);
#endif
						regFile->setIntReg<int32_t>( phys_int_regs_out[0], quo, true );
						regFile->setIntReg<int32_t>( phys_int_regs_out[1], mod, true );
					}
				}
				break;
			default:
				{
					flagError();
				}
				break;
			}
		} else {
			switch( reg_format ) {
			case VANADIS_FORMAT_INT64:
				{
			                const uint64_t src_1 = regFile->getIntReg<uint64_t>( phys_int_regs_in[0] );
			       	        const uint64_t src_2 = regFile->getIntReg<uint64_t>( phys_int_regs_in[1] );

					if( 0 == src_2 ) {
						// Behavior of a DIVU in MIPS is undefined
						// flagError();
					} else {
						const uint64_t quo = (src_1) / (src_2);
						const uint64_t mod = (src_1) % (src_2);
#ifdef VANADIS_BUILD_DEBUG
						output->verbose(CALL_INFO, 16, 0, "--> Execute: (detailed, unsigned, DIVREM64) %" PRIu64 " / %" PRIu64 " = (q: %" PRIu64 ", r: %" PRIu64 ")\n",
							src_1, src_2, quo, mod);
#endif
						regFile->setIntReg<uint64_t>( phys_int_regs_out[0], quo, false );
						regFile->setIntReg<uint64_t>( phys_int_regs_out[1], mod, false );
					}
				}
				break;
			case VANADIS_FORMAT_INT32:
				{
			                const uint32_t src_1 = regFile->getIntReg<uint32_t>( phys_int_regs_in[0] );
			       	        const uint32_t src_2 = regFile->getIntReg<uint32_t>( phys_int_regs_in[1] );

					if( 0 == src_2 ) {
						// Behavior of a DIVU in MIPS is undefined
						// flagError();
					} else {
						const uint32_t quo = (src_1) / (src_2);
						const uint32_t mod = (src_1) % (src_2);
#ifdef VANADIS_BUILD_DEBUG
						output->verbose(CALL_INFO, 16, 0, "--> Execute: (detailed, unsigned, DIVREM32) %" PRIu32 " / %" PRIu32 " = (q: %" PRIu32 ", r: %" PRIu32 ")\n",
							src_1, src_2, quo, mod);
#endif
						regFile->setIntReg<uint32_t>( phys_int_regs_out[0], quo, false );
						regFile->setIntReg<uint32_t>( phys_int_regs_out[1], mod, false );
					}
				}
				break;
			default:
				{
					flagError();
				}
				break;
			}
		}

		markExecuted();
	}

protected:
	VanadisRegisterFormat reg_format;
	const bool performSigned;

};

}
}

#endif
