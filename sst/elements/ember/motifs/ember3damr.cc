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

#include "ember3damr.h"
#include "emberirecvev.h"
#include "embersendev.h"
#include "emberwaitev.h"
#include "emberallredev.h"

using namespace SST::Ember;

Ember3DAMRGenerator::Ember3DAMRGenerator(SST::Component* owner, Params& params) :
	EmberMessagePassingGenerator(owner, params) {

	nx  = (uint32_t) params.find_integer("nx", 100);
	ny  = (uint32_t) params.find_integer("ny", 100);
	nz  = (uint32_t) params.find_integer("nz", 100);

	peX = (uint32_t) params.find_integer("pex", 0);
	peY = (uint32_t) params.find_integer("pey", 0);
	peZ = (uint32_t) params.find_integer("pez", 0);

	x_down = -1;
	x_up   = -1;
	y_down = -1;
	y_up   = -1;
	z_down = -1;
	z_up   = -1;

	std::string blockpath = params.find_string("blockfile", "blocks.amr");
	blockFilePath = (char*) malloc(sizeof(char) * (blockpath.size() + 1));

	strcpy(blockFilePath, blockpath.c_str());
	blockFilePath[blockpath.size()] = '\0';
}

void Ember3DAMRGenerator::loadBlocks() {
	FILE* blockFile = fopen(blockFilePath, "rt");

	const int header = fscanf(blockFile, "%" PRIu32 " %" PRIu32 " %" PRIu32 " %" PRIu32 " %" PRIu32 "\n",
		&blockCount, &maxLevel, &blocksX, &blocksY, &blocksZ);

	if( EOF == header ) {
		fprintf(stderr, "Error: Unable to read AMR header.\n");
		exit(-1);
	}

	uint32_t blocksOnRank = 0;
	uint32_t blockID = 0;
	int32_t  blockLevel = 0;
	int32_t  xUp = 0;
	int32_t  xDown = 0;
	int32_t  yUp = 0;
	int32_t  yDown = 0;
	int32_t  zUp = 0;
	int32_t  zDown = 0;

	uint32_t line = 0;

	for(uint32_t currentRank = 0; currentRank < worldSize; ++currentRank) {
		printf("Loading blocks for rank %" PRIu32 " line %" PRIu32 "\n", currentRank, line);
		line++;

		if( EOF == fscanf(blockFile, "%" PRIu32 "\n", &blocksOnRank) ) {
			// Error
			fprintf(stderr, "Error: Unable to read blocks on rank %" PRIu32 " world is %" PRIu32 "\n", currentRank, worldSize);
			exit(-1);
		} else {
			if(currentRank == rank) {
				for(uint32_t lineID = 0; lineID < blocksOnRank; ++lineID) {
					//printf("Reading line %" PRIu32 "\n", line);
					++line;

					// I need to pay attention and record the file contents
					fscanf(blockFile, "%" PRIu32 " %d %d %d %d %d %d %d\n",
						&blockID, &blockLevel, &xDown, &xUp, &yDown, &yUp, &zDown, &zUp);

					localBlocks.push_back( new Ember3DAMRBlock(blockID, blockLevel, xUp, xDown, yUp,
						yDown, zUp, zDown) );
				}
			} else {
				for(uint32_t lineID = 0; lineID < blocksOnRank; ++lineID) {
					line++;

					fscanf(blockFile, "%" PRIu32 " %d %d %d %d %d %d %d\n",
						&blockID, &blockLevel, &xDown, &xUp, &yDown, &yUp, &zDown, &zUp);

					blockToNodeMap.insert( std::pair<uint32_t, uint32_t>(blockID, currentRank) );
				}
			}
		}
	}

	fclose(blockFile);

	printf("Performing wire up...\n");

	for(uint32_t i = 0; i < localBlocks.size(); ++i) {
		Ember3DAMRBlock* currentBlock = localBlocks[i];

		printf("- Wiring block %" PRIu32 "...\n", currentBlock->getBlockID());

		const uint32_t blockLevel = currentBlock->getRefinementLevel();
		uint32_t blockXPos = 0;
		uint32_t blockYPos = 0;
		uint32_t blockZPos = 0;

		calcBlockLocation(currentBlock->getBlockID(), blockLevel, &blockXPos, &blockYPos, &blockZPos);

		// Patch up X-Up
		const uint32_t blockXUp = currentBlock->getRefineXUp();

		if(blockXUp == -2) {
			// Boundary condition, no communication
			currentBlock->setCommXUp(-1, -1);
		} else if(blockLevel > blockXUp) {
			// Communication to a coarser level (my refinement is higher than block next to me)
			const uint32_t commToBlock = calcBlockID((blockXPos / 2) + 1,
				blockYPos / 2, blockZPos / 2, blockXUp);

			std::map<uint32_t, uint32_t>::iterator blockNode = blockToNodeMap.find(commToBlock);

			if(blockNode == blockToNodeMap.end() && isBlockLocal(commToBlock)) {
				if( ! isBlockLocal(commToBlock) ) {
					printf("Did not locate block: %" PRIu32 "\n", commToBlock);
					exit(-1);
				}
			} else {
				// Projecting to coarse level
				currentBlock->setCommXUp(blockNode->second, -1);
			}
		} else if(blockLevel < blockXUp) {
			// Communication to a finer level (my refinement is less than block next to me)
			const uint32_t commToBlockLower = calcBlockID(blockXPos * 2 + 2,
				blockYPos * 2,     blockZPos * 2, blockXUp);
			const uint32_t commToBlockUpper = calcBlockID(blockXPos * 2 + 2,
				blockYPos * 2 + 1, blockZPos * 2, blockXUp);

			std::map<uint32_t, uint32_t>::iterator blockNodeLower = blockToNodeMap.find(commToBlockLower);
			std::map<uint32_t, uint32_t>::iterator blockNodeUpper = blockToNodeMap.find(commToBlockUpper);

			if( blockNodeLower == blockToNodeMap.end() ) {
				if( ! isBlockLocal(commToBlockLower) ) {
					printf("Did not find lower blocks: %" PRIu32 ", current ID=%" PRIu32 "\n",
						commToBlockLower, currentBlock->getBlockID());
					exit(-1);
				}
			} else if( blockNodeUpper == blockToNodeMap.end() ) {
				if( ! isBlockLocal(commToBlockUpper) ) {
					printf("Did not find upper blocks: %" PRIu32 "\n", commToBlockUpper);
					exit(-1);
				}
			} else {
				currentBlock->setCommXUp(blockNodeLower->second, blockNodeUpper->second);
			}
		} else {
			// Same level
			const uint32_t blockNextToMe = calcBlockID(blockXPos + 1,
				blockYPos, blockZPos, blockXUp);

			std::map<uint32_t, uint32_t>::iterator blockNextToMeNode = blockToNodeMap.find(blockNextToMe);

			if(blockNextToMeNode == blockToNodeMap.end()) {
				if( ! isBlockLocal(blockNextToMe) ) {
					printf("Did not find block next %" PRIu32 ", current block=%" PRIu32 "\n",
						blockNextToMe, currentBlock->getBlockID());
					exit(-1);
				}
			} else {
				currentBlock->setCommXUp(blockNextToMeNode->second, -1);
			}
		}

	}

	printf("Blocks on rank %" PRIu32 " count=%lu\n", rank, localBlocks.size());
}

