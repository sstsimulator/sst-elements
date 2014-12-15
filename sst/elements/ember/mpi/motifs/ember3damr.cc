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

using namespace SST::Ember;
using namespace SST::Hermes::MP;

Ember3DAMRGenerator::Ember3DAMRGenerator(SST::Component* owner, Params& params) :
	EmberMessagePassingGenerator(owner, params)
{
	m_name = "3DAMR";

	int verbose = params.find_integer("verbose", 128);
	out = new Output("AMR3D [@p:@l]: ", verbose, 0, Output::STDOUT);

	// Get block sizes
	blockNx = (uint32_t) params.find_integer("arg.nx", 30);
	blockNy = (uint32_t) params.find_integer("arg.ny", 30);
	blockNz = (uint32_t) params.find_integer("arg.nz", 30);

	out->verbose(CALL_INFO, 2, 0, "Block sizes are X=%" PRIu32 ", Y=%" PRIu32 ", Z=%" PRIu32 "\n",
		blockNz, blockNy, blockNz);

	std::string blockpath = params.find_string("blockfile", "blocks.amr");

        blockFilePath = (char*) malloc(sizeof(char) * (blockpath.size() + 1));
        strcpy(blockFilePath, blockpath.c_str());
        blockFilePath[blockpath.size()] = '\0';

	out->verbose(CALL_INFO, 2, 0, "Block file to load mesh %s\n", blockFilePath);

	// Set the iteration count to zero, first loop
	iteration = 0;
	maxIterations = (uint32_t) params.find_integer("arg.iterations", 1);
	out->verbose(CALL_INFO, 2, 0, "Motif will run %" PRIu32 " iterations\n", maxIterations);

	// We are complete, let the user know
	out->verbose(CALL_INFO, 2, 0, "AMR Motif constructor completed.\n");
}

