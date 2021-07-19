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


#ifndef _SST_EMBER_CUSTOM_RANK_MAP
#define _SST_EMBER_CUSTOM_RANK_MAP

#include "embermap.h"

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace SST {
namespace Ember {

class EmberCustomRankMap : public EmberRankMap {
public:
    SST_ELI_REGISTER_MODULE(
        EmberCustomRankMap,
        "ember",
        "CustomMap",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "NetworkSim: Performs a custom mapping of MPI ranks based on an input file",
        "SST::Ember::EmberRankMap"
    )
    SST_ELI_DOCUMENT_PARAMS(
        {   "mapFile",          "Sets the name of the input file for custom map", "customMap.txt" },
        {   "_mapjobId",        "Sets the jobId for custom map", "-1" },
    )

public:

	EmberCustomRankMap(Params& params) : EmberRankMap(params)
        {
                jobId    = params.find<std::string>("_mapjobId", "-1");
                //std::cout << "EmberCustommap: mapjobId: " << jobId << std::endl;
                mapFile  = params.find<std::string>("mapFile", "mapFile.txt");
                //std::cout << "EmberCustommap: mapFile: " << mapFile.c_str() << std::endl;
                if(jobId.compare("-1")){
                        readMapFile(mapFile);
                }
        }
	~EmberCustomRankMap() {}

        // NetworkSim: added variables to construct the custom map
        std::string jobId;
        std::string mapFile;
        std::map<int, int> CustomMap; //custom task mapping
        std::map<int, int> InvCustomMap; //inverse of the custom task mapping
        // end->NetworkSim

        //NetworkSim: function that reads the custom mapping of the job with _mapjobId
        void readMapFile(std::string fileName) {

                std::ifstream input;
                input.open( fileName.c_str() );

                if(!input.is_open()){
                        std::cerr << "Error: Unable to open job task map file: \'"
                                    << fileName.c_str() << "\'" << std::endl;
                        exit(-1);
                }

                std::string line;
                std::string startIdentifier = "[JOB " + jobId + " START]";
                std::string endIdentifier = "[JOB " + jobId + " END]";
                bool inDesiredRegion = false;
                int taskNum = 0;

                while (!input.eof()) {
                        getline(input, line);
                        if (line.find_first_not_of(" \t\n") == std::string::npos)
                                continue;

                        if (!line.compare(startIdentifier)){
                                inDesiredRegion = true;
                                continue;
                        } else if (!line.compare(endIdentifier)){
                                inDesiredRegion = false;
                                break;
                        }

                        if (inDesiredRegion){
                                std::string nextStr = "";
                                std::stringstream is(line);

                                is >> nextStr;
                                while(!nextStr.empty()){
                                        CustomMap[taskNum] = std::stoi(nextStr);
                                        InvCustomMap[std::stoi(nextStr)] = taskNum;
                                        taskNum++;
                                        if(!(is >> nextStr)){
                                                break;
                                        }
                                }
                        }
                }
                input.close();

                /*
                for(std::map<int, int>::iterator it = CustomMap.begin(); it != CustomMap.end(); it++){
                        std::cout << "linearMapRankNum: " << it->first << " customMapRankNum: " << it->second << std::endl;
                }
                */
        }

	void setEnvironment(const uint32_t rank, const uint32_t worldSize) {};
	uint32_t mapRank(const uint32_t input) { return input; }

	void getPosition(const int32_t rank, const int32_t px, const int32_t py, const int32_t pz,
                int32_t* myX, int32_t* myY, int32_t* myZ) {

                int32_t customRank = (int32_t) InvCustomMap[rank];

                //std::cout << "rank: " << rank << " customRank: " << customRank << std::endl;


        	//const int32_t my_plane  = rank % (px * py);
                const int32_t my_plane  = customRank % (px * py);
        	*myY                    = my_plane / px;
        	const int32_t remain    = my_plane % px;
        	*myX                    = remain != 0 ? remain : 0;
        	//*myZ                    = rank / (px * py);
                *myZ                    = customRank / (px * py);
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

                int32_t linearMapRank;

        	if( (posX < 0) || (posY < 0) || (posZ < 0) || (posX >= peX) || (posY >= peY) || (posZ >= peZ) ) {
                	return -1;
        	} else {
                	linearMapRank = (posZ * (peX * peY)) + (posY * peX) + posX;
                        //std::cout << "linearMapTaskNum: " << linearMapRank << " customMapTaskNum: " << CustomMap[linearMapRank] << std::endl;
                        //return (posZ * (peX * peY)) + (posY * peX) + posX;
                        return (int32_t) CustomMap[linearMapRank];
        	}
	}

};

}
}

#endif
