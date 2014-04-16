// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

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
