
#include "obi64val.h"

using namespace SST;
using namespace SST::Oberon;

OberonI64ExprValue::OberonI64ExprValue(int64_t v) {
	value = v;
}

int64_t OberonI64ExprValue::getAsInt64() {
	return value;
}

double OberonI64ExprValue::getAsFP64() {
	return (double) value;
}
