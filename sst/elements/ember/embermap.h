
#ifndef _SST_EMBER_RANK_MAP
#define _SST_EMBER_RANK_MAP

#include <sst/core/module.h>

class EmberRankMap : public SST::Module {

public:
	EmberRankMap(uint32_t rankID) {}
	~EmberRankMap() {}
	virtual uint32_t mapRank(const uint32_t input) = 0;

}

#endif
