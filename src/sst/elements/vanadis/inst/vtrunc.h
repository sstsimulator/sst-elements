
#ifndef _H_VANADIS_TRUNCATE
#define _H_VANADIS_TRUNCATE

#include "inst/vinst.h"

namespace SST {
namespace Vanadis {

class VanadisTruncateInstruction : public VanadisInstruction {
public:
	VanadisTruncateInstruction(
		const uint64_t addr,
		const uint32_t hw_thr,
		const VanadisDecoderOptions* isa_opts,
		const uint16_t dest,
		const uint16_t src,
		VanadisRegisterFormat input_fmt,
		VanadisRegisterFormat output_fmt) :
		VanadisInstruction(addr, hw_thr, isa_opts, 1, 1, 1, 1, 0, 0, 0, 0),
			reg_input_format(input_fmt),
			reg_output_format(output_fmt) {

		isa_int_regs_in[0]  = src;
		isa_int_regs_out[0] = dest;
	}

	VanadisTruncateInstruction* clone() {
		return new VanadisTruncateInstruction( *this );
	}

	virtual VanadisFunctionalUnitType getInstFuncType() const {
		return INST_INT_ARITH;
	}

	virtual const char* getInstCode() const {
		return "TRUNC";
	}

	virtual void printToBuffer(char* buffer, size_t buffer_size) {
		snprintf(buffer, buffer_size, "%s    %5" PRIu16 " <- %5" PRIu16 " (phys: %5" PRIu16 " <- %5" PRIu16 ")\n",
			getInstCode(), isa_int_regs_out[0], isa_int_regs_in[0],
			phys_int_regs_out[0], phys_int_regs_in[0]);
        }

	virtual void execute( SST::Output* output, VanadisRegisterFile* regFile ) {
		output->verbose(CALL_INFO, 16, 0, "Execute: (addr=%p) %s phys: out=%" PRIu16 " in=%" PRIu16 ", isa: out=%" PRIu16 " / in=%" PRIu16 "\n",
			(void*) getInstructionAddress(), getInstCode(),
			phys_int_regs_out[0], phys_int_regs_in[0],
			isa_int_regs_out[0], isa_int_regs_in[0] );

		switch( reg_input_format ) {
		case VANADIS_FORMAT_INT64:
			{
				switch( reg_output_format ) {
				case VANADIS_FORMAT_INT64:
					{
						const int64_t src = regFile->getIntReg<int64_t>( phys_int_regs_in[0] );
						regFile->setIntReg<int64_t>( phys_int_regs_out[0], src );
					}
					break;
				case VANADIS_FORMAT_INT32:
					{
						const uint32_t src = regFile->getIntReg<int32_t>( phys_int_regs_in[0] );
						regFile->setIntReg<int32_t>( phys_int_regs_out[0], src );
					}
					break;
				default:
					{
						flagError();
					}
					break;
				}
			}
			break;
		case VANADIS_FORMAT_INT32:
			{
				switch( reg_output_format ) {
				case VANADIS_FORMAT_INT64:
					{
						const int64_t src = regFile->getIntReg<int64_t>( phys_int_regs_in[0] );
						regFile->setIntReg<int64_t>( phys_int_regs_out[0], src );
					}
					break;
				case VANADIS_FORMAT_INT32:
					{
						const int32_t src = regFile->getIntReg<int32_t>( phys_int_regs_in[0] );
						regFile->setIntReg<int32_t>( phys_int_regs_out[0], src );
					}
					break;
				default:
					{
						flagError();
					}
					break;
				}
			}
			break;
		default:
			{
				flagError();
			}
			break;
		}

		markExecuted();
	}

protected:
	VanadisRegisterFormat reg_input_format;
	VanadisRegisterFormat reg_output_format;

};

}
}

#endif
