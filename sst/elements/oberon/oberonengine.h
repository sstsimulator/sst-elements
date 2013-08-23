
#ifndef _SST_OBERON_ENGINE
#define _SST_OBERON_ENGINE

#include <sst/sst_config.h>
#include <sst/core/output.h>

#include <string>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#include "oberonmodel.h"
#include "obexprstack.h"
#include "oberonev.h"
#include "oberoninst.h"
#include "obexprval.h"

using namespace std;

namespace SST {
namespace Oberon {

class OberonEngine {

	public:
		OberonEngine(OberonModel* model, Output* out);
		OberonEvent* generateNextEvent();
		bool instanceHalted();

	private:
		int32_t pc;
		OberonModel* model;
		OberonExpressionStack* exprStack;
		bool isHalted;
		Output* output;

		void processI64Add();
		void processI64Sub();
		void processI64Mul();
		void processI64Div();
		void processI64Mod();
		void processI64Pow();

		void processI64LT();
		void processI64LTE();
		void processI64EQ();
		void processI64GT();
		void processI64GTE();

		void processPushI64(int32_t currentPC);
		void processPushFP64(int32_t currentPC);

		void processPushUI32Literal(int32_t currentPC);
		void processPushI64Literal(int32_t currentPC);
		void processPushFP64Literal(int32_t currentPC);

		void processPopI64(int32_t currentPC);
//		void popUI32();
		void processPopFP64(int32_t currentPC);

		int32_t processUnconditionalJump(int32_t currentPC);
		int32_t processUnconditionalJumpRelative(int32_t currentPC);
		int32_t processConditionalJumpRelativeOnZero(int32_t currentPC);
		int32_t processConditionalJumpRelativeOnNonZero(int32_t currentPC);

		void processPrintI64();

		void processHalt();

};

}
}

#endif
