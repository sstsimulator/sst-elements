// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_SST_ELEMENTS_EMBER_AMR_BINARY_FILE
#define _H_SST_ELEMENTS_EMBER_AMR_BINARY_FILE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ember3damrfile.h"
#include "ember3damrblock.h"

// To clean up warnings on not using return results on freads() below
#if ((__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
#endif

namespace SST {
    namespace Ember {
        
        class EmberAMRBinaryFile : public EmberAMRFile {
            
        public:
            EmberAMRBinaryFile(char* amrPath, Output* out) :
                EmberAMRFile(amrPath, out) {
                
                amrFile = fopen(amrFilePath, "rb");
                
                if(NULL == amrFile) {
                    output->fatal(CALL_INFO, -1, "Unable to open file: %s\n", amrPath);
                }
                
                rankCount = 0;
                fread(&rankCount, sizeof(rankCount), 1, amrFile);
                
                    uint32_t meshBlockCount = 0;
                    fread(&meshBlockCount, sizeof(meshBlockCount), 1, amrFile);
                    
                    uint8_t meshMaxRefineLevel = 0;
                    fread(&meshMaxRefineLevel, sizeof(meshMaxRefineLevel), 1, amrFile);
                    
                    uint32_t meshBlocksX = 0;
                    fread(&meshBlocksX, sizeof(meshBlocksX), 1, amrFile);
                    
                    uint32_t meshBlocksY = 0;
                    fread(&meshBlocksY, sizeof(meshBlocksY), 1, amrFile);
                    
                    uint32_t meshBlocksZ = 0;
                    fread(&meshBlocksZ, sizeof(meshBlocksZ), 1, amrFile);
                    
                    blocksX = (int) meshBlocksX;
                    blocksY = (int) meshBlocksY;
                    blocksZ = (int) meshBlocksZ;
          
                    totalBlockCount = (int) meshBlockCount;
                    maxRefinementLevel = (int) meshMaxRefineLevel;

                    out->verbose(CALL_INFO, 8, 0, "Read mesh header info: blocks=%" PRIu32 ", max-lev: %" PRIu32 " bkX=%" PRIu32 ", blkY=%" PRIu32 ", blkZ=%" PRIu32 "\n",
                             totalBlockCount, maxRefinementLevel, blocksX, blocksY, blocksZ);

		    rankIndexOffset = ftell(amrFile);

		    const uint64_t meshStartIndex = rankIndexOffset + (rankCount * sizeof(uint64_t));

		    out->verbose(CALL_INFO, 8, 0, "Set mesh file seek to: %" PRIu64 "\n", meshStartIndex);
		    fseek(amrFile, (long)(meshStartIndex), SEEK_SET);

            }
            
            ~EmberAMRBinaryFile() {
        	fclose(amrFile);        
            }

	    void populateGlobalBlocks(std::map<uint32_t, int32_t>* globalBlockMap) {
		fseek(amrFile, rankIndexOffset + (rankCount * sizeof(uint64_t)), SEEK_SET);

		for(uint32_t i = 0; i < rankCount; ++i) {
			uint32_t blocksOnNode = 0;
			uint32_t blockID;
			uint32_t refineLevel;
			int32_t  xDown;
			int32_t  xUp;
			int32_t  yDown;
			int32_t  yUp;
			int32_t  zDown;
			int32_t  zUp;

			readNodeMeshLine(&blocksOnNode);

			for(uint32_t j = 0; j < blocksOnNode; j++) {
				readNextMeshLine(&blockID, &refineLevel,
					&xDown, &xUp, &yDown, &yUp, &zDown, &zUp);

				auto block_present = globalBlockMap->find(blockID);

				if(block_present != globalBlockMap->end()) {
					output->fatal(CALL_INFO, -1, "Block ID: %" PRIu32 " already in map.\n", blockID);
				}

				globalBlockMap->insert(std::pair<uint32_t, int32_t>(blockID, (int32_t) i));
			}
		}
            }