uint32_t Ember3DAMRGenerator::power2(uint32_t exponent) const {
	uint32_t result = 1;

	for(uint32_t i = 0; i < exponent; ++i) {
		result *= 2;
	}

	return result;
}

void Ember3DAMRGenerator::calcBlockLocation(const uint32_t blockID, const uint32_t blockLevel, uint32_t* posX, uint32_t* posY, uint32_t* posZ) {
	uint32_t levelStartIndex = 0;

	for(uint32_t nextLevel = 0; nextLevel < blockLevel; ++nextLevel) {
		levelStartIndex += ( (blocksX * power2(nextLevel)) * (blocksY * power2(nextLevel))
			* (blocksZ * power2(nextLevel)) );
	}

	int32_t indexDiff = (int32_t) blockID - (int32_t) levelStartIndex;
	assert(indexDiff >= 0);

	const uint32_t blocksXLevel = blocksX * power2(blockLevel);
	const uint32_t blocksYLevel = blocksY * power2(blockLevel);
	const uint32_t blocksZLevel = blocksZ * power2(blockLevel);

	printf("Block location step 1: indexDiff = %" PRId32 ", blockID=%" PRIu32 "\n", indexDiff, blockID);
	printf("Blocks X=%" PRIu32 ", Y=%" PRIu32 ", Z=%" PRIu32 "\n", blocksX, blocksY, blocksZ);

	const uint32_t block_plane = indexDiff % (blocksXLevel * blocksYLevel);

	*posZ = indexDiff / (blocksXLevel * blocksYLevel);
	*posY = block_plane / blocksXLevel;

	const uint32_t block_remain = block_plane % blocksXLevel;

	*posX = (block_remain != 0) ? block_remain : 0;

	printf("Calculating location of blockID: %" PRIu32 ", at x=%" PRIu32 ", y=%" PRIu32 ", z=%" PRIu32 "\n",
		blockID, *posX, *posY, *posZ);
}

