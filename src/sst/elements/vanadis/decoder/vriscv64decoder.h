
#ifndef _H_VANADIS_RISCV64_DECODER
#define _H_VANADIS_RISCV64_DECODER

#include <cstdint>
#include <cstring>

#include "decoder/vdecoder.h"

#define VANADIS_RISCV_OPCODE_MASK 0x7F
#define VANADIS_RISCV_RD_MASK     0xF80
#define VANADIS_RISCV_RS1_MASK    0xF8000
#define VANADIS_RISCV_RS2_MASK    0x1F00000
#define VANADIS_RISCV_FUNC3_MASK  0x7000
#define VANADIS_RISCV_FUNC7_MASK  0xFE000000
#define VANADIS_RISCV_UIMM12_MASK 0xFFF00000
#define VANADIS_RISCV_UIMM7_MASK  0xFE000000
#define VANADIS_RISCV_LIMM5_MASK  0xF80
#define VANADIS_RISCV_UIMM20_MASK 0xFFFFF000


namespace SST {
namespace Vanadis {

class VanadisRISCV64Decoder : public VanadisDecoder {
public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(VanadisRISCV64Decoder, "vanadis", "VanadisRISCV64Decoder",
                                          SST_ELI_ELEMENT_VERSION(1, 0, 0),
                                          "Implements a RISCV64-compatible decoder for Vanadis CPU processing.",
                                          SST::Vanadis::VanadisDecoder)
    SST_ELI_DOCUMENT_PARAMS({ "decode_max_ins_per_cycle", "Maximum number of instructions that can be "
                                                          "decoded and issued per cycle" },
                            { "uop_cache_entries", "Number of micro-op cache entries, this "
                                                   "corresponds to ISA-level instruction counts." },
                            { "predecode_cache_entries",
                              "Number of cache lines that a cached prior to decoding (these support "
                              "loading from cache prior to decode)" },
                            { "stack_start_address", "Sets the start of the stack and dynamic program segments" })

    VanadisRISCV64Decoder(ComponentId_t id, Params& params) : VanadisDecoder(id, params) {
        options = new VanadisDecoderOptions( static_cast<uint16_t>(0), 32, 32, 2, VANADIS_REGISTER_MODE_FP64);
        max_decodes_per_cycle = params.find<uint16_t>("decode_max_ins_per_cycle", 2);

        // MIPS default is 0x7fffffff according to SYS-V manual
        start_stack_address = params.find<uint64_t>("stack_start_address", 0x7ffffff0);

        // See if we get an entry point the sub-component says we have to use
        // if not, we will fall back to ELF reading at the core level to work this
        // out
	setInstructionPointer(params.find<uint64_t>("entry_point", 0));
    }

    ~VanadisRISCV64Decoder() {}

protected:
    const VanadisDecoderOptions* options;

};

}
}

#endif
