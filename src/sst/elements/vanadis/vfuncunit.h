
#ifndef _H_VANADIS_FUNCTIONAL_UNIT
#define _H_VANADIS_FUNCTIONAL_UNIT

#include <cstdint>
#include <cinttypes>
#include <climits>

#include "inst/vinst.h"
#include "decoder/visaopts.h"

namespace SST {
namespace Vanadis {

class VanadisNoInstruction : public VanadisInstruction {
public:
	VanadisNoInstruction(const VanadisDecoderOptions* opts) :
		VanadisInstruction(UINT64_MAX, UINT64_MAX, UINT32_MAX, opts, 0, 0, 0, 0, 0, 0, 0, 0) {}

	virtual const char* getInstCode() const { return "VOID"; }
	VanadisInstruction* clone() { return new VanadisNoInstruction( *this ); };
	virtual VanadisFunctionalUnitType getInstFuncType() const { return INST_NOOP; }
	virtual void execute( SST::Output* output, VanadisRegisterFile* rFile) {
		markExecuted();
	}

	virtual bool performDeleteAtFuncUnit() const { return true; }
};

class VanadisFunctionalUnit {

public:
	VanadisFunctionalUnit( uint16_t id,
		VanadisFunctionalUnitType unit_type,
		uint16_t latency) : fu_id(id), fu_type(unit_type) {

		fu_queue = new VanadisCircularQueue<VanadisInstruction*>(latency);

		for(uint16_t i = 0; i < latency; ++i) {
			fu_queue->push(new VanadisNoInstruction(isa_options));
		}

		slot_inst = nullptr;
		isa_options = new VanadisDecoderOptions();
	}

	~VanadisFunctionalUnit() {
		delete fu_queue;
	}

	VanadisFunctionalUnitType getType() const {
		return fu_type;
	}

	bool isInstructionSlotFree() const { return slot_inst == nullptr; }

	void setSlotInstruction( VanadisInstruction* ins ) {
		slot_inst = ins;
	}

	uint16_t getUnitID() const {
		return fu_id;
	}

	void tick(const uint64_t cycle, SST::Output* output, std::vector<VanadisRegisterFile*>& regFile) {
		output->verbose(CALL_INFO, 8, 0, "Functional Unit %" PRIu16 " (%s) executing cycle %" PRIu64 "...\n", fu_id, funcTypeToString(fu_type), cycle);
		VanadisInstruction* ins = fu_queue->pop();

		if( UINT32_MAX == ins->getHWThread() ) {
			ins->execute(output, nullptr);
		} else {
			ins->execute(output, regFile[ins->getHWThread()]);
		}

		if( ins->performDeleteAtFuncUnit() ) {
			delete ins;
		}

		if( nullptr == slot_inst ) {
			fu_queue->push( new VanadisNoInstruction(isa_options) );
		} else {
			fu_queue->push(slot_inst);
			slot_inst = nullptr;
		}
	}

	void clearByHWThreadID( SST::Output* output, const uint16_t hw_thr ) {
		output->verbose(CALL_INFO, 16, 0, "-> Function Unit, clearing by hardware thread %" PRIu32 "...\n", hw_thr);

		VanadisCircularQueue<VanadisInstruction*>* tmp_q = new
			VanadisCircularQueue<VanadisInstruction*>( fu_queue->capacity() );

		VanadisInstruction* tmp_ins;

		for( uint16_t i = 0; i < fu_queue->size(); ++i ) {
			tmp_ins = fu_queue->pop();

			if( hw_thr != tmp_ins->getHWThread() ) {
				tmp_q->push( tmp_ins );
			}
		}

		delete fu_queue;
		fu_queue = tmp_q;


	}

private:
	VanadisCircularQueue<VanadisInstruction*>* fu_queue;
	const VanadisDecoderOptions* isa_options;
	VanadisFunctionalUnitType fu_type;
	VanadisInstruction* slot_inst;
	const uint16_t fu_id;
};

}
}

#endif
