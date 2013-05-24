
#include "oberonexprstack.h"

using namespace SST;
using namespace Oberon;

OberonExpressionStackValue::OberonExpressionStackValue() :

}

OberonExpressionStack::OberonExpressionStack() {

}

OberonExpressionStackValue OberonExpressionStack::pop() {
	assert(stack.size() > 0);
	return stack.pop_back();
}

OberonExpressionStackValue::push(OberonExpressionStackValue& val) {
	stack.push_back(val);
}