	    void populateLocalBlocks(std::vector<Ember3DAMRBlock*>* localBlocks, uint32_t rank) {
		const uint64_t seekOffset = rankIndexOffset + (rank * sizeof(uint64_t));

		output->verbose(CALL_INFO, 16, 0, "Seek file offet: Base=%" PRIu64 ", Rank=%" PRIu32 ", Offset=%" PRIu64 ", File Seek=%" PRIu64 "\n",
			rankIndexOffset, rank, (uint64_t)(rank * sizeof(uint64_t)), seekOffset);

		fseek(amrFile, seekOffset, SEEK_SET);

		uint64_t rankBlocksIndex = 0;
		fread(&rankBlocksIndex, sizeof(rankBlocksIndex), 1, amrFile);

		output->verbose(CALL_INFO, 16, 0, "Rank Offset: %" PRIu64 ", seeking in file...\n", rankBlocksIndex);

		fseek(amrFile, rankBlocksIndex, SEEK_SET);

		uint32_t blocksOnNode = 0;
		readNodeMeshLine(&blocksOnNode);

		output->verbose(CALL_INFO, 16, 0, "Rank has %" PRIu32 " blocks on the the node.\n", blocksOnNode);

		uint32_t blockID;
                       	uint32_t refineLevel;
                       	int32_t  xDown;
                       	int32_t  xUp;
                       	int32_t  yDown;
                       	int32_t  yUp;
                       	int32_t  zDown;
                       	int32_t  zUp;

		for(uint32_t i = 0; i < blocksOnNode; ++i) {
			readNextMeshLine(&blockID, &refineLevel,
                             	&xDown, &xUp, &yDown, &yUp, &zDown, &zUp);

			output->verbose(CALL_INFO, 32, 0, "Read Block: %" PRIu32 " X-:%" PRId32 ", X+:%" PRId32 ", Y-:%" PRId32 ", Y+:%" PRId32 " Z-:%" PRId32 " Z+:%" PRId32 "\n",
				blockID, xDown, xUp, yDown, yUp, zDown, zUp);

			Ember3DAMRBlock* newBlock = new Ember3DAMRBlock(blockID,
				refineLevel, xDown, xUp, yDown, yUp, zDown, zUp);

			localBlocks->push_back(newBlock);
		}
	    }

            void readNodeMeshLine(uint32_t* blockCount) {
                fread(blockCount, sizeof(uint32_t), 1, amrFile);
            }

	    void locateRankEntries(uint32_t rank) {
		fseek(amrFile, (long) (rankIndexOffset + (rank * sizeof(uint64_t))), SEEK_SET);

		uint64_t rankStart = 0;
		fread(&rankStart, sizeof(rankStart), 1, amrFile);

		fseek(amrFile, (long) (rankStart), SEEK_SET);
	    }

            void readNextMeshLine(uint32_t* blockID, uint32_t* refineLev,
                                  int32_t* xDown, int32_t* xUp,
                                  int32_t* yDown, int32_t* yUp,
                                  int32_t* zDown, int32_t* zUp) {
                int8_t temp = 0;

                fread(blockID, sizeof(uint32_t), 1, amrFile);

		fread(&temp, sizeof(temp), 1, amrFile);
		*refineLev = (uint32_t) temp;

                fread(&temp, sizeof(temp), 1, amrFile);
                *xDown = (int32_t) temp;

                fread(&temp, sizeof(temp), 1, amrFile);
                *xUp = (int32_t) temp;

                fread(&temp, sizeof(temp), 1, amrFile);
                *yDown = (int32_t) temp;

                fread(&temp, sizeof(temp), 1, amrFile);
                *yUp = (int32_t) temp;

                fread(&temp, sizeof(temp), 1, amrFile);
                *zDown = (int32_t) temp;
                
                fread(&temp, sizeof(temp), 1, amrFile);
                *zUp = (int32_t) temp;
            }

	    virtual bool isBinary() {
		return true;
	    }

        private:
            uint32_t rankCount;
	    uint64_t rankIndexOffset;
            FILE* amrFile;
        };
        
    }
}

#if ((__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
#pragma GCC diagnostic pop
#endif

#endif
