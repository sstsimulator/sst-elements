

#ifndef _H_VANADIS_MIPS_DECODER
#define _H_VANADIS_MIPS_DECODER

#include "decoder/vdecoder.h"
#include "vinsloader.h"
#include "inst/vinstall.h"

#include <list>

#define MIPS_OP_MASK         0xFC000000
#define MIPS_RS_MASK         0x3E00000
#define MIPS_RT_MASK         0x1F0000
#define MIPS_RD_MASK         0xF800
#define MIPS_ADDR_MASK       0x7FFFFFF
#define MIPS_IMM_MASK        0xFFFF
#define MIPS_SHFT_MASK       0x7C0
#define MIPS_FUNC_MASK       0x3F
#define MIPS_SPECIAL_OP_MASK 0x7FF

#define MIPS_SPEC_OP_MASK_ADD     0x20
#define MIPS_SPEC_OP_MASK_ADDU    0x21
#define MIPS_SPEC_OP_MASK_AND     0x24
#define MIPS_SPEC_OP_MASK_BREAK   0x0D
#define MIPS_SPEC_OP_MASK_DADD    0x2C
#define MIPS_SPEC_OP_MASK_DADDU   0x2D
#define MIPS_SPEC_OP_MASK_DIV     0x1A
#define MIPS_SPEC_OP_MASK_DIVU    0x1B
#define MIPS_SPEC_OP_MASK_DDIV    0x1E
#define MIPS_SPEC_OP_MASK_DDIVU   0x1F
#define MIPS_SPEC_OP_MASK_DMULT	  0x1C
#define MIPS_SPEC_OP_MASK_DMULTU  0x1D
#define MIPS_SPEC_OP_MASK_DSLL    0x38
#define MIPS_SPEC_OP_MASK_DSLL32  0x3C
#define MIPS_SPEC_OP_MASK_DSLLV   0x14
#define MIPS_SPEC_OP_MASK_DSRA    0x3B
#define MIPS_SPEC_OP_MASK_DSRA32  0x3F
#define MIPS_SPEC_OP_MASK_DSRAV   0x17
#define MIPS_SPEC_OP_MASK_DSRL    0x3A
#define MIPS_SPEC_OP_MASK_DSRL32  0x3E
#define MIPS_SPEC_OP_MASK_DSRLV   0x16
#define MIPS_SPEC_OP_MASK_DSUB    0x2E
#define MIPS_SPEC_OP_MASK_DSUBU   0x2F
#define MIPS_SPEC_OP_MASK_JALR    0x09
#define MIPS_SPEC_OP_MASK_JR      0x08
#define MIPS_SPEC_OP_MASK_MFHI    0x10
#define MIPS_SPEC_OP_MASK_MFLO    0x12
#define MIPS_SPEC_OP_MASK_MOVN    0x0B
#define MIPS_SPEC_OP_MASK_MOVZ    0x0A
#define MIPS_SPEC_OP_MASK_MTHI    0x11
#define MIPS_SPEC_OP_MASK_MTLO    0x13
#define MIPS_SPEC_OP_MASK_MULT    0x18
#define MIPS_SPEC_OP_MASK_MULTU   0x19
#define MIPS_SPEC_OP_MASK_NOR     0x27
#define MIPS_SPEC_OP_MASK_OR      0x25
#define MIPS_SPEC_OP_MASK_SLL     0x00
#define MIPS_SPEC_OP_MASK_SLLV    0x04
#define MIPS_SPEC_OP_MASK_SLT	  0x2A
#define MIPS_SPEC_OP_MASK_SLTU    0x2B
#define MIPS_SPEC_OP_MASK_SRA	  0x03
#define MIPS_SPEC_OP_MASK_SRAV    0x07
#define MIPS_SPEC_OP_MASK_SRL	  0x02
#define MIPS_SPEC_OP_MASK_SRLV    0x06
#define MIPS_SPEC_OP_MASK_SUB	  0x22
#define MIPS_SPEC_OP_MASK_SUBU    0x23
#define MIPS_SPEC_OP_MASK_SYSCALL 0x0C
#define MIPS_SPEC_OP_MASK_XOR     0x26