void Ember3DAMRGenerator::loadBlocks() {
	out->verbose(CALL_INFO, 2, 0, "Loading AMR block information from %s ...\n", blockFilePath);

	FILE* blockFile = fopen(blockFilePath, "rt");

	if(NULL == blockFile) {
		out->fatal(CALL_INFO, -1, "Unable to open AMR block file (%s)\n", blockFilePath);
	} else {
		out->verbose(CALL_INFO, 4, 0, "AMR block file opened successfully.\n");
	}

	const int header = fscanf(blockFile, "%" PRIu32 " %" PRIu32 " %" PRIu32 " %" PRIu32 " %" PRIu32 "\n",
		&blockCount, &maxLevel, &blocksX, &blocksY, &blocksZ);

	if( EOF == header ) {
		out->fatal(CALL_INFO, -1, "Unable to successfully read AMR header information from block file.\n");
	} else {
		out->verbose(CALL_INFO, 2, 0, "Loaded AMR block information: %" PRIu32 " blocks, %" PRIu32 " max refinement, blocks (X=%" PRIu32 ",Y=%" PRIu32 ",Z=%" PRIu32 ")\n",
			blockCount, maxLevel, blocksX, blocksY, blocksZ);
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

	for(uint32_t currentRank = 0; currentRank < size(); ++currentRank) {
		out->verbose(CALL_INFO, 4, 0, "Loading block information for rank %" PRIu32 " out of %" PRIu32 "... \n", currentRank, size());
		line++;

		if( EOF == fscanf(blockFile, "%" PRIu32 "\n", &blocksOnRank) ) {
			out->fatal(CALL_INFO, -1, "Unable to read blocks for rank %" PRIu32 " near line %" PRIu32 "\n",
				currentRank, line);
		} else {
			if(currentRank == rank) {
				for(uint32_t lineID = 0; lineID < blocksOnRank; ++lineID) {
					line++;

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

	out->verbose(CALL_INFO, 4, 0, "Performing AMR block wire up...\n");
	uint32_t maxRequests = 0;

	for(uint32_t i = 0; i < localBlocks.size(); ++i) {
		Ember3DAMRBlock* currentBlock = localBlocks[i];

		out->verbose(CALL_INFO, 8, 0, "Wiring block %" PRIu32 "...\n", currentBlock->getBlockID());

		const uint32_t blockLevel = currentBlock->getRefinementLevel();
		uint32_t blockXPos = 0;
		uint32_t blockYPos = 0;
		uint32_t blockZPos = 0;

		calcBlockLocation(currentBlock->getBlockID(), blockLevel, &blockXPos, &blockYPos, &blockZPos);

		out->verbose(CALL_INFO, 16, 0, "Block is refinement level %" PRIu32 ", located at (%" PRIu32 ", %" PRIu32 ", %" PRIu32 ")\n",
			blockLevel, blockXPos, blockYPos, blockZPos);

		// Patch up X-Up
		const uint32_t blockXUp = currentBlock->getRefineXUp();

		if(blockXUp == -2) {
			// Boundary condition, no communication
			currentBlock->setCommXUp(-1, -1, -1, -1);
		} else if(blockLevel > blockXUp) {
			// Communication to a coarser level (my refinement is higher than block next to me)
			const uint32_t commToBlock = calcBlockID((blockXPos / 2) + 1,
				blockYPos / 2, blockZPos / 2, blockXUp);

			std::map<uint32_t, uint32_t>::iterator blockNode = blockToNodeMap.find(commToBlock);

			if(blockNode == blockToNodeMap.end() && isBlockLocal(commToBlock)) {
				if( ! isBlockLocal(commToBlock) ) {
					out->fatal(CALL_INFO, -1, "Could not locate block %" PRIu32 ", during wire up phase.\n", commToBlock);
				}
			} else {
				// Projecting to coarse level
				currentBlock->setCommXUp(blockNode->second, -1, -1, -1);
				maxRequests++;
			}
		} else if(blockLevel < blockXUp) {
			// Communication to a finer level (my refinement is less than block next to me)
			const uint32_t x1 = calcBlockID(blockXPos * 2 + 2, blockYPos * 2,     blockZPos * 2,     blockXUp);
			const uint32_t x2 = calcBlockID(blockXPos * 2 + 2, blockYPos * 2 + 1, blockZPos * 2,     blockXUp);
			const uint32_t x3 = calcBlockID(blockXPos * 2 + 2, blockYPos * 2,     blockZPos * 2 + 1, blockXUp);
			const uint32_t x4 = calcBlockID(blockXPos * 2 + 2, blockYPos * 2 + 1, blockZPos * 2 + 1, blockXUp);

			std::map<uint32_t, uint32_t>::iterator blockNodeX1 = blockToNodeMap.find(x1);
			std::map<uint32_t, uint32_t>::iterator blockNodeX2 = blockToNodeMap.find(x2);
			std::map<uint32_t, uint32_t>::iterator blockNodeX3 = blockToNodeMap.find(x3);
			std::map<uint32_t, uint32_t>::iterator blockNodeX4 = blockToNodeMap.find(x4);

			if( blockNodeX1 == blockToNodeMap.end() && (! isBlockLocal(x1)) ) {
				out->fatal(CALL_INFO, -1, "X+ wireup for block failed to locate wire up partner (block: %" PRIu32 ")\n", x1);
			}

			if( blockNodeX2 == blockToNodeMap.end() && (! isBlockLocal(x2)) ) {
				out->fatal(CALL_INFO, -1, "X+ wireup for block failed to locate wire up partner (block: %" PRIu32 ")\n", x2);
			}

			if( blockNodeX3 == blockToNodeMap.end() && (! isBlockLocal(x3)) ) {
				out->fatal(CALL_INFO, -1, "X+ wireup for block failed to locate wire up partner (block: %" PRIu32 ")\n", x3);
			}

			if( blockNodeX4 == blockToNodeMap.end() && (! isBlockLocal(x4)) ) {
				out->fatal(CALL_INFO, -1, "X+ wireup for block failed to locate wire up partner (block: %" PRIu32 ")\n", x4);
			}

			currentBlock->setCommXUp(blockNodeX1->second,
				blockNodeX2->second,
				blockNodeX3->second,
				blockNodeX4->second);
			maxRequests += 4;
		} else {
			// Same level
			const uint32_t blockNextToMe = calcBlockID(blockXPos + 1,
				blockYPos, blockZPos, blockXUp);

			std::map<uint32_t, uint32_t>::iterator blockNextToMeNode = blockToNodeMap.find(blockNextToMe);

			if(blockNextToMeNode == blockToNodeMap.end()) {
				if( ! isBlockLocal(blockNextToMe) ) {
					out->fatal(CALL_INFO, -1, "X+ wireup for block failed to locate wire up on same refinement level (block=%" PRIu32 "\n",
						blockNextToMe);
				}
			} else {
				currentBlock->setCommXUp(blockNextToMeNode->second, -1, -1, -1);
				maxRequests++;
			}
		}

		//////////////////////////////////////////////////////////////////////////////////////////////////

		// Patch up X-Down
		const uint32_t blockXDown = currentBlock->getRefineXDown();

		if(blockXDown == -2) {
			// Boundary condition, no communication
			currentBlock->setCommXDown(-1, -1, -1, -1);
		} else if(blockLevel > blockXDown) {
			// Communication to a coarser level (my refinement is higher than block next to me)
			const uint32_t commToBlock = calcBlockID((blockXPos / 2) - 1,
				blockYPos / 2, blockZPos / 2, blockXDown);

			std::map<uint32_t, uint32_t>::iterator blockNode = blockToNodeMap.find(commToBlock);

			if(blockNode == blockToNodeMap.end() && isBlockLocal(commToBlock)) {
				if( ! isBlockLocal(commToBlock) ) {
					out->fatal(CALL_INFO, -1, "X- wireup for block failed to locate wire up partner (block: %" PRIu32 ")\n", commToBlock);
				}
			} else {
				// Projecting to coarse level
				currentBlock->setCommXDown(blockNode->second, -1, -1, -1);
				maxRequests++;
			}
		} else if(blockLevel < blockXDown) {
			// Communication to a finer level (my refinement is less than block next to me)
			const uint32_t x1 = calcBlockID(blockXPos * 2 - 1, blockYPos * 2,     blockZPos * 2,     blockXDown);
			const uint32_t x2 = calcBlockID(blockXPos * 2 - 1, blockYPos * 2 + 1, blockZPos * 2,     blockXDown);
			const uint32_t x3 = calcBlockID(blockXPos * 2 - 1, blockYPos * 2,     blockZPos * 2 + 1, blockXDown);
			const uint32_t x4 = calcBlockID(blockXPos * 2 - 1, blockYPos * 2 + 1, blockZPos * 2 + 1, blockXDown);

			std::map<uint32_t, uint32_t>::iterator blockNodeX1 = blockToNodeMap.find(x1);
			std::map<uint32_t, uint32_t>::iterator blockNodeX2 = blockToNodeMap.find(x2);
			std::map<uint32_t, uint32_t>::iterator blockNodeX3 = blockToNodeMap.find(x3);
			std::map<uint32_t, uint32_t>::iterator blockNodeX4 = blockToNodeMap.find(x4);

			if( blockNodeX1 == blockToNodeMap.end() && (! isBlockLocal(x1)) ) {
				out->fatal(CALL_INFO, -1, "X- wireup for block failed to locate wire up partner (block: %" PRIu32 ")\n", x1);
			}

			if( blockNodeX2 == blockToNodeMap.end() && (! isBlockLocal(x2)) ) {
				out->fatal(CALL_INFO, -1, "X- wireup for block failed to locate wire up partner (block: %" PRIu32 ")\n", x2);
			}

			if( blockNodeX3 == blockToNodeMap.end() && (! isBlockLocal(x3)) ) {
				out->fatal(CALL_INFO, -1, "X- wireup for block failed to locate wire up partner (block: %" PRIu32 ")\n", x3);
			}

			if( blockNodeX4 == blockToNodeMap.end() && (! isBlockLocal(x4)) ) {
				out->fatal(CALL_INFO, -1, "X- wireup for block failed to locate wire up partner (block: %" PRIu32 ")\n", x4);
			}

			currentBlock->setCommXDown(blockNodeX1->second,
				blockNodeX2->second,
				blockNodeX3->second,
				blockNodeX4->second);
			maxRequests += 4;
		} else {
			// Same level
			const uint32_t blockNextToMe = calcBlockID(blockXPos - 1,
				blockYPos, blockZPos, blockXDown);

			std::map<uint32_t, uint32_t>::iterator blockNextToMeNode = blockToNodeMap.find(blockNextToMe);

			if(blockNextToMeNode == blockToNodeMap.end()) {
				if( ! isBlockLocal(blockNextToMe) ) {
					out->fatal(CALL_INFO, -1, "X- wireup for block failed to locate wire up block on same refinment level (block: %" PRIu32 ")\n", blockNextToMe);
				}
			} else {
				currentBlock->setCommXDown(blockNextToMeNode->second, -1, -1, -1);
				maxRequests++;
			}
		}

        //////////////////////////////////////////////////////////////////////////////////////////////////
        // Patch up Y-Up
        const uint32_t blockYUp = currentBlock->getRefineYUp();

        if(blockYUp == -2) {
            // Boundary condition, no communication
            currentBlock->setCommYUp(-1, -1, -1, -1);
        } else if(blockLevel > blockYUp) {
            // Communication to a coarser level (my refinement is higher than block next to me)
            const uint32_t commToBlock = calcBlockID((blockXPos / 2),
                                                     (blockYPos / 2) + 1, blockZPos / 2, blockYUp);

            std::map<uint32_t, uint32_t>::iterator blockNode = blockToNodeMap.find(commToBlock);

            if(blockNode == blockToNodeMap.end() && isBlockLocal(commToBlock)) {
                if( ! isBlockLocal(commToBlock) ) {
                    printf("Y+ Did not locate block: %" PRIu32 "\n", commToBlock);
                    exit(-1);
                }
            } else {
                // Projecting to coarse level
                currentBlock->setCommYUp(blockNode->second, -1, -1, -1);
		maxRequests++;
            }
        } else if(blockLevel < blockYUp) {
            // Communication to a finer level (my refinement is less than block next to me)
            const uint32_t y1 = calcBlockID(blockXPos * 2,     blockYPos * 2 + 2, blockZPos * 2,     blockYUp);
            const uint32_t y2 = calcBlockID(blockXPos * 2 + 1, blockYPos * 2 + 2, blockZPos * 2,     blockYUp);
            const uint32_t y3 = calcBlockID(blockXPos * 2,     blockYPos * 2 + 2, blockZPos * 2 + 1, blockYUp);
            const uint32_t y4 = calcBlockID(blockXPos * 2 + 1, blockYPos * 2 + 2, blockZPos * 2 + 1, blockYUp);

            std::map<uint32_t, uint32_t>::iterator blockNodeY1 = blockToNodeMap.find(y1);
            std::map<uint32_t, uint32_t>::iterator blockNodeY2 = blockToNodeMap.find(y2);
            std::map<uint32_t, uint32_t>::iterator blockNodeY3 = blockToNodeMap.find(y3);
            std::map<uint32_t, uint32_t>::iterator blockNodeY4 = blockToNodeMap.find(y4);

            if( blockNodeY1 == blockToNodeMap.end() && (! isBlockLocal(y1)) ) {
		out->fatal(CALL_INFO, -1, "Y+ wireup for block failed to locate wire up partner (block: %" PRIu32 ")\n", y1);
            }

            if( blockNodeY2 == blockToNodeMap.end() && (! isBlockLocal(y2)) ) {
		out->fatal(CALL_INFO, -1, "Y+ wireup for block failed to locate wire up partner (block: %" PRIu32 ")\n", y2);
            }

            if( blockNodeY3 == blockToNodeMap.end() && (! isBlockLocal(y3)) ) {
		out->fatal(CALL_INFO, -1, "Y+ wireup for block failed to locate wire up partner (block: %" PRIu32 ")\n", y3);
            }

            if( blockNodeY4 == blockToNodeMap.end() && (! isBlockLocal(y4)) ) {
		out->fatal(CALL_INFO, -1, "Y+ wireup for block failed to locate wire up partner (block: %" PRIu32 ")\n", y4);
            }
            currentBlock->setCommYUp(blockNodeY1->second,
                                     blockNodeY2->second,
                                     blockNodeY3->second,
                                     blockNodeY4->second);
	    maxRequests += 4;
        } else {
            // Same level
            const uint32_t blockNextToMe = calcBlockID(blockXPos,
                                                       blockYPos + 1, blockZPos, blockYUp);
            std::map<uint32_t, uint32_t>::iterator blockNextToMeNode = blockToNodeMap.find(blockNextToMe);

            if(blockNextToMeNode == blockToNodeMap.end()) {
                if( ! isBlockLocal(blockNextToMe) ) {
		    out->fatal(CALL_INFO, -1, "Y+ wireup for block failed to locate wire up partner (block: %" PRIu32 ")\n", blockNextToMe);
                }
            } else {
                currentBlock->setCommYUp(blockNextToMeNode->second, -1, -1, -1);
		maxRequests++;
            }
        }

        //////////////////////////////////////////////////////////////////////////////////////////////////
        // Patch up Y-Down

        const uint32_t blockYDown = currentBlock->getRefineYDown();

        if(blockYDown == -2) {
            // Boundary condition, no communication
            currentBlock->setCommYDown(-1, -1, -1, -1);
        } else if(blockLevel > blockYDown) {
            // Communication to a coarser level (my refinement is higher than block next to me)
            const uint32_t commToBlock = calcBlockID((blockXPos / 2),
                                                     (blockYPos / 2) - 1, blockZPos / 2, blockYDown);

            std::map<uint32_t, uint32_t>::iterator blockNode = blockToNodeMap.find(commToBlock);

            if(blockNode == blockToNodeMap.end() && isBlockLocal(commToBlock)) {
                if( ! isBlockLocal(commToBlock) ) {
                    printf("Y- Did not locate block: %" PRIu32 "\n", commToBlock);
                    exit(-1);
                }
            } else {
                // Projecting to coarse level
                currentBlock->setCommYDown(blockNode->second, -1, -1, -1);
		maxRequests++;
            }
        } else if(blockLevel < blockYDown) {
            // Communication to a finer level (my refinement is less than block next to me)
            const uint32_t y1 = calcBlockID(blockXPos * 2,     blockYPos * 2 - 1, blockZPos * 2,     blockYDown);
            const uint32_t y2 = calcBlockID(blockXPos * 2 + 1, blockYPos * 2 - 1, blockZPos * 2,     blockYDown);
            const uint32_t y3 = calcBlockID(blockXPos * 2,     blockYPos * 2 - 1, blockZPos * 2 + 1, blockYDown);
            const uint32_t y4 = calcBlockID(blockXPos * 2 + 1, blockYPos * 2 - 1, blockZPos * 2 + 1, blockYDown);

            std::map<uint32_t, uint32_t>::iterator blockNodeY1 = blockToNodeMap.find(y1);
            std::map<uint32_t, uint32_t>::iterator blockNodeY2 = blockToNodeMap.find(y2);
            std::map<uint32_t, uint32_t>::iterator blockNodeY3 = blockToNodeMap.find(y3);
            std::map<uint32_t, uint32_t>::iterator blockNodeY4 = blockToNodeMap.find(y4);

            if( blockNodeY1 == blockToNodeMap.end() && (! isBlockLocal(y1)) ) {
                out->fatal(CALL_INFO, -1, "Y- wireup for block failed to locate wire up partner (block: %" PRIu32 ")\n", y1);
            }

            if( blockNodeY2 == blockToNodeMap.end() && (! isBlockLocal(y2)) ) {
                out->fatal(CALL_INFO, -1, "Y- wireup for block failed to locate wire up partner (block: %" PRIu32 ")\n", y2);
            }

            if( blockNodeY3 == blockToNodeMap.end() && (! isBlockLocal(y3)) ) {
                out->fatal(CALL_INFO, -1, "Y- wireup for block failed to locate wire up partner (block: %" PRIu32 ")\n", y3);
            }

            if( blockNodeY4 == blockToNodeMap.end() && (! isBlockLocal(y4)) ) {
                out->fatal(CALL_INFO, -1, "Y- wireup for block failed to locate wire up partner (block: %" PRIu32 ")\n", y4);
            }

            currentBlock->setCommYDown(blockNodeY1->second,
                                       blockNodeY2->second,
                                       blockNodeY3->second,
                                       blockNodeY4->second);
	    maxRequests += 4;
        } else {
            // Same level
            const uint32_t blockNextToMe = calcBlockID(blockXPos,
                                                       blockYPos - 1, blockZPos, blockYDown);

            std::map<uint32_t, uint32_t>::iterator blockNextToMeNode = blockToNodeMap.find(blockNextToMe);

            if(blockNextToMeNode == blockToNodeMap.end()) {
                if( ! isBlockLocal(blockNextToMe) ) {
                	out->fatal(CALL_INFO, -1, "Y- wireup for block failed to locate wire up partner (block: %" PRIu32 ")\n", blockNextToMe);
                }
            } else {
                currentBlock->setCommYDown(blockNextToMeNode->second, -1, -1, -1);
		maxRequests++;
            }
        }

	//////////////////////////////////////////////////////////////////////////////////////////////////
        // Patch up Z-Up

        const uint32_t blockZUp = currentBlock->getRefineZUp();

        if(blockZUp == -2) {
            // Boundary condition, no communication
            currentBlock->setCommZUp(-1, -1, -1, -1);
        } else if(blockLevel > blockZUp) {
            // Communication to a coarser level (my refinement is higher than block next to me)
            const uint32_t commToBlock = calcBlockID((blockXPos / 2),
                                                     (blockYPos / 2), (blockZPos / 2) + 1, blockZUp);

            std::map<uint32_t, uint32_t>::iterator blockNode = blockToNodeMap.find(commToBlock);
            
            if(blockNode == blockToNodeMap.end() && isBlockLocal(commToBlock)) {
                if( ! isBlockLocal(commToBlock) ) {
                    printf("Y+ Did not locate block: %" PRIu32 "\n", commToBlock);
                    exit(-1);
                }
            } else {
                // Projecting to coarse level
                currentBlock->setCommZUp(blockNode->second, -1, -1, -1);
		maxRequests++;
            }
        } else if(blockLevel < blockZUp) {
            // Communication to a finer level (my refinement is less than block next to me)
            const uint32_t z1 = calcBlockID(blockXPos * 2,     blockYPos * 2,     blockZPos * 2 + 2, blockZUp);
            const uint32_t z2 = calcBlockID(blockXPos * 2 + 1, blockYPos * 2,     blockZPos * 2 + 2, blockZUp);
            const uint32_t z3 = calcBlockID(blockXPos * 2,     blockYPos * 2 + 1, blockZPos * 2 + 2, blockZUp);
            const uint32_t z4 = calcBlockID(blockXPos * 2 + 1, blockYPos * 2 + 1, blockZPos * 2 + 2, blockZUp);

            std::map<uint32_t, uint32_t>::iterator blockNodeZ1 = blockToNodeMap.find(z1);
            std::map<uint32_t, uint32_t>::iterator blockNodeZ2 = blockToNodeMap.find(z2);
            std::map<uint32_t, uint32_t>::iterator blockNodeZ3 = blockToNodeMap.find(z3);
            std::map<uint32_t, uint32_t>::iterator blockNodeZ4 = blockToNodeMap.find(z4);

            if( blockNodeZ1 == blockToNodeMap.end() && (! isBlockLocal(z1)) ) {
                out->fatal(CALL_INFO, -1, "Z+ wireup for block failed to locate wire up partner (block: %" PRIu32 ")\n", z1);
            }

            if( blockNodeZ2 == blockToNodeMap.end() && (! isBlockLocal(z2)) ) {
                out->fatal(CALL_INFO, -1, "Z+ wireup for block failed to locate wire up partner (block: %" PRIu32 ")\n", z2);
            }

            if( blockNodeZ3 == blockToNodeMap.end() && (! isBlockLocal(z3)) ) {
                out->fatal(CALL_INFO, -1, "Z+ wireup for block failed to locate wire up partner (block: %" PRIu32 ")\n", z3);
            }

            if( blockNodeZ4 == blockToNodeMap.end() && (! isBlockLocal(z4)) ) {
                out->fatal(CALL_INFO, -1, "Z+ wireup for block failed to locate wire up partner (block: %" PRIu32 ")\n", z4);
            }
            currentBlock->setCommZUp(blockNodeZ1->second,
                                     blockNodeZ2->second,
                                     blockNodeZ3->second,
                                     blockNodeZ4->second);
	    maxRequests += 4;
        } else {
            // Same level
            const uint32_t blockNextToMe = calcBlockID(blockXPos,
                                                       blockYPos, blockZPos + 1, blockZUp);
            std::map<uint32_t, uint32_t>::iterator blockNextToMeNode = blockToNodeMap.find(blockNextToMe);

            if(blockNextToMeNode == blockToNodeMap.end()) {
                if( ! isBlockLocal(blockNextToMe) ) {
                    out->fatal(CALL_INFO, -1, "Z+ wireup for block failed to locate wire up partner (block: %" PRIu32 ")\n", blockNextToMe);
                }
            } else {
                currentBlock->setCommZUp(blockNextToMeNode->second, -1, -1, -1);
		maxRequests++;
            }
        }

        //////////////////////////////////////////////////////////////////////////////////////////////////
        // Patch up Z-Down
        const uint32_t blockZDown = currentBlock->getRefineZDown();

        if(blockZDown == -2) {
            // Boundary condition, no communication
            currentBlock->setCommZDown(-1, -1, -1, -1);
        } else if(blockLevel > blockZDown) {
            // Communication to a coarser level (my refinement is higher than block next to me)
            const uint32_t commToBlock = calcBlockID((blockXPos / 2),
                                                     (blockYPos / 2), (blockZPos / 2) - 1, blockZDown);

            std::map<uint32_t, uint32_t>::iterator blockNode = blockToNodeMap.find(commToBlock);

            if(blockNode == blockToNodeMap.end() && isBlockLocal(commToBlock)) {
                if( ! isBlockLocal(commToBlock) ) {
	                out->fatal(CALL_INFO, -1, "Z- wireup for block failed to locate wire up partner (block: %" PRIu32 ")\n", commToBlock);
                }
            } else {
                // Projecting to coarse level
                currentBlock->setCommZDown(blockNode->second, -1, -1, -1);
		maxRequests++;
            }
        } else if(blockLevel < blockZDown) {
            // Communication to a finer level (my refinement is less than block next to me
            const uint32_t z1 = calcBlockID(blockXPos * 2,     blockYPos * 2,     blockZPos * 2 - 1, blockZDown);
            const uint32_t z2 = calcBlockID(blockXPos * 2 + 1, blockYPos * 2,     blockZPos * 2 - 1, blockZDown);
            const uint32_t z3 = calcBlockID(blockXPos * 2,     blockYPos * 2 + 1, blockZPos * 2 - 1, blockZDown);
            const uint32_t z4 = calcBlockID(blockXPos * 2 + 1, blockYPos * 2 + 1, blockZPos * 2 - 1, blockZDown);

            std::map<uint32_t, uint32_t>::iterator blockNodeZ1 = blockToNodeMap.find(z1);
            std::map<uint32_t, uint32_t>::iterator blockNodeZ2 = blockToNodeMap.find(z2);
            std::map<uint32_t, uint32_t>::iterator blockNodeZ3 = blockToNodeMap.find(z3);
            std::map<uint32_t, uint32_t>::iterator blockNodeZ4 = blockToNodeMap.find(z4);

            if( blockNodeZ1 == blockToNodeMap.end() && (! isBlockLocal(z1)) ) {
                out->fatal(CALL_INFO, -1, "Z- wireup for block failed to locate wire up partner (block: %" PRIu32 ")\n", z1);
            }

            if( blockNodeZ2 == blockToNodeMap.end() && (! isBlockLocal(z2)) ) {
                out->fatal(CALL_INFO, -1, "Z- wireup for block failed to locate wire up partner (block: %" PRIu32 ")\n", z2);
            }

            if( blockNodeZ3 == blockToNodeMap.end() && (! isBlockLocal(z3)) ) {
                out->fatal(CALL_INFO, -1, "Z- wireup for block failed to locate wire up partner (block: %" PRIu32 ")\n", z3);
            }

            if( blockNodeZ4 == blockToNodeMap.end() && (! isBlockLocal(z4)) ) {
                out->fatal(CALL_INFO, -1, "Z- wireup for block failed to locate wire up partner (block: %" PRIu32 ")\n", z4);
            }

            currentBlock->setCommZDown(blockNodeZ1->second,
                                       blockNodeZ2->second,
                                       blockNodeZ3->second,
                                       blockNodeZ4->second);
	    maxRequests += 4;
        } else {
            // Same level
            const uint32_t blockNextToMe = calcBlockID(blockXPos,
                                                       blockYPos, blockZPos - 1, blockZDown);
            std::map<uint32_t, uint32_t>::iterator blockNextToMeNode = blockToNodeMap.find(blockNextToMe);

            if(blockNextToMeNode == blockToNodeMap.end()) {
                if( ! isBlockLocal(blockNextToMe) ) {
                    out->fatal(CALL_INFO, -1, "Z- wireup for block failed to locate wire up partner (block: %" PRIu32 ")\n", blockNextToMe);
                }
            } else {
                currentBlock->setCommZDown(blockNextToMeNode->second, -1, -1, -1);
		maxRequests++;
            }
        }
	}

	out->verbose(CALL_INFO, 2, 0, "Maximum requests from rank %" PRIu32 " will be set up: %" PRIu32 "\n", rank, maxRequests);
	requests   = (MessageRequest*)   malloc( sizeof(MessageRequest) * maxRequests * 2);
        responses  = (MessageResponse**) malloc( sizeof(MessageResponse*) * maxRequests * 2);

	uint32_t maxFaceDim = std::max(blockNx, std::max(blockNy, blockNz));
	blockMessageBuffer = memAlloc( sizeof(double) * maxFaceDim * maxFaceDim * localBlocks.size() * 2);

	printf("Blocks on rank %" PRIu32 " count=%lu\n", rank, localBlocks.size());
}

void Ember3DAMRGenerator::configure()
{
	out->verbose(CALL_INFO, 2, 0, "Configuring AMR motif...\n");

	loadBlocks();
	free(blockFilePath);

	out->verbose(CALL_INFO, 2, 0, "Motif configuration is complete.\n");
}

void Ember3DAMRGenerator::postBlockCommunication(std::queue<EmberEvent*>& evQ, int32_t* blockComm, uint32_t* nextReq, const uint32_t faceSize,
	const uint32_t msgTag) {

	const uint32_t maxFaceDim = std::max(blockNx, std::max(blockNy, blockNz));
	char* bufferPtr = (char*) blockMessageBuffer;

	for(uint32_t i = 0; i < 4; ++i) {
		if(blockComm[i] >= 0) {
			enQ_irecv( evQ, &bufferPtr[(*nextReq) * maxFaceDim * maxFaceDim],
				items_per_cell * faceSize, DOUBLE, blockComm[i], msgTag, GroupWorld, &requests[(*nextReq)]);
			(*nextReq) = (*nextReq) + 1;

			enQ_isend( evQ, &bufferPtr[(*nextReq) * maxFaceDim * maxFaceDim],
				items_per_cell * faceSize, DOUBLE, blockComm[i], msgTag, GroupWorld, &requests[(*nextReq)]);
			(*nextReq) = (*nextReq) + 1;
		}
	}
}

bool Ember3DAMRGenerator::generate( std::queue<EmberEvent*>& evQ)
{
	uint32_t nextReq = 0;

	if(iteration < maxIterations) {
		out->verbose(CALL_INFO, 8, 0, "Executing iteration: %" PRIu32 "\n", iteration);

		for(uint32_t i = 0; i < localBlocks.size(); ++i) {
			Ember3DAMRBlock* currentBlock = localBlocks[i];

			out->verbose(CALL_INFO, 16, 0, "Creating communication events for block %" PRIu32 "\n", currentBlock->getBlockID());

			out->verbose(CALL_INFO, 32, 0, "-> Processing X-Down direction...\n");
			postBlockCommunication(evQ, currentBlock->getCommXDown(), &nextReq, blockNy * blockNz, 1001);

			out->verbose(CALL_INFO, 32, 0, "-> Processing X-Down direction...\n");
			postBlockCommunication(evQ, currentBlock->getCommXUp(),   &nextReq, blockNy * blockNz, 2001);

			out->verbose(CALL_INFO, 32, 0, "-> Processing Y-Down direction...\n");
			postBlockCommunication(evQ, currentBlock->getCommYDown(), &nextReq, blockNx * blockNz, 3001);

			out->verbose(CALL_INFO, 32, 0, "-> Processing Y-Up direction...\n");
			postBlockCommunication(evQ, currentBlock->getCommYUp(),   &nextReq, blockNx * blockNz, 4001);

			out->verbose(CALL_INFO, 32, 0, "-> Processing Z-Down direction...\n");
			postBlockCommunication(evQ, currentBlock->getCommZDown(), &nextReq, blockNx * blockNy, 5001);

			out->verbose(CALL_INFO, 32, 0, "-> Processing Z-Up direction...\n");
			postBlockCommunication(evQ, currentBlock->getCommZUp(),   &nextReq, blockNx * blockNy, 6001);

			out->verbose(CALL_INFO, 16, 0, "Block %" PRIu32 " complete.\n", currentBlock->getBlockID());
		}

		enQ_waitall( evQ, nextReq, &requests[0], &responses[0] );

		iteration++;
		return false;
	} else {
		out->verbose(CALL_INFO, 2, 0, "Completed %" PRIu32 " iterations, will now complete and unload.\n", iteration);
		return true;
	}
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

void Ember3DAMRGenerator::printBlockMap() {
	std::map<uint32_t, uint32_t>::iterator block_itr;

	for(block_itr = blockToNodeMap.begin(); block_itr != blockToNodeMap.end(); block_itr++) {
		printf("Block %" PRIu32 " maps to node: %" PRIu32 "\n",
			block_itr->first, block_itr->second);
	}
}

Ember3DAMRGenerator::~Ember3DAMRGenerator() {
	delete out;
	memFree(blockMessageBuffer);
}

