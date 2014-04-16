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


#ifndef _H_SST_VANADIS_REG_FILE
#define _H_SST_VANADIS_REG_FILE

#include <stdint.h>

#include "regset.h"

namespace SST {
namespace Vanadis {

class VanadisRegisterFile {

	protected:
		VanadisRegisterSet** register_sets;
		const uint32_t threadCount;

	public:
		VanadisRegisterFile(uint32_t thrCount, uint32_t regCount, uint32_t regWidthBytes);
		VanadisRegisterSet* getRegisterForThread(uint32_t thrID);

};

}
}

#endif
