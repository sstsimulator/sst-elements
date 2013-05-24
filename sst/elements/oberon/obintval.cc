
#include "obintval.h"

OberonExpressionStackValue OberonIntegerStackValue::add(OberonExpressionStackValue& val) {
	OberonIntegerStackValue retVal(value + val.getIntegerValue());
	return retVal;
}

OberonExpressionStackValue OberonIntegerStackValue::sub(OberonExpressionStackValue& val) {
	OberonIntegerStackValue retVal(value - val.getIntegerValue());
	return retVal;
}

OberonExpressionStackValue OberonIntegerStackValue::div(OberonExpressionStackValue& val) {
	OberonIntegerStackValue retVal(value \ val.getIntegerValue());
	return retVal;
}

OberonExpressionStackValue OberonIntegerStackValue::mul(OberonExpressionStackValue& val) {
	OberonIntegerStackValue retVal(value * val.getIntegerValue());
	return retVal;
}


OberonBooleanStackValue OberonIntegerStackValue::eq(OberonExpressionStackValue& val) {
	OberonIntegerStackValue retVal(value == val.getIntegerValue());
	return retVal;
}

OberonBooleanStackValue OberonIntegerStackValue::not() {
	std::cerr << "Attempted to perform a NOT operation on an integer value" << std::endl;
	abort();
}

OberonBooleanStackValue OberonIntegerStackValue::lt(OberonExpressionStackValue& val) {
	OberonIntegerStackValue retVal(value < val.getIntegerValue());
	return retVal;
}

OberonBooleanStackValue OberonIntegerStackValue::lte(OberonExpressionStackValue& val) {
	OberonIntegerStackValue retVal(value <= val.getIntegerValue());
	return retVal;
}

OberonBooleanStackValue OberonIntegerStackValue::gt(OberonExpressionStackValue& val) {
	OberonIntegerStackValue retVal(value > val.getIntegerValue());
	return retVal;
}

OberonBooleanStackValue OberonIntegerStackValue::gte(OberonExpressionStackValue& val) {
	OberonIntegerStackValue retVal(value >= val.getIntegerValue());
	return retVal;
}


OberonExpressionStackValue OberonIntegerStackValue::and(OberonExpressionStackValue& val) {
	OberonIntegerStackValue retVal(value & val.getIntegerValue());
	return retVal;
}

OberonExpressionStackValue OberonIntegerStackValue::or(OberonExpressionStackValue& val) {
	OberonIntegerStackValue retVal(value | val.getIntegerValue());
	return retVal;
}

void OberonIntegerStackValue::print() {
	printf("%llu", value);
}

OberonExpressionType OberonIntegerStackValue::getExpressionType() {
	return INTEGER;
}

double OberonIntegerStackValue::getDoubleValue() {
	return (double) value;
}

int64_t OberonIntegerStackValue::getIntegerValue() {
	return value;
}

bool OberonIntegerStackValue::getBooleanValue() {
	std::cerr << "Attempted to convert an integer value to a boolean value implicitly" << std::endl;
	abort();
}

string OberonIntegerStackValue::getStringValue() {
	char buffer[64];
	sprintf(buffer, "%llu", value);
	string retVal(buffer);
	
	return retVal;
}
