
#ifndef _H_VANADIS_ISA_OPTIONS
#define _H_VANADIS_ISA_OPTIONS

#include <cstdint>
#include <cinttypes>

namespace SST {
namespace Vanadis {

class VanadisDecoderOptions {
public:
        VanadisDecoderOptions(
                const uint16_t reg_ignore,
		const uint16_t isa_int_reg_c,
		const uint16_t isa_fp_reg_c,
		const uint16_t isa_sysc_reg) :
                reg_ignore_writes(reg_ignore),
		isa_int_reg_count(isa_int_reg_c),
		isa_fp_reg_count(isa_fp_reg_c),
		isa_syscall_code_reg(isa_sysc_reg) {
        }

        VanadisDecoderOptions() :
		reg_ignore_writes(0),
		isa_int_reg_count(0),
		isa_fp_reg_count(0),
		isa_syscall_code_reg(0) {
	}

        uint16_t getRegisterIgnoreWrites() const { return reg_ignore_writes; }
	uint16_t countISAIntRegisters() const { return isa_int_reg_count;    }
	uint16_t countISAFPRegisters() const { return isa_fp_reg_count;      }
	uint16_t getISASysCallCodeReg() const { return isa_syscall_code_reg; }

protected:
        const uint16_t reg_ignore_writes;
	const uint16_t isa_int_reg_count;
	const uint16_t isa_fp_reg_count;
	const uint16_t isa_syscall_code_reg;

};

}
}

#endif
