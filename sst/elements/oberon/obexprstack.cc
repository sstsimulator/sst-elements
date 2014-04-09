#include <sst_config.h>
#include "obexprstack.h"

using namespace SST;
using namespace SST::Oberon;

OberonExpressionStack::OberonExpressionStack() :
	exprStack() {

}

OberonExpressionValue* OberonExpressionStack::pop() {
	assert(exprStack.size() > 0);

	OberonExpressionValue* v = exprStack.top();
	exprStack.pop();

	return v;
}

int32_t OberonExpressionStack::size() {
	return (int32_t) exprStack.size();
}

void OberonExpressionStack::push(OberonExpressionValue* v) {
	exprStack.push(v);
}

void OberonExpressionStack::push(int64_t v) {
	OberonI64ExprValue* stackVal = new OberonI64ExprValue(v);
	exprStack.push(stackVal);
}

void OberonExpressionStack::push(double v) {
	OberonFP64ExprValue* stackVal = new OberonFP64ExprValue(v);
	exprStack.push(stackVal);
}

void OberonExpressionStack::push(int32_t v) {
	assert(0);
}
