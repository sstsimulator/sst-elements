

#ifndef _H_VANADIS_MIPS_DECODER
#define _H_VANADIS_MIPS_DECODER

#include "decoder/vdecoder.h"
#include "vinsloader.h"
#include "inst/vinstall.h"

#include <list>

#define MIPS_OP_MASK        0b11111100000000000000000000000000
#define MIPS_RS_MASK        0b00000011111000000000000000000000
#define MIPS_RT_MASK        0b00000000000111110000000000000000
#define MIPS_RD_MASK        0b00000000000000001111100000000000
#define MIPS_ADDR_MASK      0b00000111111111111111111111111111
#define MIPS_IMM_MASK       0b00000000000000001111111111111111
#define MIPS_SHFT_MASK      0b00000000000000000000011111000000
#define MIPS_FUNC_MASK      0b00000000000000000000000000111111

#define MIPS_ADDI_OP_MASK   0b00110000000000000000000000000000

namespace SST {
namespace Vanadis {

class VanadisMIPSDecoder : public VanadisDecoder {
public:
	SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
		VanadisMIPSDecoder,
		"vanadis",
		"VanadisMIPSDecoder",
		SST_ELI_ELEMENT_VERSION(1,0,0),
		"Implements a MIPS-compatible decoder for Vanadis CPU processing.",
		SST::Vanadis::VanadisDecoder )

	SST_ELI_DOCUMENT_PARAMS(
		{ "decode_max_ins_per_cycle", 		"Maximum number of instructions that can be decoded and issued per cycle"  				},
		{ "uop_cache_entries",                  "Number of micro-op cache entries, this corresponds to ISA-level instruction counts."  			},
		{ "predecode_cache_entries",            "Number of cache lines that a cached prior to decoding (these support loading from cache prior to decode)" }
		)

	VanadisMIPSDecoder( ComponentId_t id, Params& params ) :
		VanadisDecoder( id, params ) {

		options = new VanadisDecoderOptions( (uint16_t) 0 );
		max_decodes_per_cycle      = params.find<uint16_t>("decode_max_ins_per_cycle",  2);

		// See if we get an entry point the sub-component says we have to use
		// if not, we will fall back to ELF reading at the core level to work this out
		setInstructionPointer( params.find<uint64_t>("entry_point", 0) );
	}

	~VanadisMIPSDecoder() {}

	virtual const char* getISAName() const { return "MIPS"; }
	virtual uint16_t countISAIntReg() const { return 32; }
	virtual uint16_t countISAFPReg() const { return 32; }
	virtual const VanadisDecoderOptions* getDecoderOptions() const { return options; }

	virtual void tick( SST::Output* output, uint64_t cycle ) {
		output->verbose(CALL_INFO, 16, 0, "-> Decode step for thr: %" PRIu32 "\n", hw_thr);
		output->verbose(CALL_INFO, 16, 0, "---> Max decodes per cycle: %" PRIu16 "\n", max_decodes_per_cycle );

		ins_loader->printStatus( output );

		uint16_t decodes_performed = 0;
		uint16_t uop_bundles_used  = 0;

		for( uint16_t i = 0; i < max_decodes_per_cycle; ++i ) {
			// if the decoded queue has space, then lets go ahead and
			// decode the input, put it in the queue for issue.
			if( ! decoded_q->full() ) {
				if( ins_loader->hasBundleAt( ip ) ) {
					output->verbose(CALL_INFO, 16, 0, "---> Found uop bundle for ip=%p, loading from cache...\n", (void*) ip);
					VanadisInstructionBundle* bundle = ins_loader->getBundleAt( ip )->clone( &next_ins_id );

					// Do we have enough space in the decode queue for the bundle contents?
					if( bundle->getInstructionCount() < (decoded_q->capacity() - decoded_q->size()) ) {
						// Put in the queue
						for( uint32_t i = 0; i < bundle->getInstructionCount(); ++i ) {
							VanadisInstruction* next_ins = bundle->getInstructionByIndex( i );
							output->verbose(CALL_INFO, 16, 0, "---> --> issuing ins id: %" PRIu64 "(addr: %p, %s)...\n",
								next_ins->getID(), (void*) next_ins->getInstructionAddress(),
								next_ins->getInstCode());
							decoded_q->push( next_ins );
						}

						delete bundle;
						uop_bundles_used++;
					} else {
						// We don't have enough space, so we have to stop and wait for more entries to free up.
						break;
					}
				} else if( ins_loader->hasPredecodeAt( ip ) ) {
					// We do have a locally cached copy of the data at the IP though, so decode into a bundle
					output->verbose(CALL_INFO, 16, 0, "---> uop not found, but matched in predecoded L0-icache (ip=%p)\n", (void*) ip);

					uint32_t temp_ins = 0;
					VanadisInstructionBundle* decoded_bundle = new VanadisInstructionBundle( ip );

					if( ins_loader->getPredecodeBytes( output, ip, (uint8_t*) &temp_ins, 4 ) ) {
						decode( output, ip, temp_ins, decoded_bundle );
						ins_loader->cacheDecodedBundle( decoded_bundle );
						decodes_performed++;
						break;
					} else {
						output->fatal(CALL_INFO, -1, "Error: predecoded bytes found at ip=%p, but %d byte retrival failed.\n",
							(void*) ip, (int) sizeof( temp_ins ));
					}
				} else {
					output->verbose(CALL_INFO, 16, 0, "---> uop bundle and pre-decoded bytes are not found (ip=%p), requesting icache read (line-width=%" PRIu64 ")\n",
						(void*) ip, ins_loader->getCacheLineWidth() );
					ins_loader->requestLoadAt( output, ip, 4 );
					break;
				}
			} else {
				output->verbose(CALL_INFO, 16, 0, "---> Decoded pending issue queue is full, no more decodes permitted.\n");
				break;
			}
		}

		output->verbose(CALL_INFO, 16, 0, "---> Performed %" PRIu16 " decodes this cycle, %" PRIu16 " uop-bundles used.\n",
			decodes_performed, uop_bundles_used);
	}

