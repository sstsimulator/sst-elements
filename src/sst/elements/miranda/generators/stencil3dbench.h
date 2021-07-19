// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_SST_MIRANDA_STENCIL3D_BENCH_GEN
#define _H_SST_MIRANDA_STENCIL3D_BENCH_GEN

#include <sst/elements/miranda/mirandaGenerator.h>
#include <sst/core/output.h>

#include <queue>

namespace SST {
namespace Miranda {

class Stencil3DBenchGenerator : public RequestGenerator {

public:
	Stencil3DBenchGenerator( ComponentId_t id, Params& params );
        void build(Params& params);
	~Stencil3DBenchGenerator();
	void generate(MirandaRequestQueue<GeneratorRequest*>* q);
	bool isFinished();
	void completed();

	SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
                Stencil3DBenchGenerator,
                "miranda",
                "Stencil3DBenchGenerator",
                SST_ELI_ELEMENT_VERSION(1,0,0),
		"Creates a representation of a 3D 27pt stencil benchmark",
                SST::Miranda::RequestGenerator
        )

	SST_ELI_DOCUMENT_PARAMS(
		{ "nx",               "Sets the dimensions of the problem space in X", "10"},
	    	{ "ny",               "Sets the dimensions of the problem space in Y", "10"},
    		{ "nz",               "Sets the dimensions of the problem space in Z", "10"},
    		{ "verbose",          "Sets the verbosity output of the generator", "0" },
    		{ "datawidth",        "Sets the data width of the mesh element, typically 8 bytes for a double", "8" },
    		{ "startz",           "Sets the start location in Z-plane for this instance, parallelism implemented as Z-plane decomposition", "0" },
    		{ "endz",             "Sets the end location in Z-plane for this instance, parallelism implemented as Z-plane decomposition", "10" },
    		{ "iterations",       "Sets the number of iterations to perform over this mesh", "1"}
       	)

private:
	void convertIndexToPosition(const uint32_t index,
		uint32_t* posX, uint32_t* posY, uint32_t* posZ);
	uint32_t convertPositionToIndex(const uint32_t posX,
		const uint32_t posY, const uint32_t posZ);

	uint32_t nX;
	uint32_t nY;
	uint32_t nZ;
	uint32_t datawidth;

	uint32_t startZ;
	uint32_t endZ;

	uint32_t currentZ;
	uint32_t currentItr;
	uint32_t maxItr;

	Output*  out;

};

}
}

#endif
