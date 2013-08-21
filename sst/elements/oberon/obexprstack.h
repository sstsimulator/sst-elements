
#ifndef _SST_OBERON_EXPR_STACK
#define _SST_OBERON_EXPR_STACK

using <stack>

class OberonExpressionStack {

	private:
		stack<OberonExpressionValue*> exprStack;

	public:
		OberonExpressionValue* pop();
		void push(OberonExpressionValue* v);
		void push(int64_t v);
		void push(double v);
		uint32_t size();

}

#endif
