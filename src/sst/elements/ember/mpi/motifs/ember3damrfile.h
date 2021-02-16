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


#ifndef _H_SST_ELEMENTS_EMBER_AMR_FILE
#define _H_SST_ELEMENTS_EMBER_AMR_FILE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace SST {
namespace Ember {

class EmberAMRFile {

public:
	EmberAMRFile(char* amrPath, Output* out) :
		amrFilePath(amrPath),
		output(out) {
	}

	virtual ~EmberAMRFile() {

	}

    virtual void readNodeMeshLine(uint32_t* blockCount) = 0;
	virtual void readNextMeshLine(uint32_t* blockID, uint32_t* refineLev,
		int32_t* xDown, int32_t* xUp,
		int32_t* yDown, int32_t* yUp,
        int32_t* zDown, int32_t* zUp) = 0;

	uint32_t getBlocksX() const { return (uint32_t) blocksX; }
	uint32_t getBlocksY() const { return (uint32_t) blocksY; }
	uint32_t getBlocksZ() const { return (uint32_t) blocksZ; }

	uint32_t getBlockCount() const { return (uint32_t) totalBlockCount; }
	uint32_t getMaxRefinement() const { return (uint32_t) maxRefinementLevel; }

	virtual bool isBinary() { return false; }
        virtual void locateRankEntries(uint32_t rank) {
        	output->fatal(CALL_INFO, -1, "Called locate on an AMR file which does not support seek operations.\n");
	}

protected:
	char* amrFilePath;
	Output* output;
	FILE* amrFile;

	int totalBlockCount;
	int maxRefinementLevel;
	int blocksX;
	int blocksY;
	int blocksZ;

};

}
}

#endif
