
#include "oberonengine.h"

using namespace SST::Oberon;

OberonEngine::OberonEngine(string dumpPrefix, OberonModel* m) {
	pc = 0;
	model = m;
	isHalted = false;
	memoryDumpNumber = 0;
	memoryDumpPrefix = dumpPrefix;

	exprStack = new OberonExpressionStack();
}

OberonEngine::OberonEngine(OberonModel* m) {
	pc = 0;
	model = m;
	isHalted = false;
	memoryDumpNumber = 0;
	memoryDumpPrefix = "oberon";

	exprStack = new OberonExpressionStack();
}

OberonEvent* OberonEngine::generateNextEvent() {
	uint32_t incPC = 0;
	bool continueProcessing = true;

	while(continueProcessing) {
		const uint32_t thisInst = model->getInstructionAt(pc);

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

		case OBERON_INST_NOP:
			incPC = 4;
			break;

		case OBERON_INST_HALT:
			incPC = 4;
			continueProcessing = false;
			processHalt();

			break;

		case OBERON_INST_DUMP_MEM:
			char* dumpName = (char*) malloc(sizeof(char) * 1024);
			sprintf(dumpName, "%s-%d.dump", memoryDumpPrefix.c_str(),
				(memoryDumpNumber++));

			FILE* currentDump = fopen(dumpName, "wt");

			for(uint32_t i = 0; i < model->getMemorySize(); i += 8) {
				fprintf(currentDump, "%20" PRIu32 " | %f | %20" PRIu64 "\n",
					i, model->getFP64At(i), model->getInt64At(i));
			}

			fclose(currentDump);
			break;

		}

		pc += incPC;
	}

}

void OberonEngine::processHalt() {
	isHalted = true;
}

void OberonEngine::processPopFP64(uint32_t pc) {
	uint32_t indexToStoreAt = model->getUInt32At(pc);

	OberonExpressionValue* v = exprStack->pop();
	model->setFP64At(v->getAsFP64(), indexToStoreAt);

	delete v;
}

void OberonEngine::processPopI64(uint32_t pc) {
	uint32_t indexToStoreAt = model->getUInt32At(pc);

	OberonExpressionValue* v = exprStack->pop();
	model->setInt64At(v->getAsInt64(), indexToStoreAt);

	delete v;
}

void OberonEngine::processPushI64Literal(uint32_t pc) {
	int64_t value = model->getInt64At(pc);

	exprStack->push(value);
}

void OberonEngine::processPushFP64Literal(uint32_t pc) {
	double value = model->getFP64At(pc);

	exprStack->push(value);
}

void OberonEngine::processPushUI32Literal(uint32_t pc) {
	uint32_t value = model->getUInt32At(pc);

	exprStack->push(value);
}

void OberonEngine::processPushFP64(uint32_t pc) {
	uint32_t index = model->getUInt32At(pc);
	double value = model->getFP64At(index);

	exprStack->push(value);
}

void OberonEngine::processPushI64(uint32_t pc) {
	uint32_t index = model->getUInt32At(pc);
	int64_t value = model->getInt64At(index);

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
