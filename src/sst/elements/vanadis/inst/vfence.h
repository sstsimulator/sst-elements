
#ifndef _H_VANADIS_FENCE_INST
#define _H_VANADIS_FENCE_INST

#include "inst/vinst.h"

namespace SST {
namespace Vanadis {

enum VanadisFenceType {
	VANADIS_LOAD_FENCE,
	VANADIS_STORE_FENCE,
	VANADIS_LOAD_STORE_FENCE
};

class VanadisFenceInstruction : public VanadisInstruction {
public:
	VanadisFenceInstruction(
		const uint64_t ins_id,
		const uint64_t address,
		const uint32_t hw_thr,
		const VanadisDecoderOptions* isa_opts,
		const VanadisFenceType fenceT
		) : VanadisInstruction( ins_id, address, hw_thr, isa_opts, 0, 0, 0, 0, 0, 0, 0, 0 ) {
		fence = fenceT;
	}

	bool createsLoadFence() const {
		return (fence == VANADIS_LOAD_FENCE) || (fence == VANADIS_LOAD_STORE_FENCE);
	}

	bool createsStoreFence() const {
		return (fence == VANADIS_STORE_FENCE) || (fence == VANADIS_LOAD_STORE_FENCE);
	}

	virtual const char* getInstCode() const {
		return  (fence == VANADIS_LOAD_STORE_FENCE) ? "SYNC" :
                        (fence == VANADIS_LOAD_FENCE) ? "LFENCE" : "SFENCE" ;
	}

	virtual void printToBuffer(char* buffer, size_t buffer_size) {
		snprintf(buffer, buffer_size, "%s", getInstCode() );
	}

	virtual VanadisFunctionalUnitType getInstFuncType() const {
		return INST_FENCE;
	}

	virtual void execute( SST::Output* output, VanadisRegisterFile* regFile ) {}

	virtual void print( SST::Output* output ) {
		output->verbose(CALL_INFO, 8, 0, "%s", getInstCode());
	}

protected:
	VanadisFenceType fence;

};

}
}

#endif
