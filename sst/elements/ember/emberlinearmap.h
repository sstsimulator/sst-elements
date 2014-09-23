
#ifndef _SST_EMBER_LINEAR_RANK_MAP
#define _SST_EMBER_LINEAR_RANK_MAP

#include "embermap.h"

namespace SST {
namespace Ember {

class EmberLinearRankMap : public EmberRankMap {

public:

	EmberLinearRankMap(Component* owner, Params& params) : EmberRankMap(owner, params) {}
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

	int32_t convertPositionToRank(const int32_t peX, const int32_t peY, const int32_t peZ,
        	const int32_t posX, const int32_t posY, const int32_t posZ) {

        	if( (posX < 0) || (posY < 0) || (posZ < 0) || (posX >= peX) || (posY >= peY) || (posZ >= peZ) ) {
                	return -1;
        	} else {
                	return (posZ * (peX * peY)) + (posY * peX) + posX;
        	}
	}

};

}
}

#endif
