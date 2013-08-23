
#include "oberonengine.h"

using namespace SST::Oberon;

OberonEngine::OberonEngine(OberonModel* m, Output* out) {
	pc = 0;
	model = m;
	isHalted = false;

	output = out;
	exprStack = new OberonExpressionStack();
}

OberonEvent* OberonEngine::generateNextEvent() {
	uint32_t incPC = 0;
	bool continueProcessing = true;

	while(continueProcessing) {
		const uint32_t thisInst = model->getInstructionAt(pc);

		output->verbose(CALL_INFO, 1, 0, "Read instruction at: %" PRIu32 ", instruction is: %" PRIu32 "\n", pc, thisInst);

		switch(thisInst) {

		case OBERON_INST_PUSH_I64:
			incPC = 8;
			processPushI64(pc + 4);
			break;

		case OBERON_INST_PUSH_FP64:
			incPC = 8;
			processPushFP64(pc + 4);
			break;

		case OBERON_INST_PUSH_LIT_U32:
			incPC = 8;
			processPushUI32Literal(pc + 4);
			break;

		case OBERON_INST_PUSH_LIT_I64:
			incPC = 12;
			processPushI64Literal(pc + 4);
			break;

		case OBERON_INST_PUSH_LIT_FP64:
			incPC = 12;
			processPushFP64Literal(pc + 4);
			break;

		case OBERON_INST_POP_I64:
			incPC = 8;
			processPopI64(pc + 4);
			break;

		case OBERON_INST_POP_FP64:
			incPC = 8;
			processPopFP64(pc + 4);
			break;

		case OBERON_INST_IADD:
                	incPC = 4;
			processI64Add();
			break;

		case OBERON_INST_ISUB:
                	incPC = 4;
			processI64Sub();
			break;

		case OBERON_INST_IMUL:
                	incPC = 4;
			processI64Mul();
			break;

		case OBERON_INST_IDIV:
                	incPC = 4;
			processI64Div();
			break;

		case OBERON_INST_IMOD:
                	incPC = 4;
			processI64Mod();
			break;

		case OBERON_INST_JUMPREL:
			incPC = 0;
			pc = processUnconditionalJumpRelative(pc + 4);
			break;

		case OBERON_INST_JUMP:
			incPC = 0;
			pc = processUnconditionalJump(pc + 4);
			break;

		case OBERON_INST_NOP:
			incPC = 4;
			break;

		case OBERON_INST_PRINT_I64:
			incPC = 4;
			processPrintI64();
			break;

		case OBERON_INST_HALT:
			incPC = 4;
			continueProcessing = false;
			processHalt();

			break;

		case OBERON_INST_DUMP_MEM:
			model->dumpMemory();
			break;

		}

		output->verbose(CALL_INFO, 1, 0, "Incrementing instruction program counter from: %" PRIu32 " by: %" PRIu32 " = %" PRIu32 "\n", pc, incPC, (pc + incPC));

		pc += incPC;
	}

	return NULL;
}

void OberonEngine::processPrintI64() {
	output->verbose(CALL_INFO, 1, 0, "Print I64\n");

	OberonExpressionValue* v = exprStack->pop();

	printf("OBERON: %" PRIu64 "\n", v->getAsInt64());

	delete v;
}

uint32_t OberonEngine::processUnconditionalJump(uint32_t pc) {
	uint32_t jumpTo = model->getUInt32At(pc);
	output->verbose(CALL_INFO, 1, 0, "Unconditional Jump to %" PRIu32 "\n", jumpTo);
	return jumpTo;
}

uint32_t OberonEngine::processUnconditionalJumpRelative(uint32_t pc) {
	uint32_t jumpTo = pc + model->getUInt32At(pc);
	output->verbose(CALL_INFO, 1, 0, "Unconditional Relative Jump to %" PRIu32 "\n", jumpTo);
	return jumpTo;
}

void OberonEngine::processHalt() {
	isHalted = true;
}

