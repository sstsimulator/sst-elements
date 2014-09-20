
#ifndef _SST_EMBER_LINEAR_RANK_MAP
#define _SST_EMBER_LINEAR_RANK_MAP

#include "embermap.h"

class EmberLinearRankMap : public SST::Module {

public:
	EmberLinearRankMap(uint32_t rankID) {}
	~EmberLinearRankMap() {}
	uint32_t mapRank(const uint32_t input) { return input; }

}

#endif
