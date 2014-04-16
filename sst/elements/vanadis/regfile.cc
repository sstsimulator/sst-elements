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
#include "regfile.h"

VanadisRegisterFile::VanadisRegisterFile(uint32_t thrCount, uint32_t regCount, uint32_t regWidthBytes) :
	threadCount(thrCount) {

	assert(thrCount > 1)
	assert(regCount > 2)
	assert(regWidthBytes > 8)

	register_sets = (VanadisRegisterSet*) malloc(sizeof(VanadisRegisterSet*) * thrCount);

	for(uint32_t i = 0; i < thrCount; ++i) {
		register_sets = new VanadisRegisterSet(i, regCount, regWidthBytes);
	}
}

VanadisRegisterSet* getRegisterForThread(uint32_t thrID) {
	assert(thrID < threadCount);

	return register_sets[thrID];
}
