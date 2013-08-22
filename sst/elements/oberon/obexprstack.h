
#ifndef _SST_OBERON_EXPR_STACK
#define _SST_OBERON_EXPR_STACK

#include <stack>
#include <stdint.h>
#include <assert.h>

#include "obexprval.h"
#include "obi64val.h"
#include "obfp64val.h"

using namespace std;

namespace SST {
namespace Oberon {

class OberonExpressionStack {

	private:
		stack<OberonExpressionValue*> exprStack;

	public:
		OberonExpressionStack();
		OberonExpressionValue* pop();
		void push(OberonExpressionValue* v);
		void push(int64_t v);
		void push(double v);
		void push(uint32_t v);
		uint32_t size();

};

}
}

#endif
