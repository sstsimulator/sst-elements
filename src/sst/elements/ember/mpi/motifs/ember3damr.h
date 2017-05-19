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


#ifndef _H_EMBER_3D_AMR
#define _H_EMBER_3D_AMR

#include <sst/core/component.h>
#include <sst/core/params.h>
#include <sst/core/output.h>

#include "mpi/embermpigen.h"
#include "ember3damrblock.h"

using namespace SST;

namespace SST {
namespace Ember {

class Ember3DAMRGenerator : public EmberMessagePassingGenerator {

public:
	Ember3DAMRGenerator(SST::Component* owner, Params& params);
	~Ember3DAMRGenerator();
	void configure();
        bool generate( std::queue<EmberEvent*>& evQ );
	int32_t power3(const uint32_t expon);

	uint32_t power2(uint32_t exponent) const;
        void loadBlocks();

	uint32_t calcBlockID(const uint32_t posX, const uint32_t posY, const uint32_t posZ, const uint32_t level);
        void calcBlockLocation(const uint32_t blockID, const uint32_t blockLevel, uint32_t* posX, uint32_t* posY, uint32_t* posZ);
        bool isBlockLocal(const uint32_t bID) const;
	void postBlockCommunication(std::queue<EmberEvent*>& evQ, int32_t* blockComm, uint32_t* nextReq, const uint32_t faceSize, const uint32_t msgTag,
		const Ember3DAMRBlock* theBlock);
	void aggregateBlockCommunication(const std::vector<Ember3DAMRBlock*>& blocks, std::map<int32_t, uint32_t>& blockToMessageSize);
	void aggregateCommBytes(Ember3DAMRBlock* curBlock, std::map<int32_t, uint32_t>& blockToMessageSize);

private:
	void printBlockMap();

        std::vector<Ember3DAMRBlock*> localBlocks;
	MessageRequest*   requests;
	MessageResponse** responses;

	void* blockMessageBuffer;

//        static std::map<uint32_t, int32_t>  blockToNodeMap;
        char* blockFilePath;

	Output* out;

	uint32_t iteration;
	uint32_t maxIterations;
	uint32_t nextBlockToBeProcessed;
	uint32_t nextRequestID;
	uint32_t maxRequestCount;

        uint32_t blockCount;
        uint32_t maxLevel;
        uint32_t blocksX;
        uint32_t blocksY;
        uint32_t blocksZ;

        // Share these over all instances of the motif
        uint32_t peX;
        uint32_t peY;
        uint32_t peZ;

        uint32_t nsCompute;
        uint32_t nsCopyTime;

        uint32_t blockNx;
        uint32_t blockNy;
        uint32_t blockNz;

        uint32_t items_per_cell;
        uint32_t sizeof_cell;

        int32_t  x_down;
        int32_t  x_up;
        int32_t  y_down;
        int32_t  y_up;
        int32_t  z_down;
        int32_t  z_up;
	bool printMaps;
    
    int8_t meshType;

};

}
}

#endif
