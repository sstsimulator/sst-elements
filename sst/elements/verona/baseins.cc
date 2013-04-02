#include "baseins.h"

using namespace SST::Verona;

Instruction::Instruction(uint32_t ins) {
	instruction = ins;
}

int Instruction::getOpCode() {
	return instruction & OP_CODE_MASK;	
}
