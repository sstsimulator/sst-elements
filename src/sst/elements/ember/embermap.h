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


#ifndef _SST_EMBER_RANK_MAP
#define _SST_EMBER_RANK_MAP

#include <sst/core/module.h>
#include <sst/core/component.h>

namespace SST {
namespace Ember {

class EmberRankMap : public Module {

public:
	EmberRankMap(Params& params) {}
	~EmberRankMap() {}
	virtual void setEnvironment(const uint32_t rank, const uint32_t worldSize) = 0;
	virtual uint32_t mapRank(const uint32_t input) = 0;

	virtual void getPosition(const int32_t rank, const int32_t px, const int32_t py, const int32_t pz,
                int32_t* myX, int32_t* myY, int32_t* myZ) = 0;

    virtual void getPosition(const int32_t rank, const int32_t px, const int32_t py,
                int32_t* myX, int32_t* myY) = 0;

    virtual int32_t convertPositionToRank(const int32_t px, const int32_t py,
	                const int32_t myX, const int32_t myY) = 0;

    virtual int32_t convertPositionToRank(const int32_t px, const int32_t py, const int32_t pz,
	                const int32_t myX, const int32_t myY, const int32_t myZ) = 0;

};

}
}

#endif
