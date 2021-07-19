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

#include <sst_config.h>

#include <climits>

#include "ember3damr.h"
#include "ember3damrfile.h"
#include "ember3damrtextfile.h"
#include "ember3damrbinaryfile.h"

using namespace SST::Ember;
using namespace SST::Hermes::MP;

static std::map<uint32_t, int32_t>  blockToNodeMap;

Ember3DAMRGenerator::Ember3DAMRGenerator(SST::ComponentId_t id, Params& params) :
	EmberMessagePassingGenerator(id, params, "3DAMR")
{
	int verbose = params.find("arg.verbose", 0);
	out = new Output("AMR3D [@p:@l]: ", verbose, 0, Output::STDOUT);

	// Get block sizes
	blockNx = (uint32_t) params.find("arg.nx", 8);
	blockNy = (uint32_t) params.find("arg.ny", 8);
	blockNz = (uint32_t) params.find("arg.nz", 8);

	items_per_cell = (uint32_t) params.find("arg.fieldspercell", 1);

	out->verbose(CALL_INFO, 2, 0, "Block sizes are X=%" PRIu32 ", Y=%" PRIu32 ", Z=%" PRIu32 "\n",
		blockNz, blockNy, blockNz);

	std::string blockpath = params.find<std::string>("arg.blockfile", "blocks.amr");

        blockFilePath = (char*) malloc(sizeof(char) * (blockpath.size() + 1));
        strcpy(blockFilePath, blockpath.c_str());
        blockFilePath[blockpath.size()] = '\0';

	out->verbose(CALL_INFO, 2, 0, "Block file to load mesh %s\n", blockFilePath);

        if("binary" == params.find<std::string>("arg.filetype") ) {
        	meshType = 2;
        } else {
        	meshType = 1;
    	}

	printMaps = params.find<std::string>("arg.printmap", "no") == "yes";
	if(printMaps) {
		out->verbose(CALL_INFO, 16, 0, "Configured to print rank to block maps\n");
	} else {
		out->verbose(CALL_INFO, 16, 0, "Configured not to print rank to block mapping.\n");
	}

	// Set the iteration count to zero, first loop
	iteration = 0;
	nextBlockToBeProcessed = 0;
	nextRequestID = 0;

	maxIterations = (uint32_t) params.find("arg.iterations", 1);
	out->verbose(CALL_INFO, 2, 0, "Motif will run %" PRIu32 " iterations\n", maxIterations);

	// We are complete, let the user know
	out->verbose(CALL_INFO, 2, 0, "AMR Motif constructor completed.\n");
	configure();
}

