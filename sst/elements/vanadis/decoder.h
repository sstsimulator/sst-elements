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



#ifndef _H_SST_VANADIS_DECODER
#define _H_SST_VANADIS_DECODER

#include <queue>

#include "microop.h"

namespace SST {
namespace Vanadis {

class VanadisDecoder {

	protected:
		VanadisDispatchEngine* dispatcher;

	public:
		VanadisDecoder(VanadisDispatchEngine* dispatch);
		virtual void decode(uint64_t* pc, char* inst_buffer) = 0;

};

}
}

#endif
