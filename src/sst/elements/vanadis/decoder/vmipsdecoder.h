

#ifndef _H_VANADIS_MIPS_DECODER
#define _H_VANADIS_MIPS_DECODER

#include "decoder/vdecoder.h"
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

class VanadisMIPSDecodeBlock {
public:
	VanadisMIPSDecodeBlock( const uint64_t addr, const uint32_t potentialIns ):
		startAddr(addr), ins(potentialIns) {}

	uint64_t getStartAddress() const { return startAddr; }
	uint32_t getInstruction()  const { return ins; }

private:
	const uint64_t startAddr;
	const uint32_t ins;
};

class VanadisMIPSDecoder : public VanadisDecoder {
public:
	SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
		VanadisMIPSDecoder,
		"vanadis",
		"VanadisMIPSDecoder",
		SST_ELI_ELEMENT_VERSION(1,0,0),
		"Implements a MIPS-compatible decoder for Vanadis CPU processing.",
		SST::Vanadis::VanadisDecoder )

	VanadisMIPSDecoder( ComponentId_t id, Params& params ) :
		VanadisDecoder( id, params ) {

		options = new VanadisDecoderOptions( (uint16_t) 0 );
		icache_max_bytes_per_cycle = params.find<uint16_t>("icache_bytes_per_request",  4);
		decode_buffer_max_entries  = params.find<uint16_t>("decode_buffer_max_entries", 4);
		max_decodes_per_cycle      = params.find<uint16_t>("decode_max_ins_per_cycle",  2);

		delegatedLoadWidth = icache_max_bytes_per_cycle;
		delegatedLoadAddr  = 0;

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
		output->verbose(CALL_INFO, 16, 0, "---> Max decodes per cycle: %" PRIu16 ", buffer has %" PRIu32 " pending entries.\n",
			max_decodes_per_cycle, (uint32_t) pendingBlocks.size() );

		uint16_t decodes_performed = 0;

		for( uint16_t i = 0; i < max_decodes_per_cycle; ++i ) {
			// if the decoded queue has space, then lets go ahead and
			// decode the input, put it in the queue for issue.
			if( ! decoded_q->full() ) {
				if( ! pendingBlocks.empty() ) {
					VanadisMIPSDecodeBlock* nextBlock = pendingBlocks.front();
					pendingBlocks.pop_front();

					// Push this off to the decoder to pull the data in
					decode( output, nextBlock->getStartAddress(),
						nextBlock->getInstruction());

					decodes_performed++;

					delete nextBlock;
				}			
			}
		}

		output->verbose(CALL_INFO, 16, 0, "---> Performed %" PRIu16 " decodes this cycle.\n", decodes_performed );
	
		if( pendingBlocks.size() < decode_buffer_max_entries ) {
			output->verbose(CALL_INFO, 16, 0, "---> Requesting a delegated load (addr=%" PRIu64 ", width=%" PRIu16 ")\n",
				delegatedLoadAddr, delegatedLoadWidth);
			wantDelegatedLoad = true;
		} else {
			output->verbose(CALL_INFO, 16, 0, "---> Disabling delegated load this cycle, decode-pending buffers are full (%" PRIu32 " entries in use)\n",
				(uint32_t) pendingBlocks.size());
			wantDelegatedLoad = false;
		}
	}

	virtual void deliverPayload( SST::Output* output, uint8_t* decodePayload, const uint16_t payloadLength ) {
		uint32_t tmp = 0;
		uint8_t* tmp_addr = (uint8_t*) &tmp;

		output->verbose(CALL_INFO, 16, 0, "-> Payload delivery to thr: %" PRIu32 " (addr=%" PRIu64 ", len=%" PRIu16 ")\n",
			hw_thr, delegatedLoadAddr, payloadLength);

		for( uint16_t i = 0; i < payloadLength; i += 4 ) {
			// Copy in memory bytes to integer for processing
			for( uint16_t j = 0; j < 4; ++j ) {
				tmp_addr[j] = decodePayload[i + j];
			}

			pendingBlocks.push_back( new VanadisMIPSDecodeBlock( delegatedLoadAddr + i, tmp ) );
		}

		// Turn off delivery of data so tick phase can decide if we need more
		// but ... crank the address on so we don't get the same data
		wantDelegatedLoad  = false;
		delegatedLoadAddr  += (payloadLength / 4);
		delegatedLoadWidth -= payloadLength;
	}

	virtual void clearDecoderAfterMisspeculate() {
		for( VanadisMIPSDecodeBlock* block : pendingBlocks ) {
			delete block;
		}

		pendingBlocks.clear();

		wantDelegatedLoad  = true;
		delegatedLoadAddr  = ip;

		// We will load the rest of the cache line into the decoder
		delegatedLoadWidth = (iCacheLineWidth) - (ip % iCacheLineWidth);
	}

	
protected:
	void decode( SST::Output* output, const uint64_t ins_addr,
		const uint32_t next_ins ) {

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

	void extract_imm( const uint32_t ins, uint32_t* imm ) const {
		(*imm) = (ins & MIPS_IMM_MASK);
	}

	void extract_three_regs( const uint32_t ins, uint16_t* rt, uint16_t* rs, uint16_t* rd ) const {
		(*rt) = (ins & MIPS_RT_MASK) >> 16;
		(*rs) = (ins & MIPS_RS_MASK) >> 21;
		(*rd) = (ins & MIPS_RD_MASK) >> 11;
	}

	const VanadisDecoderOptions* options;

	uint64_t next_ins_id;

	uint16_t icache_max_bytes_per_cycle;
	uint16_t max_decodes_per_cycle;
	uint16_t decode_buffer_max_entries;
	std::list<VanadisMIPSDecodeBlock*> pendingBlocks;

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
