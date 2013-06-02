
#ifndef _H_VANADIS_MICRO_OP
#define _H_VANADIS_MICRO_OP

namespace SST {
namespace Vanadis {

enum MicroOpType {
	CONTROL,
	LOAD,
	STORE,
	ATOMIC,
	INTEGER,
	INTEGER32,
	FPLOAD,
	FPSTORE,
	FPCOMPUTE,
	FP2FP,
	INT2FP,
	FP2INT,
	FPCOMPARE,
	FENCE,
	VECTOR,
	SYSTEM
}

#define VANADIS_INSTRUCTION_DECODE_MASK 127
#define VANADIS_CONTROL_INS_MASK 103
#define VANADIS_MEMORY_INS_LOAD_MASK 3
#define VANADIS_MEMORY_INS_STORE_MASK 35
#define VANADIS_ATOMIC_INS_MASK 43
#define VANADIS_INTEGER_IMMEDIATE_MASK 19
#define VANADIS_INTEGER_COMPUTE_MASK 51
#define VANADIS_INTEGER32_IMMEDIATE_MASK 27
#define VANADIS_INTEGER32_COMPUTE_MASK 59
#define VANADIS_FLOAT_MEMORY_OP_MASK 95
#define VANADIS_FLOAT_LOAD_MASK 7
#define VANADIS_FLOAT_STORE_MASK 39
#define VANADIS_FLOAT_COMPUTE_MASK 83
#define VANADIS_FLOAT_FUSED_COMPUTE_MASK 115
#define VANADIS_FLOAT_FUSED_COMPUTE 67 
#define VANADIS_CONVERT_MASK 83
#define VANADIS_FP_COMPARE_MASK 102271
#define VANADIS_FP_COMPARE_VALUE 65619

#define VANADIS_FENCE_MASK 47
#define VANADIS_SYSTEM_MASK 119

MicroOpType decodeToInstructionGroup(uint32_t ins) {
	uint32_t category = (uint32_t) (ins & (uint32_t) VANADIS_INSTRUCTION_DECODE_MASK;

	if((category & ((uint32_t) VANADIS_CONTROL_INS_MASK)) == 
		(uint32_t) VANADIS_CONTROL_INS_MASK) {
		return CONTROL;
	} else if(category == VANADIS_MEMORY_INS_LOAD_MASK) {
		return LOAD;
	} else if(category == VANADIS_MEMORY_INS_STORE_MASK) {
		return STORE;
	} else if(category == VANADIS_ATOMIC_INS_MASK) {
		return ATOMIC;
	} else if(category == VANADIS_INTEGER_IMMEDIATE_MASK) {
		return INTEGER; // does this need to be IMMEDIATE?
	} else if(category == VANADIS_INTEGER_COMPUTE_MASK) {
		return INTEGER;
	} else if(category == VANADIS_INTEGER32_IMMEDIATE_MASK) {
		return INTEGER32; // does this need to be IMMEDIATE?
	} else if(category == VANADIS_INTEGER32_COMPUTE_MASK) {
		return INTEGER32;
	} else if((category & ((uint32_t) VANADIS_FLOAT_MEMORY_OP_MASK)) ==
		VANADIS_FLOAT_LOAD_MASK) {
		return FPLOAD;
	} else if((category & ((uint32_t) VANADIS_FLOAT_MEMORY_OP_MASK)) ==
		VANADIS_FLOAT_STORE_MASK) {
		return FPSTORE;
	// note this compare needs to be at full instruction length as the
	// decode path exceeds the least-seven significant bits
	} else if((ins & ((uint32_t) VANADIS_FP_COMPARE_MASK)) ==
		VANADIS_FP_COMPARE_VALUE) {
		return FPCOMPARE;
	} else if((category == VANADIS_FLOAT_COMPUTE_MASK) {
		return FPCOMPUTE;
	} else if((category & ((uint32_t) VANADIS_FLOAT_FUSED_COMPUTE_MASK) ==
		VANADIS_FLOAT_FUSED_COMPUTE) {
		return FPFUSEDCOMPUTE;
	} else if(category == VANADIS_CONVERT_MASK) {
		return CONVERT;
	} else if(category == VANADIS_FENCE_MASK) {
		return FENCE;
	} else if(category == VANADIS_SYSTEM_MASK) {
		return SYSTEM;
	}
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
