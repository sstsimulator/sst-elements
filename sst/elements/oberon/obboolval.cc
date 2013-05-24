
#include "obboolval.h"

using namespace SST;
using namespace Oberon;

OberonBooleanStackValue::OberonBooleanStackValue(bool v) {
	value = v;
}

OberonExpressionStackValue OberonBooleanStackValue::add(OberonExpressionStackValue val) {
	std::cerr << "Attempted to perform an ADD operation on a boolean value." << std::endl;
	abort();
}

OberonExpressionStackValue OberonBooleanStackValue::sub(OberonExpressionStackValue val) {
	std::cerr << "Attempted to perform an SUB operation on a boolean value." << std::endl;
	abort();
}

OberonExpressionStackValue OberonBooleanStackValue::div(OberonExpressionStackValue val) {
	std::cerr << "Attempted to perform an DIV operation on a boolean value." << std::endl;
	abort();
}

OberonExpressionStackValue OberonBooleanStackValue::mul(OberonExpressionStackValue val) {
	std::cerr << "Attempted to perform an MUL operation on a boolean value." << std::endl;
	abort();
}

OberonBooleanStackValue OberonBooleanStackValue::eq(OberonExpressionStackValue val) {
	bool check = (value == val.getBooleanValue());
	OberonBooleanStackValue retVal(check);
	
	return retVal;
}

OberonBooleanStackValue OberonBooleanStackValue::not() {
	OberonBooleanStackValue retVal( ! value );
	return retVal;
}

OberonBooleanStackValue OberonBooleanStackValue::lt(OberonExpressionStackValue val) {
	std::cerr << "Attempted to perform an LT operation on a boolean value." << std::endl;
	abort();
}

OberonBooleanStackValue OberonBooleanStackValue::lte(OberonExpressionStackValue val) {
	std::cerr << "Attempted to perform an LTE operation on a boolean value." << std::endl;
	abort();
}

OberonBooleanStackValue OberonBooleanStackValue::gt(OberonExpressionStackValue val) {
	std::cerr << "Attempted to perform an GT operation on a boolean value." << std::endl;
	abort();
}

OberonBooleanStackValue OberonBooleanStackValue::gte(OberonExpressionStackValue val) {
	std::cerr << "Attempted to perform an GTE operation on a boolean value." << std::endl;
	abort();
}


OberonExpressionStackValue OberonBooleanStackValue::and(OberonExpressionStackValue val) {
	bool check = (value & val.getBooleanValue());
	OberonBooleanStackValue retVal(check);
	
	return retVal;
}

OberonExpressionStackValue OberonBooleanStackValue::or(OberonExpressionStackValue val) {
	bool check = (value | val.getBooleanValue());
	OberonBooleanStackValue retVal(check);
	
	return retVal;
}

void OberonBooleanStackValue::print() {
	if(value)
		printf("TRUE");
	else
		printf("FALSE");
}

OberonExpressionType OberonBooleanStackValue::getExpressionType() {
	return BOOLEAN;
}
		
double OberonBooleanStackValue::getDoubleValue() {
	std::cerr << "Attempted to convert a boolean value to a double precision value." << std::endl;
	abort();
}

int64_t OberonBooleanStackValue::getIntegerValue() {
	std::cerr << "Attempted to convert a boolean value to an integer value." << std::endl;
	abort();
}

bool OberonBooleanStackValue::getBooleanValue() {
	return value;
}

string OberonBooleanStackValue::getStringValue() {
	if(value)
		return "TRUE";
	else
		return "FALSE";
}