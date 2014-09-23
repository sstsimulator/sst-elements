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
#include "embergen.h"

using namespace SST::Ember;

EmberGenerator::EmberGenerator( Component* owner, Params& params ) {

}

EmberGenerator::~EmberGenerator() {

}

void EmberGenerator::setRankMap(EmberRankMap* mapper) {
	rankMap = mapper;
}

void EmberGenerator::getPosition(const int32_t rank, const int32_t px, const int32_t py, const int32_t pz,
                int32_t* myX, int32_t* myY, int32_t* myZ) {

	rankMap->getPosition(rank, px, py, pz, myX, myY, myZ);
}

void EmberGenerator::getPosition(const int32_t rank, const int32_t px, const int32_t py,
                int32_t* myX, int32_t* myY) {

	rankMap->getPosition(rank, px, py, myX, myY);
}

int32_t EmberGenerator::convertPositionToRank(const int32_t peX, const int32_t peY, const int32_t peZ,
	const int32_t posX, const int32_t posY, const int32_t posZ) {

	rankMap->convertPositionToRank(peX, peY, peZ, posX, posY, posZ);
}