void Ember3DAMRGenerator::loadBlocks() {
	out->verbose(CALL_INFO, 2, 0, "Loading AMR block information from %s ...\n", blockFilePath);

    EmberAMRBinaryFile* amrFile = NULL;

    if(2 == meshType) {
        amrFile = new EmberAMRBinaryFile(blockFilePath, out);

	if(blockToNodeMap.empty()) {
		amrFile->populateGlobalBlocks(&blockToNodeMap);
	}
    } else {
//        amrFile = new EmberAMRTextFile(blockFilePath, out);
	out->fatal(CALL_INFO, -1, "Binary mesh files are the only type currently supported, use sst-meshconvert\n");
    }

	maxLevel   = amrFile->getMaxRefinement();
	blockCount = amrFile->getBlockCount();
	blocksX    = amrFile->getBlocksX();
	blocksY    = amrFile->getBlocksY();
	blocksZ    = amrFile->getBlocksZ();

	out->verbose(CALL_INFO, 2, 0, "Loaded AMR block information: %" PRIu32 " blocks, %" PRIu32 " max refinement, blocks (X=%" PRIu32 ",Y=%" PRIu32 ",Z=%" PRIu32 ")\n",
		blockCount, maxLevel, blocksX, blocksY, blocksZ);

//	uint32_t blocksOnRank = 0;
//	uint32_t blockID = 0;
//	uint32_t  blockLevel = 0;
//	int32_t  xUp = 0;
//	int32_t  xDown = 0;
//	int32_t  yUp = 0;
//	int32_t  yDown = 0;
//	int32_t  zUp = 0;
//	int32_t  zDown = 0;

	uint32_t line = 0;


	/*
	for(int32_t currentRank = 0; currentRank < size(); ++currentRank) {
		out->verbose(CALL_INFO, 4, 0, "Loading block information for rank %" PRIu32 " out of %" PRIu32 "... \n", currentRank, size());
		line++;

		int otherRankBlocks = 0;

		amrFile->readNodeMeshLine(&blocksOnRank);
		out->verbose(CALL_INFO, 16, 0, "Processing node %" PRIu32 ", node header says there are %" PRIu32 " blocks on this node.\n",
			currentRank, blocksOnRank);

		if(currentRank == rank()) {
			for(uint32_t lineID = 0; lineID < blocksOnRank; ++lineID) {
				line++;

				amrFile->readNextMeshLine(&blockID, &blockLevel, &xDown, &xUp, &yDown, &yUp, &zDown, &zUp);
				out->verbose(CALL_INFO, 32, 0, "Read mesh block: %" PRIu32 " level=%" PRIu32 ", (%" PRId32 ",%" PRId32 ",%" PRId32 ",%" PRId32 ",%" PRId32 ",%" PRId32 ")\n",
					blockID, blockLevel, xDown, xUp, yDown, yUp, zDown, zUp);

				localBlocks.push_back( new Ember3DAMRBlock(blockID, blockLevel, xUp, xDown, yUp, yDown, zUp, zDown) );
			}
		} else {
			for(uint32_t lineID = 0; lineID < blocksOnRank; ++lineID) {
				line++;

				amrFile->readNextMeshLine(&blockID, &blockLevel, &xDown, &xUp, &yDown, &yUp, &zDown, &zUp);
				out->verbose(CALL_INFO, 32, 0, "Read mesh block: %" PRIu32 " level=%" PRIu32 ", (%" PRId32 ",%" PRId32 ",%" PRId32 ",%" PRId32 ",%" PRId32 ",%" PRId32 ")\n",
					blockID, blockLevel, xDown, xUp, yDown, yUp, zDown, zUp);

				std::map<uint32_t, int32_t>::iterator checkExists = blockToNodeMap.find(blockID);

				if(checkExists != blockToNodeMap.end()) {
					out->fatal(CALL_INFO, -1, "Read in block %" PRIu32 " but that block already exists and points to rank %" PRId32 " (processing rank: %" PRIu32 ")\n",
						blockID, checkExists->second, currentRank);
				}

				blockToNodeMap.insert( std::pair<uint32_t, int32_t>(blockID, (int32_t) currentRank) );
				otherRankBlocks++;
			}
		}

		out->verbose(CALL_INFO, 4, 0, "Rank %" PRIu32 " loaded %d for rank %" PRIu32 "\n",
			rank(), otherRankBlocks, currentRank);
	}*/
	amrFile->populateLocalBlocks(&localBlocks , rank());

	out->verbose(CALL_INFO, 2, 0, "Rank %" PRIu32 ", loaded %" PRIu32 " blocks locally and %" PRIu32 " remotely, stopped at line: %" PRIu32 ".\n", (uint32_t) rank(),
		(uint32_t) localBlocks.size(), (uint32_t) blockToNodeMap.size(), line);

	out->verbose(CALL_INFO, 4, 0, "Performing AMR block wire up...\n");
	uint32_t maxRequests = 0;

	// Print out the block map to file if we are running in verbose mode.
//	if(out->getVerboseLevel() >= 8) {
	if(printMaps) {
		printBlockMap();
	}
//	}

	for(uint32_t i = 0; i < localBlocks.size(); ++i) {
		Ember3DAMRBlock* currentBlock = localBlocks[i];

		out->verbose(CALL_INFO, 16, 0, "Wiring block %" PRIu32 "...\n", currentBlock->getBlockID());

		const int32_t blockLevel = currentBlock->getRefinementLevel();
		uint32_t blockXPos = 0;
		uint32_t blockYPos = 0;
		uint32_t blockZPos = 0;

		calcBlockLocation(currentBlock->getBlockID(), blockLevel, &blockXPos, &blockYPos, &blockZPos);

		out->verbose(CALL_INFO, 16, 0, "Block is refinement level %" PRIu32 ", located at (%" PRIu32 ", %" PRIu32 ", %" PRIu32 ")\n",
			blockLevel, blockXPos, blockYPos, blockZPos);

		// Patch up X-Up
		const int32_t blockXUp = currentBlock->getRefineXUp();

		out->verbose(CALL_INFO, 16, 0, "Block %" PRIu32 " has X+:%" PRId32 "\n", currentBlock->getBlockID(), blockXUp);

		if(blockXUp == -2) {
			out->verbose(CALL_INFO, 16, 0, "BLOCK IS -2\n");
			// Boundary condition, no communication
			currentBlock->setCommXUp(-1, -1, -1, -1);
		} else if(blockLevel > blockXUp) {
			out->verbose(CALL_INFO, 16, 0, "BLOCK IS NOT -2, WILL BE PROCESSED\n");
			// Communication to a coarser level (my refinement is higher than block next to me)
			const uint32_t commToBlock = calcBlockID((blockXPos / 2) + 1,
				blockYPos / 2, blockZPos / 2, blockXUp);

			std::map<uint32_t, int32_t>::iterator blockNode = blockToNodeMap.find(commToBlock);

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

			std::map<uint32_t, int32_t>::iterator blockNodeX1 = blockToNodeMap.find(x1);
			std::map<uint32_t, int32_t>::iterator blockNodeX2 = blockToNodeMap.find(x2);
			std::map<uint32_t, int32_t>::iterator blockNodeX3 = blockToNodeMap.find(x3);
			std::map<uint32_t, int32_t>::iterator blockNodeX4 = blockToNodeMap.find(x4);

			int32_t rankX1 = blockNodeX1->second;
			int32_t rankX2 = blockNodeX2->second;
			int32_t rankX3 = blockNodeX3->second;
			int32_t rankX4 = blockNodeX4->second;

			if( blockNodeX1 == blockToNodeMap.end() ) {
				if( isBlockLocal(x1) ) {
					rankX1 = -1;
				} else {
					out->fatal(CALL_INFO, -1, "X+ wireup for block failed to locate wire up partner (block: %" PRIu32 ")\n", x1);
				}
			}

			if( blockNodeX2 == blockToNodeMap.end() ) {
				if( isBlockLocal(x2) ) {
					rankX2 = -1;
				} else {
					out->fatal(CALL_INFO, -1, "X+ wireup for block failed to locate wire up partner (block: %" PRIu32 ")\n", x2);
				}
			}

			if( blockNodeX3 == blockToNodeMap.end() ) {
				if( isBlockLocal(x3) ) {
					rankX3 = -1;
				} else {
					out->fatal(CALL_INFO, -1, "X+ wireup for block failed to locate wire up partner (block: %" PRIu32 ")\n", x3);
				}
			}

			if( blockNodeX4 == blockToNodeMap.end() ) {
				if( isBlockLocal(x4) ) {
					rankX4 = -1;
				} else {
					out->fatal(CALL_INFO, -1, "X+ wireup for block failed to locate wire up partner (block: %" PRIu32 ")\n", x4);
				}
			}

		        out->verbose(CALL_INFO, 32, 0, "Mapping block %" PRIu32 " to nodes %" PRId32 ", %" PRId32 ", %" PRId32 ", %" PRId32 " in X+\n",
				currentBlock->getBlockID(), rankX1, rankX2, rankX3, rankX4);

			currentBlock->setCommXUp(rankX1, rankX2, rankX3, rankX4);

			maxRequests += 4;
		} else {
			// Same level
			const uint32_t blockNextToMe = calcBlockID(blockXPos + 1,
				blockYPos, blockZPos, blockXUp);

			std::map<uint32_t, int32_t>::iterator blockNextToMeNode = blockToNodeMap.find(blockNextToMe);

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
		const int32_t blockXDown = currentBlock->getRefineXDown();

		out->verbose(CALL_INFO, 16, 0, "Block %" PRIu32 " has X-:%" PRId32 "\n", currentBlock->getBlockID(), blockXDown);

		if(blockXDown == -2) {
			// Boundary condition, no communication
			currentBlock->setCommXDown(-1, -1, -1, -1);
		} else if(blockLevel > blockXDown) {
			out->verbose(CALL_INFO, 16, 0, "Block %" PRIu32 " calculating X down, (%" PRIu32 "/2-1=%d" PRId32 ", %" PRIu32 "/2=%d" PRId32 ", %" PRIu32 "/2=%" PRId32 "\n",
				currentBlock->getBlockID(), blockXPos,
				(int32_t) (blockXPos/2)-1,
				blockYPos, blockYPos/2,
				blockZPos, blockZPos/2);

			// Communication to a coarser level (my refinement is higher than block next to me)
			const uint32_t commToBlock = calcBlockID((blockXPos / 2) - 1,
				blockYPos / 2, blockZPos / 2, blockXDown);

			std::map<uint32_t, int32_t>::iterator blockNode = blockToNodeMap.find(commToBlock);

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
			out->verbose(CALL_INFO, 16, 0, "Block %" PRIu32 " calculating X down with refinement, (%" PRIu32 "/2-1=%d" PRId32 ", %" PRIu32 "/2=%d" PRId32 ", %" PRIu32 "/2=%" PRId32 "\n",
				currentBlock->getBlockID(), blockXPos,
				(int32_t) (blockXPos*22)-1,
				blockYPos, blockYPos*22,
				blockZPos, blockZPos*22);

			// Communication to a finer level (my refinement is less than block next to me)
			const uint32_t x1 = calcBlockID(blockXPos * 2 - 1, blockYPos * 2,     blockZPos * 2,     blockXDown);
			const uint32_t x2 = calcBlockID(blockXPos * 2 - 1, blockYPos * 2 + 1, blockZPos * 2,     blockXDown);
			const uint32_t x3 = calcBlockID(blockXPos * 2 - 1, blockYPos * 2,     blockZPos * 2 + 1, blockXDown);
			const uint32_t x4 = calcBlockID(blockXPos * 2 - 1, blockYPos * 2 + 1, blockZPos * 2 + 1, blockXDown);

			std::map<uint32_t, int32_t>::iterator blockNodeX1 = blockToNodeMap.find(x1);
			std::map<uint32_t, int32_t>::iterator blockNodeX2 = blockToNodeMap.find(x2);
			std::map<uint32_t, int32_t>::iterator blockNodeX3 = blockToNodeMap.find(x3);
			std::map<uint32_t, int32_t>::iterator blockNodeX4 = blockToNodeMap.find(x4);

			int32_t rankX1 = blockNodeX1->second;
			int32_t rankX2 = blockNodeX2->second;
			int32_t rankX3 = blockNodeX3->second;
			int32_t rankX4 = blockNodeX4->second;

			if( blockNodeX1 == blockToNodeMap.end() ) {
				if( isBlockLocal(x1) ) {
					rankX1 = -1;
				} else {
					out->fatal(CALL_INFO, -1, "X- wireup for block failed to locate wire up partner (block: %" PRIu32 ")\n", x1);
				}
			}

			if( blockNodeX2 == blockToNodeMap.end() ) {
				if( isBlockLocal(x2) ) {
					rankX2 = -1;
				} else {
					out->fatal(CALL_INFO, -1, "X- wireup for block failed to locate wire up partner (block: %" PRIu32 ")\n", x2);
				}
			}

			if( blockNodeX3 == blockToNodeMap.end() ) {
				if( isBlockLocal(x3) ) {
					rankX3 = -1;
				} else {
					out->fatal(CALL_INFO, -1, "X- wireup for block failed to locate wire up partner (block: %" PRIu32 ")\n", x3);
				}
			}

			if( blockNodeX4 == blockToNodeMap.end() ) {
				if( isBlockLocal(x4) ) {
					rankX4 = -1;
				} else {
					out->fatal(CALL_INFO, -1, "X- wireup for block failed to locate wire up partner (block: %" PRIu32 ")\n", x4);
				}
			}

		        out->verbose(CALL_INFO, 32, 0, "Mapping block %" PRIu32 " to nodes %" PRId32 ", %" PRId32 ", %" PRId32 ", %" PRId32 " in X-\n",
				currentBlock->getBlockID(), rankX1, rankX2, rankX3, rankX4);

			currentBlock->setCommXDown(rankX1, rankX2, rankX3, rankX4);

			maxRequests += 4;
		} else {
			// Same level
			const uint32_t blockNextToMe = calcBlockID(blockXPos - 1,
				blockYPos, blockZPos, blockXDown);

			std::map<uint32_t, int32_t>::iterator blockNextToMeNode = blockToNodeMap.find(blockNextToMe);

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
        const int32_t blockYUp = currentBlock->getRefineYUp();

        if(blockYUp == -2) {
            // Boundary condition, no communication
            currentBlock->setCommYUp(-1, -1, -1, -1);
        } else if(blockLevel > blockYUp) {
            // Communication to a coarser level (my refinement is higher than block next to me)
            const uint32_t commToBlock = calcBlockID((blockXPos / 2),
                                                     (blockYPos / 2) + 1, blockZPos / 2, blockYUp);

            std::map<uint32_t, int32_t>::iterator blockNode = blockToNodeMap.find(commToBlock);

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

            std::map<uint32_t, int32_t>::iterator blockNodeY1 = blockToNodeMap.find(y1);
            std::map<uint32_t, int32_t>::iterator blockNodeY2 = blockToNodeMap.find(y2);
            std::map<uint32_t, int32_t>::iterator blockNodeY3 = blockToNodeMap.find(y3);
            std::map<uint32_t, int32_t>::iterator blockNodeY4 = blockToNodeMap.find(y4);

			int32_t rankY1 = blockNodeY1->second;
			int32_t rankY2 = blockNodeY2->second;
			int32_t rankY3 = blockNodeY3->second;
			int32_t rankY4 = blockNodeY4->second;

			if( blockNodeY1 == blockToNodeMap.end() ) {
				if( isBlockLocal(y1) ) {
					rankY1 = -1;
				} else {
					out->fatal(CALL_INFO, -1, "Y+ wireup for block failed to locate wire up partner (block: %" PRIu32 ")\n", y1);
				}
			}

			if( blockNodeY2 == blockToNodeMap.end() ) {
				if( isBlockLocal(y2) ) {
					rankY2 = -1;
				} else {
					out->fatal(CALL_INFO, -1, "Y+ wireup for block failed to locate wire up partner (block: %" PRIu32 ")\n", y2);
				}
			}

			if( blockNodeY3 == blockToNodeMap.end() ) {
				if( isBlockLocal(y3) ) {
					rankY3 = -1;
				} else {
					out->fatal(CALL_INFO, -1, "Y+ wireup for block failed to locate wire up partner (block: %" PRIu32 ")\n", y3);
				}
			}

			if( blockNodeY4 == blockToNodeMap.end() ) {
				if( isBlockLocal(y4) ) {
					rankY4 = -1;
				} else {
					out->fatal(CALL_INFO, -1, "Y+ wireup for block failed to locate wire up partner (block: %" PRIu32 ")\n", y4);
				}
			}

		        out->verbose(CALL_INFO, 32, 0, "Mapping block %" PRIu32 " to nodes %" PRId32 ", %" PRId32 ", %" PRId32 ", %" PRId32 " in Y+\n",
				currentBlock->getBlockID(), rankY1, rankY2, rankY3, rankY4);

			currentBlock->setCommYUp(rankY1, rankY2, rankY3, rankY4);

	    maxRequests += 4;
        } else {
            // Same level
            const uint32_t blockNextToMe = calcBlockID(blockXPos,
                                                       blockYPos + 1, blockZPos, blockYUp);
            std::map<uint32_t, int32_t>::iterator blockNextToMeNode = blockToNodeMap.find(blockNextToMe);

            if(blockNextToMeNode == blockToNodeMap.end()) {
                if( ! isBlockLocal(blockNextToMe) ) {
		    out->output("Dumping block map for rank: %" PRIu32 "\n", rank());
		    out->fatal(CALL_INFO, -1, "Y+ wireup for block failed to locate wire up partner (block: %" PRIu32 ")\n", blockNextToMe);
                }
            } else {
                currentBlock->setCommYUp(blockNextToMeNode->second, -1, -1, -1);
		maxRequests++;
            }
        }

        //////////////////////////////////////////////////////////////////////////////////////////////////
        // Patch up Y-Down

        const int32_t blockYDown = currentBlock->getRefineYDown();

        if(blockYDown == -2) {
            // Boundary condition, no communication
            currentBlock->setCommYDown(-1, -1, -1, -1);
        } else if(blockLevel > blockYDown) {
            // Communication to a coarser level (my refinement is higher than block next to me)
            const uint32_t commToBlock = calcBlockID((blockXPos / 2),
                                                     (blockYPos / 2) - 1, blockZPos / 2, blockYDown);

            std::map<uint32_t, int32_t>::iterator blockNode = blockToNodeMap.find(commToBlock);

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

            std::map<uint32_t, int32_t>::iterator blockNodeY1 = blockToNodeMap.find(y1);
            std::map<uint32_t, int32_t>::iterator blockNodeY2 = blockToNodeMap.find(y2);
            std::map<uint32_t, int32_t>::iterator blockNodeY3 = blockToNodeMap.find(y3);
            std::map<uint32_t, int32_t>::iterator blockNodeY4 = blockToNodeMap.find(y4);

			int32_t rankY1 = blockNodeY1->second;
			int32_t rankY2 = blockNodeY2->second;
			int32_t rankY3 = blockNodeY3->second;
			int32_t rankY4 = blockNodeY4->second;

			if( blockNodeY1 == blockToNodeMap.end() ) {
				if( isBlockLocal(y1) ) {
					rankY1 = -1;
				} else {
					out->fatal(CALL_INFO, -1, "Y- wireup for block failed to locate wire up partner (block: %" PRIu32 ")\n", y1);
				}
			}

			if( blockNodeY2 == blockToNodeMap.end() ) {
				if( isBlockLocal(y2) ) {
					rankY2 = -1;
				} else {
					out->fatal(CALL_INFO, -1, "Y- wireup for block failed to locate wire up partner (block: %" PRIu32 ")\n", y2);
				}
			}

			if( blockNodeY3 == blockToNodeMap.end() ) {
				if( isBlockLocal(y3) ) {
					rankY3 = -1;
				} else {
					out->fatal(CALL_INFO, -1, "Y- wireup for block failed to locate wire up partner (block: %" PRIu32 ")\n", y3);
				}
			}

			if( blockNodeY4 == blockToNodeMap.end() ) {
				if( isBlockLocal(y4) ) {
					rankY4 = -1;
				} else {
					out->fatal(CALL_INFO, -1, "Y- wireup for block failed to locate wire up partner (block: %" PRIu32 ")\n", y4);
				}
			}

		        out->verbose(CALL_INFO, 32, 0, "Mapping block %" PRIu32 " to nodes %" PRId32 ", %" PRId32 ", %" PRId32 ", %" PRId32 " in Y-\n",
				currentBlock->getBlockID(), rankY1, rankY2, rankY3, rankY4);

			currentBlock->setCommYDown(rankY1, rankY2, rankY3, rankY4);
	    maxRequests += 4;
        } else {
            // Same level
            const uint32_t blockNextToMe = calcBlockID(blockXPos,
                                                       blockYPos - 1, blockZPos, blockYDown);

            std::map<uint32_t, int32_t>::iterator blockNextToMeNode = blockToNodeMap.find(blockNextToMe);

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

        const int32_t blockZUp = currentBlock->getRefineZUp();

        if(blockZUp == -2) {
            // Boundary condition, no communication
            currentBlock->setCommZUp(-1, -1, -1, -1);
        } else if(blockLevel > blockZUp) {
            // Communication to a coarser level (my refinement is higher than block next to me)
            const uint32_t commToBlock = calcBlockID((blockXPos / 2),
                                                     (blockYPos / 2), (blockZPos / 2) + 1, blockZUp);

            std::map<uint32_t, int32_t>::iterator blockNode = blockToNodeMap.find(commToBlock);

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

            std::map<uint32_t, int32_t>::iterator blockNodeZ1 = blockToNodeMap.find(z1);
            std::map<uint32_t, int32_t>::iterator blockNodeZ2 = blockToNodeMap.find(z2);
            std::map<uint32_t, int32_t>::iterator blockNodeZ3 = blockToNodeMap.find(z3);
            std::map<uint32_t, int32_t>::iterator blockNodeZ4 = blockToNodeMap.find(z4);

			int32_t rankZ1 = blockNodeZ1->second;
			int32_t rankZ2 = blockNodeZ2->second;
			int32_t rankZ3 = blockNodeZ3->second;
			int32_t rankZ4 = blockNodeZ4->second;

			if( blockNodeZ1 == blockToNodeMap.end() ) {
				if( isBlockLocal(z1) ) {
					rankZ1 = -1;
				} else {
					out->fatal(CALL_INFO, -1, "Z+ wireup for block failed to locate wire up partner (block: %" PRIu32 ")\n", z1);
				}
			}

			if( blockNodeZ2 == blockToNodeMap.end() ) {
				if( isBlockLocal(z2) ) {
					rankZ2 = -1;
				} else {
					out->fatal(CALL_INFO, -1, "Z+ wireup for block failed to locate wire up partner (block: %" PRIu32 ")\n", z2);
				}
			}

			if( blockNodeZ3 == blockToNodeMap.end() ) {
				if( isBlockLocal(z3) ) {
					rankZ3 = -1;
				} else {
					out->fatal(CALL_INFO, -1, "Z+ wireup for block failed to locate wire up partner (block: %" PRIu32 ")\n", z3);
				}
			}

			if( blockNodeZ4 == blockToNodeMap.end() ) {
				if( isBlockLocal(z4) ) {
					rankZ4 = -1;
				} else {
					out->fatal(CALL_INFO, -1, "Z+ wireup for block failed to locate wire up partner (block: %" PRIu32 ")\n", z4);
				}
			}

		        out->verbose(CALL_INFO, 32, 0, "Mapping block %" PRIu32 " to nodes %" PRId32 ", %" PRId32 ", %" PRId32 ", %" PRId32 " in Y-\n",
				currentBlock->getBlockID(), rankZ1, rankZ2, rankZ3, rankZ4);

			currentBlock->setCommZUp(rankZ1, rankZ2, rankZ3, rankZ4);
	    maxRequests += 4;
        } else {
            // Same level
            const uint32_t blockNextToMe = calcBlockID(blockXPos,
                                                       blockYPos, blockZPos + 1, blockZUp);
            std::map<uint32_t, int32_t>::iterator blockNextToMeNode = blockToNodeMap.find(blockNextToMe);

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
        const int32_t blockZDown = currentBlock->getRefineZDown();

        if(blockZDown == -2) {
            // Boundary condition, no communication
            currentBlock->setCommZDown(-1, -1, -1, -1);
        } else if(blockLevel > blockZDown) {
            // Communication to a coarser level (my refinement is higher than block next to me)
            const uint32_t commToBlock = calcBlockID((blockXPos / 2),
                                                     (blockYPos / 2), (blockZPos / 2) - 1, blockZDown);

            std::map<uint32_t, int32_t>::iterator blockNode = blockToNodeMap.find(commToBlock);

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

            std::map<uint32_t, int32_t>::iterator blockNodeZ1 = blockToNodeMap.find(z1);
            std::map<uint32_t, int32_t>::iterator blockNodeZ2 = blockToNodeMap.find(z2);
            std::map<uint32_t, int32_t>::iterator blockNodeZ3 = blockToNodeMap.find(z3);
            std::map<uint32_t, int32_t>::iterator blockNodeZ4 = blockToNodeMap.find(z4);

			int32_t rankZ1 = blockNodeZ1->second;
			int32_t rankZ2 = blockNodeZ2->second;
			int32_t rankZ3 = blockNodeZ3->second;
			int32_t rankZ4 = blockNodeZ4->second;

			if( blockNodeZ1 == blockToNodeMap.end() ) {
				if( isBlockLocal(z1) ) {
					rankZ1 = -1;
				} else {
					out->fatal(CALL_INFO, -1, "Z- wireup for block failed to locate wire up partner (block: %" PRIu32 ")\n", z1);
				}
			}

			if( blockNodeZ2 == blockToNodeMap.end() ) {
				if( isBlockLocal(z2) ) {
					rankZ2 = -1;
				} else {
					out->fatal(CALL_INFO, -1, "Z- wireup for block failed to locate wire up partner (block: %" PRIu32 ")\n", z2);
				}
			}

			if( blockNodeZ3 == blockToNodeMap.end() ) {
				if( isBlockLocal(z3) ) {
					rankZ3 = -1;
				} else {
					out->fatal(CALL_INFO, -1, "Z- wireup for block failed to locate wire up partner (block: %" PRIu32 ")\n", z3);
				}
			}

			if( blockNodeZ4 == blockToNodeMap.end() ) {
				if( isBlockLocal(z4) ) {
					rankZ4 = -1;
				} else {
					out->fatal(CALL_INFO, -1, "Z- wireup for block failed to locate wire up partner (block: %" PRIu32 ")\n", z4);
				}
			}

		        out->verbose(CALL_INFO, 32, 0, "Mapping block %" PRIu32 " to nodes %" PRId32 ", %" PRId32 ", %" PRId32 ", %" PRId32 " in Z-\n",
				currentBlock->getBlockID(), rankZ1, rankZ2, rankZ3, rankZ4);

			currentBlock->setCommZDown(rankZ1, rankZ2, rankZ3, rankZ4);
	    maxRequests += 4;
        } else {
            // Same level
            const uint32_t blockNextToMe = calcBlockID(blockXPos,
                                                       blockYPos, blockZPos - 1, blockZDown);
            std::map<uint32_t, int32_t>::iterator blockNextToMeNode = blockToNodeMap.find(blockNextToMe);

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

	out->verbose(CALL_INFO, 2, 0, "Maximum requests from rank %" PRIu32 " will be set up: %" PRIu32 "\n", (uint32_t) rank(), maxRequests);
	requests   = (MessageRequest*)   malloc( sizeof(MessageRequest) * maxRequests * 2);
	maxRequestCount = maxRequests * 2;
	//requests = (MessageRequest*) malloc( sizeof(MessageRequest) * localBlocks.size() * 4 * 2);
	//maxRequestCount = localBlocks.size() * 2 * 4;

	out->verbose(CALL_INFO, 2, 0, "Requests allocated at: %p\n", requests);

	uint32_t maxFaceDim = std::max(blockNx, std::max(blockNy, blockNz));
	blockMessageBuffer = memAlloc( sizeof(double) * maxFaceDim * maxFaceDim * localBlocks.size() * 2);

        out->verbose(CALL_INFO, 2, 0, "Blocks on rank %" PRIu32 " count is: %" PRIu32 "\n", (uint32_t) rank(), (uint32_t) localBlocks.size());

	delete amrFile;
}

void Ember3DAMRGenerator::configure()
{
	out->verbose(CALL_INFO, 2, 0, "Configuring AMR motif...\n");

	char* newPrefix = (char*) malloc(sizeof(char) * 64);

	if(out->getVerboseLevel() > 8) {
		sprintf(newPrefix, "AMR3D[%7" PRIu32 "](@f:@p:@l): ", (uint32_t) rank());
	} else {
		sprintf(newPrefix, "AMR3D[%7" PRIu32 "](@p:@l): ", (uint32_t) rank());
	}

	std::string newPrefixStr(newPrefix);
	out->setPrefix(newPrefixStr);

	free(newPrefix);

	loadBlocks();

	// Clear the path string
	free(blockFilePath);

	// Clear system wide block wire up map
	//blockToNodeMap.clear();

	out->verbose(CALL_INFO, 2, 0, "Motif configuration is complete.\n");
}

void Ember3DAMRGenerator::postBlockCommunication(std::queue<EmberEvent*>& evQ, int32_t* blockComm, uint32_t* nextReq, const uint32_t faceSize,
	const uint32_t msgTag, const Ember3DAMRBlock* theBlock) {

	const uint32_t maxFaceDim = std::max(blockNx, std::max(blockNy, blockNz));
	char* bufferPtr = (char*) blockMessageBuffer;

	const int32_t thisRank = rank();

	for(uint32_t i = 0; i < 4; ++i) {
		if(blockComm[i] >= 0) {
			if(blockComm[i] != thisRank) {
				out->verbose(CALL_INFO, 16, 0, "Setting up exchange with rank %" PRId32 " for message size: %" PRIu32 ", tag: %" PRIu32 "\n",
					blockComm[i], faceSize, msgTag);

				out->verbose(CALL_INFO, 32, 0, "Enqueue non-blocking recv from: %" PRIu32 " on rank %" PRIu32 ", size: %" PRIu32 " doubles, tag: %" PRIu32 ", blockID=%" PRIu32 " (%" PRId32 ",%" PRId32 ",%" PRId32 ",%" PRId32 ",%" PRId32 ",%" PRId32 ")\n",
					blockComm[i], rank(), items_per_cell * faceSize, msgTag, theBlock->getBlockID(),
					theBlock->getRefineXDown(),
                                        theBlock->getRefineXUp(),
                                        theBlock->getRefineYDown(),
                                        theBlock->getRefineYUp(),
                                        theBlock->getRefineZDown(),
                                        theBlock->getRefineZUp());

				if( (*nextReq) < maxRequestCount ) {
					enQ_irecv( evQ, &bufferPtr[(*nextReq) * maxFaceDim * maxFaceDim],
						items_per_cell * faceSize, DOUBLE, blockComm[i], msgTag, GroupWorld, &requests[(*nextReq)]);
					(*nextReq) = (*nextReq) + 1;
				} else {
					out->fatal(CALL_INFO, -1, "Error: max requests at: %" PRIu32 ", current total: %" PRIu32 "\n", maxRequestCount, (*nextReq));
				}

				out->verbose(CALL_INFO, 32, 0, "Enqueue non-blocking send to: %" PRIu32 " from rank %" PRIu32 ", size: %" PRIu32 " doubles, tag: %" PRIu32 ", blockID=%" PRIu32 " (%" PRId32 ",%" PRId32 ",%" PRId32 ",%" PRId32 ",%" PRId32 ",%" PRId32 ")\n",
					blockComm[i], rank(), items_per_cell * faceSize, msgTag, theBlock->getBlockID(),
					theBlock->getRefineXDown(),
					theBlock->getRefineXUp(),
					theBlock->getRefineYDown(),
					theBlock->getRefineYUp(),
					theBlock->getRefineZDown(),
					theBlock->getRefineZUp());

				if( (*nextReq) < maxRequestCount) {
					enQ_isend( evQ, &bufferPtr[(*nextReq) * maxFaceDim * maxFaceDim],
						items_per_cell * faceSize, DOUBLE, blockComm[i], msgTag, GroupWorld, &requests[(*nextReq)]);
					(*nextReq) = (*nextReq) + 1;
				} else {
					out->fatal(CALL_INFO, -1, "Error: max requests at: %" PRIu32 ", current total: %" PRIu32 "\n", maxRequestCount, (*nextReq));
				}

			}
		}
	}
}

void Ember3DAMRGenerator::aggregateCommBytes(Ember3DAMRBlock* curBlock, std::map<int32_t, uint32_t>& blockToMessageSize) {

	const int32_t thisRank = rank();

	curBlock->print();

	for(int i = 0 ; i < 4; ++i) {
		int32_t* commPartners = curBlock->getCommXDown();
		if(commPartners[i] >= 0 && commPartners[i] != thisRank) {
			blockToMessageSize[commPartners[i]] =
				blockToMessageSize[commPartners[i]] + blockNy * blockNz * 8;
		}

		commPartners = curBlock->getCommXUp();
		if(commPartners[i] >= 0 && commPartners[i] != thisRank) {
			blockToMessageSize[commPartners[i]] =
				blockToMessageSize[commPartners[i]] + blockNy * blockNz * 8;
		}

		commPartners = curBlock->getCommYDown();
		if(commPartners[i] >= 0 && commPartners[i] != thisRank) {
			blockToMessageSize[commPartners[i]] =
				blockToMessageSize[commPartners[i]] + blockNx * blockNz * 8;
		}

		commPartners = curBlock->getCommYUp();
		if(commPartners[i] >= 0 && commPartners[i] != thisRank) {
			blockToMessageSize[commPartners[i]] =
				blockToMessageSize[commPartners[i]] + blockNx * blockNz * 8;
		}

		commPartners = curBlock->getCommZDown();
		if(commPartners[i] >= 0 && commPartners[i] != thisRank) {
			blockToMessageSize[commPartners[i]] =
				blockToMessageSize[commPartners[i]] + blockNy * blockNx * 8;
		}

		commPartners = curBlock->getCommZUp();
		if(commPartners[i] >= 0 && commPartners[i] != thisRank) {
			blockToMessageSize[commPartners[i]] =
				blockToMessageSize[commPartners[i]] + blockNy * blockNx * 8;
		}
	}
}

void Ember3DAMRGenerator::aggregateBlockCommunication(const std::vector<Ember3DAMRBlock*>& blocks,
	std::map<int32_t, uint32_t>& blockToMessageSize) {

	const uint32_t blocksCount = blocks.size();

	for(uint32_t i = 0; i < blocksCount; ++i) {
		Ember3DAMRBlock* currentBlock = blocks[i];
		aggregateCommBytes(currentBlock, blockToMessageSize);
	}
}

bool Ember3DAMRGenerator::generate( std::queue<EmberEvent*>& evQ)
{
	if(iteration < maxIterations) {
		enQ_compute( evQ, 5 );

		out->verbose(CALL_INFO, 8, 0, "Executing iteration: %" PRIu32 " on rank %" PRIu32 ", local blocks count: %" PRIu32 "\n", iteration,
			(uint32_t) rank(), (uint32_t) localBlocks.size());

//		for(uint32_t i = 0; i < localBlocks.size(); ++i) {
			out->verbose(CALL_INFO, 8, 0, "Loading information for block: %" PRIu32 " out of %" PRIu64 "\n",
				nextBlockToBeProcessed, (uint64_t)localBlocks.size());
			Ember3DAMRBlock* currentBlock = localBlocks[nextBlockToBeProcessed];

			out->verbose(CALL_INFO, 16, 0, "Creating communication events for block %" PRIu32 "\n", currentBlock->getBlockID());

			out->verbose(CALL_INFO, 32, 0, "-> Processing X-Down direction...\n");
			postBlockCommunication(evQ, currentBlock->getCommXDown(), &nextRequestID, blockNy * blockNz, 1001, currentBlock);

			out->verbose(CALL_INFO, 32, 0, "-> Processing X-Down direction...\n");
			postBlockCommunication(evQ, currentBlock->getCommXUp(),   &nextRequestID, blockNy * blockNz, 1001, currentBlock);

			out->verbose(CALL_INFO, 32, 0, "-> Processing Y-Down direction...\n");
			postBlockCommunication(evQ, currentBlock->getCommYDown(), &nextRequestID, blockNx * blockNz, 2001, currentBlock);

			out->verbose(CALL_INFO, 32, 0, "-> Processing Y-Up direction...\n");
			postBlockCommunication(evQ, currentBlock->getCommYUp(),   &nextRequestID, blockNx * blockNz, 2001, currentBlock);

			out->verbose(CALL_INFO, 32, 0, "-> Processing Z-Down direction...\n");
			postBlockCommunication(evQ, currentBlock->getCommZDown(), &nextRequestID, blockNx * blockNy, 4001, currentBlock);

			out->verbose(CALL_INFO, 32, 0, "-> Processing Z-Up direction...\n");
			postBlockCommunication(evQ, currentBlock->getCommZUp(),   &nextRequestID, blockNx * blockNy, 4001, currentBlock);

			out->verbose(CALL_INFO, 16, 0, "Block %" PRIu32 " complete.\n", currentBlock->getBlockID());
//		}

		nextBlockToBeProcessed++;

		if(nextBlockToBeProcessed == localBlocks.size()) {
			if(nextRequestID > 0) {
				out->verbose(CALL_INFO, 2, 0, "Enqueued %" PRIu32 " events, issuing wait all against them on rank %" PRIu32 "\n",
					(uint32_t) nextRequestID, (uint32_t) rank());
				enQ_waitall( evQ, nextRequestID, &requests[0] );
			} else {
				out->verbose(CALL_INFO, 2, 0, "Enqueued no communication events, stepping over wait-all issue.\n");
			}

			iteration++;
			nextBlockToBeProcessed = 0;
			nextRequestID = 0;
		}
/*
		std::map<int32_t, uint32_t> messageSizeMap;

		for(int i = 0; i < (int) size(); ++i) {
			messageSizeMap.insert( std::pair<int32_t, uint32_t>((int32_t) i, (uint32_t) 0) );
		}

		aggregateBlockCommunication(localBlocks, messageSizeMap);

		std::map<int32_t, uint32_t>::iterator messageSizeItr;
		for(messageSizeItr = messageSizeMap.begin(); messageSizeItr != messageSizeMap.end(); messageSizeItr++) {
			printf("Rank %" PRIu32 " to rank %" PRId32 " aggregated bytes: %" PRIu32 "\n",
				(uint32_t) rank(), messageSizeItr->first, messageSizeItr->second);
		}
*/
//		iteration++;
		return false;
	} else {
		char* timeBuffer = (char*) malloc(sizeof(char) * 64);
		sprintf(timeBuffer, "%" PRIu64 "ns", getCurrentSimTimeNano());
		UnitAlgebra timeUA(timeBuffer);

		out->verbose(CALL_INFO, 2, 0, "Completed %" PRIu32 " iterations @ time=%s, will now complete and unload.\n",
			iteration, timeUA.toString().c_str());

		free(timeBuffer);

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
//	const uint32_t blocksZLevel = blocksZ * power2(blockLevel);
	const uint32_t block_plane = indexDiff % (blocksXLevel * blocksYLevel);

	*posZ = indexDiff / (blocksXLevel * blocksYLevel);
	*posY = block_plane / blocksXLevel;

	const uint32_t block_remain = block_plane % blocksXLevel;

	*posX = (block_remain != 0) ? block_remain : 0;
}

uint32_t Ember3DAMRGenerator::calcBlockID(const uint32_t posX, const uint32_t posY, const uint32_t posZ,
	const uint32_t level) {

	out->verbose(CALL_INFO, 32, 0, "Calculating block ID for position (%" PRIu32 ", %" PRIu32 ", %" PRIu32 ")  at level %" PRIu32 "...\n",
		posX, posY, posZ, level);

	uint32_t startIndex = 0;

	for(uint32_t nextLevel = 0; nextLevel < level; ++nextLevel) {
		startIndex += ( (blocksX * power2(nextLevel)) * (blocksY * power2(nextLevel))
			* (blocksZ * power2(nextLevel)) );
	}

	out->verbose(CALL_INFO, 32, 0, "Start index of refinement level is: %" PRIu32 "\n", startIndex);

	const uint32_t blocksXLevel = blocksX * power2(level);
	const uint32_t blocksYLevel = blocksY * power2(level);
	const uint32_t blocksZLevel = blocksZ * power2(level);

	out->verbose(CALL_INFO, 32, 0, "Refinement %" PRIu32 " blocks X=%" PRIu32 ", Y=%" PRIu32 ", Z=%" PRIu32 "\n",
		level, blocksXLevel, blocksYLevel, blocksZLevel);

	out->verbose(CALL_INFO, 32, 0, "Calculate block ID, startIndex=%" PRIu32 " + %" PRIu32 " + %" PRIu32 " + %" PRIu32 "\n",
		startIndex, posX, (posY * blocksXLevel), (posZ * blocksXLevel * blocksYLevel));

	const uint32_t final_location = startIndex + posX + (posY * blocksXLevel) +
		(posZ * blocksXLevel * blocksYLevel);

	out->verbose(CALL_INFO, 32, 0, "Final block ID is %" PRIu32 "\n", final_location);

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
	std::map<uint32_t, int32_t>::iterator block_itr;

	char* map_output = (char*) malloc(sizeof(char) * PATH_MAX);
	sprintf(map_output, "blocks-%" PRIu32 ".map", rank());

	FILE* map_output_file = fopen(map_output, "wt");

	for(block_itr = blockToNodeMap.begin(); block_itr != blockToNodeMap.end(); block_itr++) {
		fprintf(map_output_file, "Block %" PRIu32 " maps to node: %" PRId32 "\n",
			block_itr->first, block_itr->second);
	}

	fclose(map_output_file);

	sprintf(map_output, "blocks-local-%" PRIu32 ".map", rank());
	map_output_file = fopen(map_output, "wt");

	assert(map_output_file != NULL);

	for(unsigned int i = 0; i < localBlocks.size(); ++i) {
		fprintf(map_output_file, "Block: %" PRIu32 "\n", localBlocks[i]->getBlockID());
	}

	fclose(map_output_file);
	free(map_output);
}

Ember3DAMRGenerator::~Ember3DAMRGenerator() {
	delete out;
	memFree(blockMessageBuffer);
}

