// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _SST_EMBER_CUSTOM_RANK_MAP
#define _SST_EMBER_CUSTOM_RANK_MAP

#include "embermap.h"
#include <sst/core/tinyxml/tinyxml.h>

namespace SST {
namespace Ember {

class EmberCustomRankMap : public EmberRankMap {

public:

	EmberCustomRankMap(Component* owner, Params& params) : EmberRankMap(owner, params) 
        {
                /*
                jobId     = (int) params.find_integer("_mapjobId");
                std::cout << "EmberCustommap: mapjobId: " << jobId << std::endl;
                mapFile  = params.find_string("mapFile", "sample.sim.snapshot.xml");
                std::cout << "EmberCustommap: mapFile: " << mapFile.c_str() << std::endl;
                */
                /*
                if(jobId != -1){
                        readMapFile(mapFile.c_str());
                }
                readMapFile(mapFile);
                */
                TiXmlDocument doc("sample.sim.snapshot.xml");
                std::cout << "After TiXmlDocument is created." << std::endl;
            
        }
	~EmberCustomRankMap() {}

        // NetworkSim: added variables to construct the custom map
        int jobId;
        std::string mapFile;
        std::map<int, int> customMap;
        // end->NetworkSim

        //NetworkSim: function that reads the custom mapping of the job with _mapjobId
        void readMapFile(std::string fileName) {
                //std::map<int, int> tempMap;
                //std::map<int, int> jobMap;

                TiXmlDocument doc("sample.sim.snapshot.xml");
                std::cout << "After TiXmlDocument is created." << std::endl;
        }

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
