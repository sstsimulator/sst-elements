
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
	R4,
	I,
	B,
	L,
	J
}

enum MicroOpInstruction {
	LB,
	LH,
	LW,
	LD,
	LBU,
	LHU,
	LWU,
	SB,
	SH,
	SW,
	SD,
	IADD,
	ISUB,
	IMUL,
	IDIV,
	FADD,
	FSUB,
	FMUL,
	FDIV,
	AND,
	OR,
	BLT,
	BGTE,
	BLTU,
	BGTEU,
	BEQ,
	BNE,
	JUMP,
	JUMPAL,
	JUMPALRC,
	JUMPALRR,
	JUMPALRJ,
	RDNPC,
	SYSCALL,
	DECODEERROR
}

class VanadisMicroOp 
{
	private:
		uint64_t macroOpID;
		uint64_t microOpSeq;
		uint32_t ins;

	public:
		VanadisMicroOp(uint32_t ins);

		uint32_t getInstruction();		
		MicroOpType getMicroOpType();
		MicroOpInstruction getMicroOpIns();
		virtual MicroOpInsClass getMicroOpInsClass();

}

}
}

#endif
