
#ifndef _H_OBERON_EXPRESSION_STACK
#define _H_OBERON_EXPRESSION_STACK

#include <stdint.h>
#include <vector>

#include "obboolval.h"

namespace SST {
namespace Oberon {

enum OberonExpressionType {
	DOUBLE,
	INTEGER,
	BOOLEAN,
	STRING
}

class OberonExpressionStackValue {

	public:
		OberonExpressionStackValue();

		virtual OberonExpressionStackValue add(OberonExpressionStackValue val) = 0;
		virtual OberonExpressionStackValue sub(OberonExpressionStackValue val) = 0;
		virtual OberonExpressionStackValue div(OberonExpressionStackValue val) = 0;
		virtual OberonExpressionStackValue mul(OberonExpressionStackValue val) = 0;

		virtual OberonBooleanStackValue eq(OberonExpressionStackValue val) = 0;
		virtual OberonBooleanStackValue not() = 0;
		virtual OberonBooleanStackValue lt(OberonExpressionStackValue val) = 0;
		virtual OberonBooleanStackValue lte(OberonExpressionStackValue val) = 0;
		virtual OberonBooleanStackValue gt(OberonExpressionStackValue val) = 0;
		virtual OberonBooleanStackValue gte(OberonExpressionStackValue val) = 0;

		virtual OberonExpressionStackValue and(OberonExpressionStackValue val) = 0;
		virtual OberonExpressionStackValue or(OberonExpressionStackValue val) = 0;

		virtual void print() = 0;

		virtual OberonExpressionType getExpressionType() = 0;
		
		virtual double getDoubleValue() = 0;
		virtual int64_t getIntegerValue() = 0;
		virtual bool getBooleanValue() = 0;
		virtual string getStringValue() = 0;

}

class OberonExpressionStack {

	public:
		OberonExpressionStack();
		OberonExpressionStackValue pop();
		void push(OberonExpressionStackValue& val);

	protected:
		vector<OberonExpressionStackValue> stack;

}

}
}

#endif
