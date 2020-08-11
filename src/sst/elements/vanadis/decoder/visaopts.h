

#ifndef _H_VANADIS_ISA_OPTIONS
#define _H_VANADIS_ISA_OPTIONS

#include <cstdint>
#include <cinttypes>

namespace SST {
namespace Vanadis {

class VanadisDecoderOptions {
public:
        VanadisDecoderOptions(
                const uint16_t reg_ignore) :
                reg_ignore_writes(reg_ignore) {
        }

        VanadisDecoderOptions() :
		reg_ignore_writes(0) {

	}

        uint16_t getRegisterIgnoreWrites() const { return reg_ignore_writes; }

protected:
        const uint16_t reg_ignore_writes;

};

}
}

#endif
