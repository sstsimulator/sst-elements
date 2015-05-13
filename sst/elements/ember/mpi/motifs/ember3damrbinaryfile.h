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


#ifndef _H_SST_ELEMENTS_EMBER_AMR_BINARY_FILE
#define _H_SST_ELEMENTS_EMBER_AMR_BINARY_FILE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ember3damrfile.h"

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
