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
