
#ifndef _H_VANADIS_DECODER
#define _H_VANADIS_DECODER

#include <sst/core/subcomponent.h>
#include <sst/core/interfaces/simpleMem.h>

#include <cstdint>
#include <cinttypes>

#include "datastruct/cqueue.h"
#include "decoder/visaopts.h"
#include "inst/vinst.h"
#include "vbranchunit.h"
#include "vinsloader.h"

namespace SST {
namespace Vanadis {

class VanadisDecoder : public SST::SubComponent {
public:
	SST_ELI_REGISTER_SUBCOMPONENT_API(SST::Vanadis::VanadisDecoder)

	VanadisDecoder( ComponentId_t id, Params& params ) : SubComponent(id) {
		ip = 0;

		const size_t decode_q_len = params.find<size_t>("decode_q_len", 8);
		decoded_q = new VanadisCircularQueue< VanadisInstruction* >( decode_q_len );

		branch_unit = nullptr;

		icache_line_width = params.find<uint64_t>("icache_line_width", 64);

		const size_t uop_cache_size          = params.find<size_t>("uop_cache_entries", 128);
		const size_t predecode_cache_entries = params.find<size_t>("predecode_cache_entries", 32);

		ins_loader = new VanadisInstructionLoader( uop_cache_size, uop_cache_size, predecode_cache_entries );
	}

	virtual ~VanadisDecoder() {
		delete decoded_q;
	}

	void setInsCacheLineWidth( const uint64_t ic_width ) {
		icache_line_width = ic_width;
		ins_loader->setCacheLineWidth( ic_width );
	}

	bool acceptCacheResponse( SST::Interfaces::SimpleMem::Request* req ) {
		return ins_loader->acceptResponse( req );
	}

	uint64_t getInsCacheLineWidth() const { return icache_line_width; }

	virtual const char* getISAName() const = 0;
	virtual uint16_t countISAIntReg() const = 0;
	virtual uint16_t countISAFPReg() const = 0;
	virtual void tick( SST::Output* output, uint64_t cycle ) = 0;
	virtual const VanadisDecoderOptions* getDecoderOptions() const = 0;

	uint64_t getNextInsID() { return next_ins_id++; }

	uint64_t getInstructionPointer() const { return ip; }

	void setInstructionPointer( const uint64_t newIP ) {
			ip = newIP;

			// Do we need to clear here or not?
		}

	void setInstructionPointerAfterMisspeculate( const uint64_t newIP ) {
		ip = newIP;

		// Clear out the decode queue, need to restart
		decoded_q->clear();

		clearDecoderAfterMisspeculate();
	}

	void setBranchUnit( VanadisBranchUnit* new_branch ) { branch_unit = new_branch; }
	VanadisCircularQueue<VanadisInstruction*>* getDecodedQueue() { return decoded_q; }

	void setHardwareThread( const uint32_t thr ) { hw_thr = thr; }
	uint32_t getHardwareThread() const { return hw_thr; }

	VanadisInstructionLoader* getInstructionLoader() {
		return ins_loader;
	}

protected:
	virtual void clearDecoderAfterMisspeculate() = 0;

	uint64_t ip;
	uint64_t next_ins_id;
	uint64_t icache_line_width;
	uint32_t hw_thr;

	bool wantDelegatedLoad;
	VanadisCircularQueue<VanadisInstruction*>* decoded_q;
	VanadisBranchUnit* branch_unit;

	VanadisInstructionLoader* ins_loader;

};

}
}

#endif
