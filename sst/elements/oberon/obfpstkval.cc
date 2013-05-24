
using namespace SST;
using namespace Oberon;

#include <stdio.h>

OberonDoubleStackValue::OberonDoubleStackValue(double v) {
	value = v;
}

double OberonDoubleStackValue::getDoubleValue() {
	return value;
}

int64_t OberonDoubleStackValue::getIntegerValue() {
	return (int64_t) value;
}

bool OberonDoubleStackValue::getBooleanValue() {
	std::cerr << "Attempted to convert an double value to a boolean value implicitly" << std::endl;
	abort();
}

string OberonDoubleStackValue::getStringValue() {
	char buffer[64];
	sprintf(buffer, "%f", value);
	string retVal(buffer);
	
	return retVal;
}

OberonExpressionType OberonDoubleStackValue::getExpressionType() {
	return DOUBLE;
}

OberonExpressionStackValue OberonDoubleStackValue::add(OberonExpressionStackValue& val) {
	OberonExpressionStackValue retValue(value + val.getDoubleValue());
	return retValue;
}

OberonExpressionStackValue OberonDoubleStackValue::sub(OberonExpressionStackValue& val) {
	OberonExpressionStackValue retValue(value - val.getDoubleValue());
	return retValue;
}

OberonExpressionStackValue OberonDoubleStackValue::div(OberonExpressionStackValue& val) {
	OberonExpressionStackValue retValue(value / val.getDoubleValue());
	return retValue;
}

OberonExpressionStackValue OberonDoubleStackValue::mul(OberonExpressionStackValue& val) {
	OberonExpressionStackValue retValue(value * val.getDoubleValue());
	return retValue;
}

OberonBooleanStackValue OberonDoubleStackValue::eq(OberonExpressionStackValue val) {
	bool check = ( value == val.getDoubleValue() );
	OberonBooleanStackValue retVal(check);

	return check;
}

OberonBooleanStackValue OberonDoubleStackValue::not() {
	std::cerr << "Attempted to perform a NOT operation against a double precision value." << std::endl;
	abort();
}

OberonBooleanStackValue OberonDoubleStackValue::lt(OberonExpressionStackValue val) {
	bool check = ( value < val.getDoubleValue() );
	OberonBooleanStackValue retVal(check);

	return check;
}

OberonBooleanStackValue OberonDoubleStackValue::lte(OberonExpressionStackValue val) {
	bool check = ( value <= val.getDoubleValue() );
	OberonBooleanStackValue retVal(check);

	return check;
}

OberonBooleanStackValue OberonDoubleStackValue::gt(OberonExpressionStackValue val) {
	bool check = ( value > val.getDoubleValue() );
	OberonBooleanStackValue retVal(check);

	return check;
}

OberonBooleanStackValue OberonDoubleStackValue::gte(OberonExpressionStackValue val) {
	bool check = ( value >= val.getDoubleValue() );
	OberonBooleanStackValue retVal(check);

	return check;
}

OberonExpressionStackValue OberonDoubleStackValue::and(OberonExpressionStackValue val) {
	std::cerr << "Attempted to perform an AND operation against a double precision value." << std::endl;
	abort();
}

OberonExpressionStackValue OberonDoubleStackValue::or(OberonExpressionStackValue val) {
	std::cerr << "Attempted to perform an OR operation against a double precision value." << std::endl;
	abort();
}

void OberonDoubleStackValue::print() {
	printf("%f\n", value);
}
