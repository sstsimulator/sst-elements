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

Ember3DAMRGenerator::Ember3DAMRGenerator(SST::Component* owner, Params& params) :
	EmberMessagePassingGenerator(owner, params)
{
	m_name = "3DCommDoubling";

//	peX = (uint32_t) params.find_integer("arg.pex", 0);
//	peY = (uint32_t) params.find_integer("arg.pey", 0);
//	peZ = (uint32_t) params.find_integer("arg.pez", 0);

	std::string blockpath = params.find_string("blockfile", "blocks.amr");
        blockFilePath = (char*) malloc(sizeof(char) * (blockpath.size() + 1));

        strcpy(blockFilePath, blockpath.c_str());
        blockFilePath[blockpath.size()] = '\0';
}

void Ember3DAMRGenerator::loadBlocks() {
	printf("LOADING BLOCKS FOR AMR");

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
			currentBlock->setCommXUp(-1, -1, -1, -1);
		} else if(blockLevel > blockXUp) {
			// Communication to a coarser level (my refinement is higher than block next to me)
			const uint32_t commToBlock = calcBlockID((blockXPos / 2) + 1,
				blockYPos / 2, blockZPos / 2, blockXUp);

			std::map<uint32_t, uint32_t>::iterator blockNode = blockToNodeMap.find(commToBlock);

			if(blockNode == blockToNodeMap.end() && isBlockLocal(commToBlock)) {
				if( ! isBlockLocal(commToBlock) ) {
					printf("X- Did not locate block: %" PRIu32 "\n", commToBlock);
					exit(-1);
				}
			} else {
				// Projecting to coarse level
				currentBlock->setCommXUp(blockNode->second, -1, -1, -1);
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
				printf("Fail on XUp, block x1");
				exit(-1);
			}

			if( blockNodeX2 == blockToNodeMap.end() && (! isBlockLocal(x2)) ) {
				printf("Fail on XUp, block x2");
				exit(-1);
			}

			if( blockNodeX3 == blockToNodeMap.end() && (! isBlockLocal(x3)) ) {
				printf("Fail on XUp, block x3: %" PRIu32 "\n", x3);
				printBlockMap();
				exit(-1);
			}

			if( blockNodeX4 == blockToNodeMap.end() && (! isBlockLocal(x4)) ) {
				printf("Fail on XUp, block x4: %" PRIu32 "\n", x4);
				exit(-1);
			}

			currentBlock->setCommXUp(blockNodeX1->second,
				blockNodeX2->second,
				blockNodeX3->second,
				blockNodeX4->second);
		} else {
			// Same level
			const uint32_t blockNextToMe = calcBlockID(blockXPos + 1,
				blockYPos, blockZPos, blockXUp);

			std::map<uint32_t, uint32_t>::iterator blockNextToMeNode = blockToNodeMap.find(blockNextToMe);

			if(blockNextToMeNode == blockToNodeMap.end()) {
				if( ! isBlockLocal(blockNextToMe) ) {
					printf("X- Did not find block next %" PRIu32 ", current block=%" PRIu32 "\n",
						blockNextToMe, currentBlock->getBlockID());
					exit(-1);
				}
			} else {
				currentBlock->setCommXUp(blockNextToMeNode->second, -1, -1, -1);
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
					printf("X+ Did not locate block: %" PRIu32 "\n", commToBlock);
					exit(-1);
				}
			} else {
				// Projecting to coarse level
				currentBlock->setCommXDown(blockNode->second, -1, -1, -1);
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
				printf("Fail on XDown, block x1");
				exit(-1);
			}

			if( blockNodeX2 == blockToNodeMap.end() && (! isBlockLocal(x2)) ) {
				printf("Fail on XDown, block x2");
				exit(-1);
			}

			if( blockNodeX3 == blockToNodeMap.end() && (! isBlockLocal(x3)) ) {
				printf("Fail on XDown, block x3: %" PRIu32 "\n", x3);
				printBlockMap();
				exit(-1);
			}

			if( blockNodeX4 == blockToNodeMap.end() && (! isBlockLocal(x4)) ) {
				printf("Fail on XDown, block x4: %" PRIu32 "\n", x4);
				exit(-1);
			}

			currentBlock->setCommXDown(blockNodeX1->second,
				blockNodeX2->second,
				blockNodeX3->second,
				blockNodeX4->second);

		} else {
			// Same level
			const uint32_t blockNextToMe = calcBlockID(blockXPos - 1,
				blockYPos, blockZPos, blockXDown);

			std::map<uint32_t, uint32_t>::iterator blockNextToMeNode = blockToNodeMap.find(blockNextToMe);

			if(blockNextToMeNode == blockToNodeMap.end()) {
				if( ! isBlockLocal(blockNextToMe) ) {
					printf("X+ Did not find block next %" PRIu32 ", current block=%" PRIu32 "\n",
						blockNextToMe, currentBlock->getBlockID());
					exit(-1);
				}
			} else {
				currentBlock->setCommXDown(blockNextToMeNode->second, -1, -1, -1);
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
                printf("Fail on YUp, block y1");
                exit(-1);
            }

            if( blockNodeY2 == blockToNodeMap.end() && (! isBlockLocal(y2)) ) {
                printf("Fail on YUp, block y2");
                exit(-1);
            }

            if( blockNodeY3 == blockToNodeMap.end() && (! isBlockLocal(y3)) ) {
                printf("Fail on YUp, block y3: %" PRIu32 "\n", y3);
                printBlockMap();
                exit(-1);
            }

            if( blockNodeY4 == blockToNodeMap.end() && (! isBlockLocal(y4)) ) {
                printf("Fail on YUp, block y4: %" PRIu32 "\n", y4);
                exit(-1);
            }
            currentBlock->setCommYUp(blockNodeY1->second,
                                     blockNodeY2->second,
                                     blockNodeY3->second,
                                     blockNodeY4->second);
        } else {
            // Same level
            const uint32_t blockNextToMe = calcBlockID(blockXPos,
                                                       blockYPos + 1, blockZPos, blockYUp);
            std::map<uint32_t, uint32_t>::iterator blockNextToMeNode = blockToNodeMap.find(blockNextToMe);

            if(blockNextToMeNode == blockToNodeMap.end()) {
                if( ! isBlockLocal(blockNextToMe) ) {
                    printf("Y+ Did not find block next %" PRIu32 ", current block=%" PRIu32 "\n",
                           blockNextToMe, currentBlock->getBlockID());
                    exit(-1);
                }
            } else {
                currentBlock->setCommYUp(blockNextToMeNode->second, -1, -1, -1);
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
                printf("Fail on YDown, block y1");
                exit(-1);
            }

            if( blockNodeY2 == blockToNodeMap.end() && (! isBlockLocal(y2)) ) {
                printf("Fail on YDown, block y2");
                exit(-1);
            }

            if( blockNodeY3 == blockToNodeMap.end() && (! isBlockLocal(y3)) ) {
                printf("Fail on YDown, block y3: %" PRIu32 "\n", y3);
                printBlockMap();
                exit(-1);
            }

            if( blockNodeY4 == blockToNodeMap.end() && (! isBlockLocal(y4)) ) {
                printf("Fail on YDown, block y4: %" PRIu32 "\n", y4);
                exit(-1);
            }

            currentBlock->setCommYDown(blockNodeY1->second,
                                       blockNodeY2->second,
                                       blockNodeY3->second,
                                       blockNodeY4->second);

        } else {
            // Same level
            const uint32_t blockNextToMe = calcBlockID(blockXPos,
                                                       blockYPos - 1, blockZPos, blockYDown);

            std::map<uint32_t, uint32_t>::iterator blockNextToMeNode = blockToNodeMap.find(blockNextToMe);

            if(blockNextToMeNode == blockToNodeMap.end()) {
                if( ! isBlockLocal(blockNextToMe) ) {
                    printf("Y- Did not find block next %" PRIu32 ", current block=%" PRIu32 "\n",
                           blockNextToMe, currentBlock->getBlockID());
                    exit(-1);
                }
            } else {
                currentBlock->setCommYDown(blockNextToMeNode->second, -1, -1, -1);
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
                printf("Fail on ZUp, block y1");
                exit(-1);
            }
            
            if( blockNodeZ2 == blockToNodeMap.end() && (! isBlockLocal(z2)) ) {
                printf("Fail on ZUp, block y2");
                exit(-1);
            }
            
            if( blockNodeZ3 == blockToNodeMap.end() && (! isBlockLocal(z3)) ) {
                printf("Fail on ZUp, block y3: %" PRIu32 "\n", z3);
                printBlockMap();
                exit(-1);
            }
            
            if( blockNodeZ4 == blockToNodeMap.end() && (! isBlockLocal(z4)) ) {
                printf("Fail on ZUp, block y4: %" PRIu32 "\n", z4);
                exit(-1);
            }
            currentBlock->setCommZUp(blockNodeZ1->second,
                                     blockNodeZ2->second,
                                     blockNodeZ3->second,
                                     blockNodeZ4->second);
        } else {
            // Same level
            const uint32_t blockNextToMe = calcBlockID(blockXPos,
                                                       blockYPos, blockZPos + 1, blockZUp);
            std::map<uint32_t, uint32_t>::iterator blockNextToMeNode = blockToNodeMap.find(blockNextToMe);
            
            if(blockNextToMeNode == blockToNodeMap.end()) {
                if( ! isBlockLocal(blockNextToMe) ) {
                    printf("Y+ Did not find block next %" PRIu32 ", current block=%" PRIu32 "\n",
                           blockNextToMe, currentBlock->getBlockID());
                    exit(-1);
                }
            } else {
                currentBlock->setCommZUp(blockNextToMeNode->second, -1, -1, -1);
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
                    printf("Y- Did not locate block: %" PRIu32 "\n", commToBlock);
                    exit(-1);
                }
            } else {
                // Projecting to coarse level
                currentBlock->setCommZDown(blockNode->second, -1, -1, -1);
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
                printf("Fail on ZDown, block z1");
                exit(-1);
            }
            
            if( blockNodeZ2 == blockToNodeMap.end() && (! isBlockLocal(z2)) ) {
                printf("Fail on ZDown, block z2");
                exit(-1);
            }
            
            if( blockNodeZ3 == blockToNodeMap.end() && (! isBlockLocal(z3)) ) {
                printf("Fail on ZDown, block z3: %" PRIu32 "\n", z3);
                printBlockMap();
                exit(-1);
            }
            
            if( blockNodeZ4 == blockToNodeMap.end() && (! isBlockLocal(z4)) ) {
                printf("Fail on ZDown, block z4: %" PRIu32 "\n", z4);
                exit(-1);
            }
            
            currentBlock->setCommZDown(blockNodeZ1->second,
                                       blockNodeZ2->second,
                                       blockNodeZ3->second,
                                       blockNodeZ4->second);
            
        } else {
            // Same level
            const uint32_t blockNextToMe = calcBlockID(blockXPos,
                                                       blockYPos, blockZPos - 1, blockZDown);
            
            std::map<uint32_t, uint32_t>::iterator blockNextToMeNode = blockToNodeMap.find(blockNextToMe);
            
            if(blockNextToMeNode == blockToNodeMap.end()) {
                if( ! isBlockLocal(blockNextToMe) ) {
                    printf("Y- Did not find block next %" PRIu32 ", current block=%" PRIu32 "\n",
                           blockNextToMe, currentBlock->getBlockID());
                    exit(-1);
                }
            } else {
                currentBlock->setCommZDown(blockNextToMeNode->second, -1, -1, -1);
            }
        }
	}

	printf("Blocks on rank %" PRIu32 " count=%lu\n", rank, localBlocks.size());
}

void Ember3DAMRGenerator::configure()
{
//	if((peX * peY * peZ) != (unsigned) size()) {
//		m_output->fatal(CALL_INFO, -1, "Processor decomposition of %" PRIu32 "x%" PRIu32 "x%" PRIu32 " != rank count of %" PRIu32 "\n",
//			peX, peY, peZ, size());
//	}

	loadBlocks();
	free(blockFilePath);
}

bool Ember3DAMRGenerator::generate( std::queue<EmberEvent*>& evQ)
{
	return true;
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

}