uint32_t Ember3DAMRGenerator::calcBlockID(const uint32_t posX, const uint32_t posY, const uint32_t posZ,
	const uint32_t level) {

	uint32_t startIndex = 0;

	for(uint32_t nextLevel = 0; nextLevel < level; ++nextLevel) {
		startIndex += ( (blocksX * power2(nextLevel)) * (blocksY * power2(nextLevel))
			* (blocksZ * power2(nextLevel)) );
	}

	const uint32_t blocksXLevel = blocksX * power2(level);
	const uint32_t blocksYLevel = blocksY * power2(level);
	const uint32_t blocksZLevel = blocksZ * power2(level);

	const uint32_t final_location = startIndex + posX + (posY * blocksXLevel) +
		(posZ * blocksXLevel * blocksYLevel);

	printf("Calculating location of %" PRIu32 ", %" PRIu32 ", %" PRIu32 " to be %" PRIu32 " (level=%" PRIu32 ")\n",
		posX, posY, posZ, final_location, level);

	return final_location;
}

Ember3DAMRGenerator::~Ember3DAMRGenerator() {

}

void Ember3DAMRGenerator::configureEnvironment(const SST::Output* output, uint32_t myRank, uint32_t world) {
	rank = myRank;
	worldSize = world;

	int32_t my_Z = 0;
	int32_t my_Y = 0;
	int32_t my_X = 0;

	getPosition(rank, peX, peY, peZ, &my_X, &my_Y, &my_Z);

	y_up    = convertPositionToRank(peX, peY, peZ, my_X, my_Y + 1, my_Z);
	y_down  = convertPositionToRank(peX, peY, peZ, my_X, my_Y - 1, my_Z);
	x_up    = convertPositionToRank(peX, peY, peZ, my_X + 1, my_Y, my_Z);
	x_down  = convertPositionToRank(peX, peY, peZ, my_X - 1, my_Y, my_Z);
	z_up    = convertPositionToRank(peX, peY, peZ, my_X, my_Y, my_Z + 1);
	z_down  = convertPositionToRank(peX, peY, peZ, my_X, my_Y, my_Z - 1);

	loadBlocks();
	free(blockFilePath);
}

bool Ember3DAMRGenerator::isBlockLocal(const uint32_t bID) const {
	bool found = false;

	for(uint32_t i = 0; i < localBlocks.size(); ++i) {
		if(localBlocks[i]->getBlockID() == bID) {
			found = true;
			break;
		}
	}

	return found;
}

void Ember3DAMRGenerator::generate(const SST::Output* output, const uint32_t phase, std::queue<EmberEvent*>* evQ) {
	exit(-1);
}

void Ember3DAMRGenerator::finish(const SST::Output* output) {

}
