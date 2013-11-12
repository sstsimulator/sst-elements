
#ifndef _H_SST_VANADIS_MICRO_OP
#define _H_SST_VANADIS_MICRO_OP

namespace SST {
namespace Vanadis {

enum MicroOpType {
	INT_S_ALU,
	FP_S_ALU,
	BRANCH,
	NO_OP,
	NOT_DECODED
};

class VanadisMicroOp {

	protected:
		uint64_t pc;
		uint32_t seq;

	public:
		VanadisMicroOp(uint64_t pc, uint32_t seq);
		uint64_t getProgramCounter();
		uint32_t getSequenceNumber();

		virtual MicroOpType getMicroOpType() = 0;

};

}
}

#endif
