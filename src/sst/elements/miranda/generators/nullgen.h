// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_SST_MIRANDA_EMPTY_GEN
#define _H_SST_MIRANDA_EMPTY_GEN

#include <sst/elements/miranda/mirandaGenerator.h>
#include <sst/core/output.h>

#include <queue>

using namespace SST::RNG;

namespace SST {
namespace Miranda {

class EmptyGenerator : public RequestGenerator {

public:
	EmptyGenerator( Component* owner, Params& params ) : RequestGenerator(owner, params) {}
	~EmptyGenerator() { }
	void generate(MirandaRequestQueue<GeneratorRequest*>* q) { }
	bool isFinished() { return true; }
	void completed() { }

        SST_ELI_REGISTER_SUBCOMPONENT(
		EmptyGenerator,
               	"miranda",
                "EmptyGenerator",
                SST_ELI_ELEMENT_VERSION(1,0,0),
               	"Creates an empty (null) generator",
                "SST::Miranda::RequestGenerator"
        )

	SST_ELI_DOCUMENT_PARAMS(
        )

};

}
}

#endif
