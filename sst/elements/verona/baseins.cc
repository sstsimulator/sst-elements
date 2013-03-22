#include "baseins.h"

Instruction::Instruction(uint32_t ins) {
	instruction = ins;
}

int Instruction::getOpCode() {
	return instruction & OP_CODE_MASK;	
}
