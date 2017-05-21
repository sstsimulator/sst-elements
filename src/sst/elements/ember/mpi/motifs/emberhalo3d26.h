// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2017, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_EMBER_HALO_3D_26
#define _H_EMBER_HALO_3D_26

#include "mpi/embermpigen.h"

namespace SST {
namespace Ember {

class EmberHalo3D26Generator : public EmberMessagePassingGenerator {

public:
	EmberHalo3D26Generator(SST::Component* owner, Params& params);
	~EmberHalo3D26Generator() {}
    bool generate( std::queue<EmberEvent*>& evQ);

private:
#ifdef HAVE_STDCXX_LAMBDAS
	std::function<uint64_t()> compute_the_time;
#else
	uint64_t compute_the_time;
#endif
	std::vector<MessageRequest> requests;
	bool performReduction;

	uint32_t m_loopIndex;
	uint32_t iterations;

	// Share these over all instances of the motif
	uint32_t peX;
	uint32_t peY;
	uint32_t peZ;

	uint32_t nsCopyTime;

	uint32_t nx;
	uint32_t ny;
	uint32_t nz;
	uint32_t items_per_cell;
	uint32_t sizeof_cell;

	int32_t  xface_down;
	int32_t  xface_up;

	int32_t  yface_down;
	int32_t  yface_up;

	int32_t  zface_down;
	int32_t  zface_up;

	int32_t  line_a;
	int32_t  line_b;
	int32_t  line_c;
	int32_t  line_d;
	int32_t  line_e;
	int32_t  line_f;
	int32_t  line_g;
	int32_t  line_h;
	int32_t  line_i;
	int32_t  line_j;
	int32_t  line_k;
	int32_t  line_l;

	int32_t  corner_a;
	int32_t  corner_b;
	int32_t  corner_c;
	int32_t  corner_d;
	int32_t  corner_e;
	int32_t  corner_f;
	int32_t  corner_g;
	int32_t  corner_h;
};

}
}

#endif
