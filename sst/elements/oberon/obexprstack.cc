#include "obexprstack.h"


OberonExpressionStack::OberonExpressionStack() :
	exprStack() {

}

OberonExpressionValue* OberonExpressionStack::pop() {
	assert(exprStack.size() > 0);

	OberonExpressionValue* v = exprStack.top();
	exprStack.pop();

	return v;
}

uint32_t OberonExpressionStack::size() {
	return (uint32_t) exprStack.size();
}

void OberonExpressionStack::push(OberonExpressionValue* v) {
	exprStack.push(v);
}

void OberonExpressionStack::push(int64_t v) {
	OberonExpressionValue* v = new OberonI64ExprValue(v);
	exprStack.push(v);
}

void OberonExpressionStack::push(double v) {
	assert(0);
}
