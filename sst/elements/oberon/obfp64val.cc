
#include <sst_config.h>
#include "obfp64val.h"

using namespace SST;
using namespace SST::Oberon;

OberonFP64ExprValue::OberonFP64ExprValue(double v) {
	value = v;
}

double OberonFP64ExprValue::getAsFP64() {
	return value;
}

int64_t OberonFP64ExprValue::getAsInt64() {
	return (int64_t) value;
}