void OberonEngine::processPopFP64(uint32_t pc) {
	uint32_t indexToStoreAt = model->getUInt32At(pc);

	OberonExpressionValue* v = exprStack->pop();
	model->setFP64At(indexToStoreAt, v->getAsFP64());

	output->verbose(CALL_INFO, 1, 0, "Pop FP64 %f to index %" PRIu32 "\n", v->getAsFP64(), indexToStoreAt);

	delete v;
}

void OberonEngine::processPopI64(uint32_t pc) {
	uint32_t indexToStoreAt = model->getUInt32At(pc);

	OberonExpressionValue* v = exprStack->pop();
	model->setInt64At(indexToStoreAt, v->getAsInt64());

	output->verbose(CALL_INFO, 1, 0, "Pop Int64 %" PRIu64 " to index %" PRIu32 "\n", v->getAsInt64(), indexToStoreAt);

	delete v;
}

void OberonEngine::processPushI64Literal(uint32_t pc) {
	int64_t value = model->getInt64At(pc);

	output->verbose(CALL_INFO, 1, 0, "Push Literal Int64 %" PRIu64 " (from PC+4: %" PRIu32 ")\n", value, pc);

	exprStack->push(value);
}

void OberonEngine::processPushFP64Literal(uint32_t pc) {
	double value = model->getFP64At(pc);

	output->verbose(CALL_INFO, 1, 0, "Push Literal FP64 %f (from PC+4: %" PRIu32 ")\n", value, pc);

	exprStack->push(value);
}

void OberonEngine::processPushUI32Literal(uint32_t pc) {
	uint32_t value = model->getUInt32At(pc);

	output->verbose(CALL_INFO, 1, 0, "Push Literal Int32 %" PRIu32 " (from PC+4: %" PRIu32 ")\n", value, pc);

	exprStack->push(value);
}

void OberonEngine::processPushFP64(uint32_t pc) {
	uint32_t index = model->getUInt32At(pc);
	double value = model->getFP64At(index);

	output->verbose(CALL_INFO, 1, 0, "Push FP64 Value=%f (from index: %" PRIu32 " at pc %" PRIu32 ")\n", value, index, pc);

	exprStack->push(value);
}

void OberonEngine::processPushI64(uint32_t pc) {
	uint32_t index = model->getUInt32At(pc);
	int64_t value = model->getInt64At(index);

	output->verbose(CALL_INFO, 1, 0, "Push Int64 Value=%" PRIu64 " (from index: %" PRIu32 " at pc %" PRIu32 ")\n", value, index, pc);

	exprStack->push(value);
}

void OberonEngine::processI64Add() {
	OberonExpressionValue* r = exprStack->pop();
	OberonExpressionValue* l = exprStack->pop();
	int64_t result = l->getAsInt64() + r->getAsInt64();

	exprStack->push(result);

	delete r;
	delete l;
}

void OberonEngine::processI64Sub() {
	OberonExpressionValue* r = exprStack->pop();
	OberonExpressionValue* l = exprStack->pop();
	int64_t result = l->getAsInt64() - r->getAsInt64();

	exprStack->push(result);

	delete r;
	delete l;
}

void OberonEngine::processI64Mul() {
	OberonExpressionValue* r = exprStack->pop();
	OberonExpressionValue* l = exprStack->pop();
	int64_t result = l->getAsInt64() * r->getAsInt64();

	exprStack->push(result);

	delete r;
	delete l;
}

void OberonEngine::processI64Div() {
	OberonExpressionValue* r = exprStack->pop();
	OberonExpressionValue* l = exprStack->pop();
	int64_t result = l->getAsInt64() / r->getAsInt64();

	exprStack->push(result);

	delete r;
	delete l;
}

void OberonEngine::processI64Mod() {
	OberonExpressionValue* r = exprStack->pop();
	OberonExpressionValue* l = exprStack->pop();
	int64_t result = l->getAsInt64() % r->getAsInt64();

	exprStack->push(result);

	delete r;
	delete l;
}

bool OberonEngine::instanceHalted() {
	return isHalted;
}
