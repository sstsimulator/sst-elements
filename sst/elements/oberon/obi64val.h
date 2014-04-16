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


#ifndef SST_OBERON_I64_VALUE
#define SST_OBERON_I64_VALUE

#include <stdint.h>

#include "obexprval.h"

namespace SST {
namespace Oberon {

class OberonI64ExprValue : public OberonExpressionValue {

	private:
		int64_t value;

	public:
		OberonI64ExprValue(int64_t v);
		int64_t getAsInt64();
		double  getAsFP64();

};

}
}

#endif