	virtual void clearDecoderAfterMisspeculate() {
		// TODO - what do we need to do here?
	}

protected:
	void extract_imm( const uint32_t ins, uint32_t* imm ) const {
		(*imm) = (ins & MIPS_IMM_MASK);
	}

	void extract_three_regs( const uint32_t ins, uint16_t* rt, uint16_t* rs, uint16_t* rd ) const {
		(*rt) = (ins & MIPS_RT_MASK) >> 16;
		(*rs) = (ins & MIPS_RS_MASK) >> 21;
		(*rd) = (ins & MIPS_RD_MASK) >> 11;
	}

	void decode( SST::Output* output, const uint64_t ins_addr, const uint32_t next_ins, VanadisInstructionBundle* bundle ) {

		const uint32_t ins_mask = next_ins & MIPS_OP_MASK;

		uint16_t rt = 0;
		uint16_t rs = 0;
		uint16_t rd = 0;

		uint32_t imm = 0;

		// Perform a register and parameter extract in case we need later
		extract_three_regs( next_ins, &rt, &rs, &rd );
		extract_imm( next_ins, &imm );

		const uint64_t imm64 = (uint64_t) imm;

		switch( ins_mask ) {
		case 0:
			// Special category
			break;

		case MIPS_ADDI_OP_MASK:
			decoded_q->push( new VanadisAddImmInstruction( getNextInsID(),
				ins_addr, getHardwareThread(), options, rt, rs, imm64 ) );
			break;

		default:
			break;
		}
	}

	const VanadisDecoderOptions* options;

	uint64_t next_ins_id;

	uint16_t icache_max_bytes_per_cycle;
	uint16_t max_decodes_per_cycle;
	uint16_t decode_buffer_max_entries;

/*
	constexpr uint32_t addi_op_mask   = 0b00110000000000000000000000000000;
	constexpr uint32_t beq_op_mask    = 0b00010000000000000000000000000000;
	constexpr uint32_t beql_op_mask   = 0b01010000000000000000000000000000;
	constexpr uint32_t regimm_op_mask = 0b00000100000000000000000000000000;
	constexpr uint32_t bgtz_op_mask   = 0b00011100000000000000000000000000;
	constexpr uint32_t bgtzl_op_mask  = 0b01011100000000000000000000000000;
	constexpr uint32_t blez_op_mask   = 0b00011000000000000000000000000000;
	constexpr uint32_t blezl_op_mask  = 0b01011000000000000000000000000000;
	constexpr uint32_t bne_op_mask    = 0b00010100000000000000000000000000;
	constexpr uint32_t bnel_op_mask   = 0b01010100000000000000000000000000;
	constexpr uint32_t daddi_op_mask  = 0b01100000000000000000000000000000;
	constexpr uint32_t daddiu_op_mask = 0b01100100000000000000000000000000;
	constexpr uint32_t j_op_mask      = 0b00001000000000000000000000000000;
	constexpr uint32_t jal_op_mask    = 0b00001100000000000000000000000000;
	constexpr uint32_t lb_op_mask     = 0b10000000000000000000000000000000;
	constexpr uint32_t lbu_op_mask    = 0b10010000000000000000000000000000;
	constexpr uint32_t ld_op_mask     = 0b11011100000000000000000000000000;
	constexpr uint32_t ldl_op_mask    = 0b01101000000000000000000000000000;
*/
};

}
}

#endif
