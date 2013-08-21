
#include "oberonengine.h"

/*        public:
               	void generateNextEvent();

        private:
                OberonModel model;
                OberonExpressionStack exprStack;
                bool isHalted;

#define OBERON_INST_NOP         0

#define OBERON_INST_PUSH        1
#define OBERON_INST_POP         2

#define OBERON_INST_PUSH_I64    1
#define OBERON_INST_POP_I64     2

#define OBERON_INST_PUSH_FP64   3
#define OBERON_INST_POP_FP64    4

#define OBERON_INST_PUSH_LIT_I64        5
#define OBERON_INST_PUSH_LIT_U32        6
#define OBERON_INST_PUSH_LIT_FP64       7

#define OBERON_INST_IADD        16
#define OBERON_INST_ISUB        17
#define OBERON_INST_IMUL        18
#define OBERON_INST_IDIV        19
#define OBERON_INST_IMOD        20

*/

OberonEngine::OberonEngine() {
	pc = 0;
}

void OberonEngine::generateNextEvent() {
	uint32_t incPC = 0;
	bool continueProcessing = true;

	while(continueProcessing) {
		const uint32_t thisInst = model.getInstructionAt(pc);

		switch(thisInst) {

		case OBERON_INST_PUSH_I64:
		case OBERON_INST_PUSH_FP64:
		case OBERON_INST_PUSH_LIT_U32:
			incPC = 4;
			uint32_t val = model.getUInt32At(pc + incPC);

			switch(thisInt) {

			case OBERON_INST_PUSH_I64:
				exprStack.push(model.getInt64At(val));
				break;
			case OBERON_INST_PUSH_FP64:
				exprStack.push(model.getFP64At(val));
				break;

			case OBERON_INST_PUSH_LIT_U32:
				exprStack.push(val);
				break;

			}

			incPC += 4;
			break;

		case OBERON_INST_POP_I64:
			incPC = 4;

			uint32_t idx = model.getUInt32At(pc + incPC);
			OberonExpresssionValue* v = exprStack.pop();
			model->setInt64At(v->getAtInt64());

			delete v;
			break;

		case OBERON_INST_POP_FP64:
			incPC = 4;

			uint32_t idx = model.getUInt32At(pc + incPC);
			OberonExpresssionValue* v = exprStack.pop();
			model->setFP64At(v->getAtFP64());

			delete v;
			break;

		case OBERON_INST_IADD:
		case OBERON_INST_ISUB:
		case OBERON_INST_IMUL:
		case OBERON_INST_IDIV:
		case OBERON_INST_IMOD:
                	incPC = 4;

			OberonExpressionValue* r = exprStack.pop();
			OberonExpressionValue* l = exprStack.pop();

			int64_t result = 0;

			switch(thisInt) {

				case: OBERON_INST_IADD:
				result = l->getAsInt64() + r->getAsInt64();
				break;

				case OBERON_INST_ISUB:
				result = l->getAsInt64() - r->getAsInt64();
				break;

				case OBERON_INST_IMUL:
				result = l->getAsInt64() * r->getAsInt64();
				break;

				case OBERON_INST_IDIV:
				result = l->getAsInt64() + r->getAsInt64();
				break;

				case OBERON_INST_IMOD:
				result = l->getAsInt64() % r->getAsInt64();
				break;

			}

			exprStack.push(result);

			delete r;
			delete l;

			break;

		case OBERON_INST_NOP:
			incPC = 4;
			break;

		}

		pc += incPC;
	}

}
