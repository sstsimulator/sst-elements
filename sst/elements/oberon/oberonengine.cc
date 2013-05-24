
#include "oberonengine.h"

OberonEngine::OberonEngine(uint32_t stackAllocation,
			uint32_t initialPC,
			void* binary) :
	exprStack() {

	engineHalted = false;
	programBinary = binary;
	stack = malloc(stackAllocation);
	pc = initialPC;
	stackPtr = 0;
}

bool OberonEngine::isHalted() {
	return engineHalted;
}

void processHaltInstruction() {
	engineHalted = true;
}

void processAddInstruction() {
	OberonExpressionStackValue op_one = exprStack.pop();
	OberonExpressionStackValue op_two = exprStack.pop();
	
	OberonExpressionStackValue result = op_one.add(op_two);
	exprStack.push(result);
}

void processSubInstruction() {
	OberonExpressionStackValue op_one = exprStack.pop();
	OberonExpressionStackValue op_two = exprStack.pop();
	
	OberonExpressionStackValue result = op_one.sub(op_two);
	exprStack.push(result);
}

void processMulInstruction() {
	OberonExpressionStackValue op_one = exprStack.pop();
	OberonExpressionStackValue op_two = exprStack.pop();
	
	OberonExpressionStackValue result = op_one.mul(op_two);
	exprStack.push(result);
}

void prcessDivInstruction() {
	OberonExpressionStackValue op_one = exprStack.pop();
	OberonExpressionStackValue op_two = exprStack.pop();
	
	OberonExpressionStackValue result = op_one.div(op_two);
	exprStack.push(result);
}

void processPopInstruction(uint32_t target_var_offset) {
	OberonExpressionStackValue value = exprStack.pop();
	
	switch(value.getExpressionType()) {
		case DOUBLE:
			double real_value = val.getDoubleValue();
			memcpy(stack + target_var_offset, &real_value, sizeof(real_value));
			break;
			
		case INTEGER:
			int64_t real_value = val.getIntegerValue();
			memcpy(stack + target_var_offset, &real_value, sizeof(real_value));
			break;
			
		case BOOLEAN:
			bool real_value = val.getBooleanValue();
			memcpy(stack + target_var_offset, &real_value, sizeof(real_value));
			break;
			
		case STRING:
			std::cerr << "Cannot POP a variable to a location if the expression is a STRING" << std::endl;
			abort();
			break;
	}
}

void processPushIntegerInstruction(uint32_t target_var_offset) {
	int64_t the_value;
	memcpy(&the_value, stack + target_var_offset, sizeof(the_value);
	
	OberonIntegerStackValue ob_value(the_value);
	stack.push(ob_value);	
}

void processBuildInCallInstruction(uint32_t func) {
	switch(func) {
		case OBERON_BUILTIN_PRINT:
			OberonExpressionStackValue printMe = exprStack.pop();
			printMe.print();
			break;
			
		default:
			std::cerr << "Unknown call to build in function: " << func << std::endl;
			abort();
			break;
	}
}

OberonEvent generateNextEvent() {
	bool continueProcessing = true;
	uint32_t ins;
	const size_t ins_width = sizeof(uint32_t);

	while(continueProcessing) {
		// Copy the next instruction from memory
		memcpy(&ins, pc, ins_width);
		
		// Increment the program counter
		pc += ins_width;
		
		switch(ins) {
		
			case OBERON_INS_ADD:
				processAddInstruction();
				break;
				
			case OBERON_INS_SUB:
				processSubInstruction();
				break;
				
			case OBERON_INS_POP:
				uint32_t target_var_offset;
				memcpy(&target_var_offset, pc, ins_width);
				pc += ins_width;
				
				processPopInstruction(target_var_offset);
				break;
				
			case OBERON_INS_PUSH_INT:
				uint32_t target_var_offset;
				memcpy(&target_var_offset, pc, ins_width);
				pc += ins_width;
			
				processPushIntegerInstruction();
				break;
				
			case OBERON_INS_PUSH_DBL:
				uint32_t target_var_offset;
				memcpy(&target_var_offset, pc, ins_width);
				pc += ins_width;
			
				processPushDoubleInstruction();
				break;
				
			case OBERON_INS_PUSH_BOOL:
				uint32_t target_var_offset;
				memcpy(&target_var_offset, pc, ins_width);
				pc += ins_width;
			
				processPushBoolInstruction();
				break;
				
			case OBERON_INS_MUL:
				processMulInstruction();
				break;
				
			case OBERON_INS_DIV:
				processDivInstruction();
				break;
				
			case OBERON_BUILTIN_PRINT:
				// Get the built in function that is going to be called, increment
				// the PC so we're in step and then hand to be processed
				uint32_t func_type;
				memcpy(&func_type, pc, ins_width);
				pc += ins_width;
				processBuildInCallInstruction(func_type);
				break;
				
			case OBERON_INS_HALT:
				processHaltInstruction();
				continueProcessing = false;
				break;
		
		}
	}
}

