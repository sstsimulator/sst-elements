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


#ifndef _H_SST_VANADIS_CORE
#define _H_SST_VANADIS_CORE

#include <sst/core/output.h>

using namespace SST;

namespace SST {
namespace Vanadis {

class VanadisCore {

	protected:
		VanadisHWContext* hwContexts;
		VanadisRegisterFile* regFile;

		const uint32_t hwContextCount;
		uint64_t ownerProcessor;
		Output* output;

	public:
		VanadisCore(const uint32_t hwCxtCount, const uint32_t verbosity,
			const uint64_t ownerProc, const uint32_t regCount, const uint32_t regWidthBytes);
		~VanadisCore();
		void tick();

};

}
}

#endif
