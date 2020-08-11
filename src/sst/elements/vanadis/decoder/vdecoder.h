
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

		iCacheLineWidth = params.find<uint64_t>("icache_line_width", 0);
	}

	virtual ~VanadisDecoder() {
		delete decoded_q;
	}

	void setInsCacheLineWidth( const uint64_t ic_width ) { iCacheLineWidth = ic_width; }
	uint64_t getInsCacheLineWidth() const { return iCacheLineWidth; }

	bool requestingDelegatedRead() const {
		return wantDelegatedLoad;
	}

	uint64_t getDelegatedLoadAddr() const {
		return delegatedLoadAddr;
	}

	uint16_t getDelegatedLoadWidth() const {
		return delegatedLoadWidth;
	}

	virtual void deliverPayload( SST::Output* output, uint8_t* decodePayload,
		const uint16_t payloadLen ) {
		
	}

	void clearDelegatedLoadRequest() {
		wantDelegatedLoad = false;
	}

	virtual const char* getISAName() const = 0;
	virtual uint16_t countISAIntReg() const = 0;
	virtual uint16_t countISAFPReg() const = 0;
	virtual void tick( SST::Output* output, uint64_t cycle ) = 0;
	virtual const VanadisDecoderOptions* getDecoderOptions() const = 0;
	
	uint64_t getNextInsID() { return next_ins_id; }

	uint64_t getInstructionPointer() const { return ip; }
	void setInstructionPointer( const uint64_t newIP ) {
			ip = newIP;

			// Do we need to clear here or not?
		}

	void setInstructionPointerAfterMisspeculate( const uint64_t newIP ) {
		ip = newIP;

		// Clear out the decode queue, need to restart
		decoded_q->clear();

		// False, next cycle decoder will decide what we need to pull from
		// the i-cache/memory subsystem
		wantDelegatedLoad  = false;
		delegatedLoadAddr  = 0;
		delegatedLoadWidth = 0;

		clearDecoderAfterMisspeculate();
	}
	void setBranchUnit( VanadisBranchUnit* new_branch ) { branch_unit = new_branch; }
	VanadisCircularQueue<VanadisInstruction*>* getDecodedQueue() { return decoded_q; }

	void setHardwareThread( const uint32_t thr ) { hw_thr = thr; }
	uint32_t getHardwareThread() const { return hw_thr; }

protected:
	virtual void clearDecoderAfterMisspeculate() = 0;

	uint64_t ip;
	uint64_t delegatedLoadAddr;
	uint16_t delegatedLoadWidth;
	uint64_t next_ins_id;
	uint64_t iCacheLineWidth;
	uint32_t hw_thr;

	bool wantDelegatedLoad;
	VanadisCircularQueue<VanadisInstruction*>* decoded_q;
	VanadisBranchUnit* branch_unit;


};

}
}

#endif
