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


#ifndef SST_OBERON_EXPR_VALUE_BASE
#define SST_OBERON_EXPR_VALUE_BASE

namespace SST {
namespace Oberon {

class OberonExpressionValue {

	public:
		virtual int64_t getAsInt64() = 0;
		virtual double  getAsFP64()  = 0;

};

}
}

#endif
