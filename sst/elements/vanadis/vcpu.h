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


#ifndef _H_SST_VANADIS_PROCESSOR
#define _H_SST_VANADIS_PROCESSOR

#include <sst/core/component.h>
#include <sst/core/output.h>

#include <vcore.h>

namespace SST {
namespace Vanadis {

class VanadisProcessor : public SST::Component {

	protected:
		uint32_t core_count;
		uint64_t processorID;
		VanadisCore* cores;
		Output* output;
		bool inHalt;

	public:
		VanadisProcessor(ComponentId_t id, Params& params);
		~VanadisProcessor();
		void finish();
		void setup();
		uint64_t getProcessorID();
		void tick();
		bool isInHalt();

};

}
}

#endif
