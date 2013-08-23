
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
		uint32_t pc;
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

		void processPushI64(uint32_t currentPC);
		void processPushFP64(uint32_t currentPC);

		void processPushUI32Literal(uint32_t currentPC);
		void processPushI64Literal(uint32_t currentPC);
		void processPushFP64Literal(uint32_t currentPC);

		void processPopI64(uint32_t currentPC);
//		void popUI32();
		void processPopFP64(uint32_t currentPC);

		uint32_t processUnconditionalJump(uint32_t currentPC);
		uint32_t processUnconditionalJumpRelative(uint32_t currentPC);

		void processPrintI64();

		void processHalt();

};

}
}

#endif