#define MIPS_ADDI_OP_MASK    0x30000000

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

		next_ins_id = 0;
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
							output->verbose(CALL_INFO, 16, 0, "---> --> issuing ins id: %" PRIu64 " (addr: %llx, %s)...\n",
								next_ins->getID(),
								next_ins->getInstructionAddress(),
								next_ins->getInstCode());
							decoded_q->push( next_ins );
						}

						delete bundle;
						uop_bundles_used++;
					} else {
						output->verbose(CALL_INFO, 16, 0, "---> --> micro-op bundle for %p contains %" PRIu32 " ops, we only have %" PRIu32 " slots available in the decode q, wait for resources to become available.\n",
							(void*) ip, (uint32_t) bundle->getInstructionCount(),
							(uint32_t) (decoded_q->capacity() - decoded_q->size()));
						// We don't have enough space, so we have to stop and wait for more entries to free up.
						break;
					}
				} else if( ins_loader->hasPredecodeAt( ip ) ) {
					// We do have a locally cached copy of the data at the IP though, so decode into a bundle
					output->verbose(CALL_INFO, 16, 0, "---> uop not found, but matched in predecoded L0-icache (ip=%p)\n", (void*) ip);

					uint32_t temp_ins = 0;
					VanadisInstructionBundle* decoded_bundle = new VanadisInstructionBundle( ip );

					if( ins_loader->getPredecodeBytes( output, ip, (uint8_t*) &temp_ins, sizeof(temp_ins) ) ) {
						output->verbose(CALL_INFO, 16, 0, "---> performing a decode of the bytes found (ins-bytes: 0x%x)\n", temp_ins);
						decode( output, ip, temp_ins, decoded_bundle );

						output->verbose(CALL_INFO, 16, 0, "---> performing a decode of the bytes found (generates %" PRIu32 " micro-op bundle).\n",
							(uint32_t) decoded_bundle->getInstructionCount());
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
		output->verbose( CALL_INFO, 16, 0, "[decode] > addr: 0x%llx ins: 0x%08x\n", ins_addr, next_ins );

		const uint32_t hw_thr    = getHardwareThread();
		const uint32_t ins_mask  = next_ins & MIPS_OP_MASK;
		const uint32_t func_mask = next_ins & MIPS_FUNC_MASK;

		output->verbose( CALL_INFO, 16, 0, "[decode] ---> ins-mask: 0x%08x / 0x%08x\n", ins_mask, func_mask );

		uint16_t rt = 0;
		uint16_t rs = 0;
		uint16_t rd = 0;

		uint32_t imm = 0;

		// Perform a register and parameter extract in case we need later
		extract_three_regs( next_ins, &rt, &rs, &rd );
		extract_imm( next_ins, &imm );

		output->verbose( CALL_INFO, 16, 0, "[decode] rt=%" PRIu32 ", rs=%" PRIu32 ", rd=%" PRIu32 "\n",
			rt, rs, rd);

		const uint64_t imm64 = (uint64_t) imm;

		bool insertDecodeFault = true;

		switch( ins_mask ) {
		case 0:
			{
				// The SHIFT 5 bits must be zero for these operations according to the manual
				if( 0 == (next_ins & MIPS_SHFT_MASK ) ) {
					output->verbose( CALL_INFO, 16, 0, "[decode] -> special-class, func-mask: 0x%x\n", func_mask);

					switch( func_mask ) {
					case MIPS_SPEC_OP_MASK_ADD:
						bundle->addInstruction( new VanadisAddInstruction( getNextInsID(), ins_addr, hw_thr, options, rd, rs, rt ) );
						insertDecodeFault = false;
						break;

					case MIPS_SPEC_OP_MASK_ADDU:
						break;

					case MIPS_SPEC_OP_MASK_AND:
						bundle->addInstruction( new VanadisAndInstruction( getNextInsID(), ins_addr, hw_thr, options, rd, rs, rt ) );
						insertDecodeFault = false;
						break;

					// _BREAK NEEDS TO GO HERE?

					case MIPS_SPEC_OP_MASK_DADD:
						break;

					case MIPS_SPEC_OP_MASK_DADDU:
						break;

					case MIPS_SPEC_OP_MASK_DDIV:
						break;

					case MIPS_SPEC_OP_MASK_DDIVU:
						break;

					case MIPS_SPEC_OP_MASK_DIV:
						break;

					case MIPS_SPEC_OP_MASK_DIVU:
						break;

					case MIPS_SPEC_OP_MASK_DMULT:
						break;

					case MIPS_SPEC_OP_MASK_DMULTU:
						break;

					case MIPS_SPEC_OP_MASK_DSLLV:
						break;

					case MIPS_SPEC_OP_MASK_DSRAV:
						break;

					case MIPS_SPEC_OP_MASK_DSRLV:
						break;

					case MIPS_SPEC_OP_MASK_DSUB:
						break;

					case MIPS_SPEC_OP_MASK_DSUBU:
						break;

					case MIPS_SPEC_OP_MASK_JALR:
						break;

					case MIPS_SPEC_OP_MASK_JR:
						break;

					case MIPS_SPEC_OP_MASK_MFHI:
						break;

					case MIPS_SPEC_OP_MASK_MFLO:
						break;

					case MIPS_SPEC_OP_MASK_MOVN:
						break;

					case MIPS_SPEC_OP_MASK_MOVZ:
						break;

					case MIPS_SPEC_OP_MASK_MTHI:
						break;

					case MIPS_SPEC_OP_MASK_MTLO:
						break;

					case MIPS_SPEC_OP_MASK_MULT:
						bundle->addInstruction( new VanadisMultiplyInstruction( getNextInsID(), ins_addr, hw_thr, options, rd, rs, rt ) );
						insertDecodeFault = false;
						break;

					case MIPS_SPEC_OP_MASK_MULTU:
						break;

					case MIPS_SPEC_OP_MASK_NOR:
						break;

					case MIPS_SPEC_OP_MASK_OR:
						bundle->addInstruction( new VanadisOrInstruction( getNextInsID(), ins_addr, hw_thr, options, rd, rs, rt ) );
						insertDecodeFault = false;
						break;
						break;

					case MIPS_SPEC_OP_MASK_SLLV:
						break;

					case MIPS_SPEC_OP_MASK_SLT:
						break;

					case MIPS_SPEC_OP_MASK_SLTU:
						break;

					case MIPS_SPEC_OP_MASK_SRA:
						break;

					case MIPS_SPEC_OP_MASK_SRAV:
						break;

					case MIPS_SPEC_OP_MASK_SRLV:
						break;

					case MIPS_SPEC_OP_MASK_SUB:
						bundle->addInstruction( new VanadisSubInstruction( getNextInsID(), ins_addr, hw_thr, options, rd, rs, rt ) );
						insertDecodeFault = false;
						break;

					case MIPS_SPEC_OP_MASK_SUBU:
						break;

					case MIPS_SPEC_OP_MASK_SYSCALL:
						break;

					case MIPS_SPEC_OP_MASK_XOR:
						bundle->addInstruction( new VanadisXorInstruction( getNextInsID(), ins_addr, hw_thr, options, rd, rs, rt ) );
						insertDecodeFault = false;
						break;
					}
				}
			}
			break;

		case MIPS_ADDI_OP_MASK:
			//decoded_q->push( new VanadisAddImmInstruction( getNextInsID(),
			//	ins_addr, getHardwareThread(), options, rt, rs, imm64 ) );
			insertDecodeFault = false;
			break;

		default:
			break;
		}

		if( insertDecodeFault ) {
			bundle->addInstruction( new VanadisInstructionDecodeFault( getNextInsID(),
				ins_addr, getHardwareThread(), options) );
		}

		for( uint32_t i = 0; i < bundle->getInstructionCount(); ++i ) {
			output->verbose(CALL_INFO, 16, 0, "-> [%3" PRIu32 "]: %s\n", i, bundle->getInstructionByIndex(i)->getInstCode());
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
