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


#ifndef _SST_EMBER_LINEAR_RANK_MAP
#define _SST_EMBER_LINEAR_RANK_MAP

#include "embermap.h"

namespace SST {
namespace Ember {

class EmberLinearRankMap : public EmberRankMap {
public:

    SST_ELI_REGISTER_MODULE(
        EmberLinearRankMap,
        "ember",
        "LinearMap",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Performs a linear mapping of MPI ranks",
        "SST::Ember::EmberRankMap"
    )

    SST_ELI_DOCUMENT_PARAMS(
    )

public:

	EmberLinearRankMap(Params& params) : EmberRankMap(params) {}
	~EmberLinearRankMap() {}

	void setEnvironment(const uint32_t rank, const uint32_t worldSize) {};
	uint32_t mapRank(const uint32_t input) { return input; }

	void getPosition(const int32_t rank, const int32_t px, const int32_t py, const int32_t pz,
                int32_t* myX, int32_t* myY, int32_t* myZ) {

        	const int32_t my_plane  = rank % (px * py);
        	*myY                    = my_plane / px;
        	const int32_t remain    = my_plane % px;
        	*myX                    = remain != 0 ? remain : 0;
        	*myZ                    = rank / (px * py);
	}

	void getPosition(const int32_t rank, const int32_t px, const int32_t py,
                int32_t* myX, int32_t* myY) {

        	*myX = rank % px;
        	*myY = rank / px;
	}

	int32_t convertPositionToRank(const int32_t px, const int32_t py,
        	const int32_t myX, const int32_t myY) {

        	if( (myX < 0) || (myY < 0) || (myX >= px) || (myY >= py) ) {
                	return -1;
        	} else {
                	return (myY * px) + myX;
        	}
	}

	int32_t convertPositionToRank(const int32_t px, const int32_t py, const int32_t pz,
        	const int32_t myX, const int32_t myY, const int32_t myZ) {

        	if( (myX < 0) || (myY < 0) || (myZ < 0) || (myX >= px) || (myY >= py) || (myZ >= pz) ) {
                	return -1;
        	} else {
                	return (myZ * (px * py)) + (myY * px) + myX;
        	}
	}

};

}
}

#endif
