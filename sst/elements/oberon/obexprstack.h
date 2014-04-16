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


#ifndef _SST_OBERON_EXPR_STACK
#define _SST_OBERON_EXPR_STACK

#include <stack>
#include <stdint.h>
#include <assert.h>

#include "obexprval.h"
#include "obi64val.h"
#include "obfp64val.h"

using namespace std;

namespace SST {
namespace Oberon {

class OberonExpressionStack {

	private:
		stack<OberonExpressionValue*> exprStack;

	public:
		OberonExpressionStack();
		OberonExpressionValue* pop();
		void push(OberonExpressionValue* v);
		void push(int64_t v);
		void push(double v);
		void push(int32_t v);
		int32_t size();

};

}
}

#endif
