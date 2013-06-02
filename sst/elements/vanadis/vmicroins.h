
#ifndef _H_VANADIS_MICRO_OP
#define _H_VANADIS_MICRO_OP

namespace SST {
namespace Vanadis {

enum MicroOpType {
	LOAD,
	STORE,
	INTEGER,
	FLOATING_POINT,
	LOGICAL,
	VECTOR,
	SYSCALL
}

enum MicroOpInsClass {
	R,
	I
}

enum MicroOpInstruction {
	IADD,
	ISUB,
	IMUL,
	IDIV,
	FADD,
	FSUB,
	FMUL,
	FDIV,
	EQ,
	AND,
	OR,
	LT,
	LTE,
	GT,
	GTE,
	SYSCALL
}

class VanadisMicroOp 
{
	private:
		uint64_t macroOpID;
		uint64_t microOpSeq;
		MicroOpInstruction insCode;		

	public:
		VanadisMicroOp();
		
		MicroOpType getMicroOpType();
		MicroOpInstruction getMicroOpIns();
		virtual MicroOpInsClass getMicroOpInsClass();
}

}
}

#endif
