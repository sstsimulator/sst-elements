
#ifndef _SST_EMBER_RANK_MAP
#define _SST_EMBER_RANK_MAP

#include <sst/core/module.h>

namespace SST {
namespace Ember {

class EmberRankMap : public Module {

public:
	EmberRankMap(Component* owner, Params& params) {}
	~EmberRankMap() {}
	virtual void setEnvironment(const uint32_t rank, const uint32_t worldSize) = 0;
	virtual uint32_t mapRank(const uint32_t input) = 0;

	virtual void getPosition(const int32_t rank, const int32_t px, const int32_t py, const int32_t pz,
                int32_t* myX, int32_t* myY, int32_t* myZ) = 0;

        virtual void getPosition(const int32_t rank, const int32_t px, const int32_t py,
                int32_t* myX, int32_t* myY) = 0;

        virtual int32_t convertPositionToRank(const int32_t peX, const int32_t peY, const int32_t peZ,
                const int32_t posX, const int32_t posY, const int32_t posZ) = 0;

};

}
}

#endif
