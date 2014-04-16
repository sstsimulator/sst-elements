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
